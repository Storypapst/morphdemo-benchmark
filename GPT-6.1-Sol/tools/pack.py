#!/usr/bin/env python3
"""An original, deterministic LZSS packer and native ELF builder."""
from collections import defaultdict, deque
from pathlib import Path
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parent.parent

def compact_elf(data):
    b = bytearray(data)
    phoff = struct.unpack_from('<Q', b, 32)[0]
    phsize, phnum = struct.unpack_from('<HH', b, 54)
    end = 0
    programs = []
    for i in range(phnum):
        p = struct.unpack_from('<IIQQQQQQ', b, phoff + i * phsize)
        programs.append(p)
        end = max(end, p[2] + p[5])
    # Nix's wrapped linker inserts a build-machine RUNPATH. The documented
    # nix-ld loader already resolves all four sonames. Remove that redundant
    # entry and its unused string without moving any ELF addresses.
    dynamic = next((p for p in programs if p[0] == 2), None)
    if dynamic:
        off, length = dynamic[2], dynamic[5]
        entries = []
        for pos in range(off, off + length, 16):
            tag, value = struct.unpack_from('<QQ', b, pos)
            if tag == 0:
                break
            entries.append((tag, value))
        strings_va = next(value for tag, value in entries if tag == 5)
        load = next(p for p in programs if p[0] == 1 and p[3] <= strings_va < p[3] + p[5])
        strings = load[2] + strings_va - load[3]
        for tag, value in entries:
            if tag in (15, 29):
                start = strings + value
                stop = b.index(0, start)
                b[start:stop] = bytes(stop - start)
        entries = [(tag, value) for tag, value in entries if tag not in (15, 29)]
        encoded = b''.join(struct.pack('<QQ', *entry) for entry in entries)
        b[off:off+length] = encoded + bytes(length - len(encoded))
    struct.pack_into('<Q', b, 40, 0)
    struct.pack_into('<HHH', b, 58, 0, 0, 0)
    return bytes(b[:end])

def compress(data):
    n = len(data)
    chains = defaultdict(deque)
    matches = [None] * n
    for i in range(n):
        key = data[i:i+3]
        positions = chains[key]
        while positions and i - positions[0] > 4096:
            positions.popleft()
        best = {}
        maximum = 2
        for p in reversed(positions):
            length = 3
            while length < 18 and i + length < n and data[p+length] == data[i+length]:
                length += 1
            if i + length > n:
                continue
            if length > maximum:
                for k in range(maximum + 1, length + 1):
                    best[k] = i - p
                maximum = length
                if length == 18:
                    break
        matches[i] = best
        positions.append(i)
    # Minimum byte cost, including each group's flag byte.
    costs = [[0] * 8 for _ in range(n + 1)]
    choices = [None] * n
    for i in range(n - 1, -1, -1):
        row = []
        for state in range(8):
            nxt = (state + 1) & 7
            best_cost = 1 + costs[i+1][nxt]
            selected = (1, 0)
            for length, distance in matches[i].items():
                c = 2 + costs[i+length][nxt]
                if c <= best_cost:
                    best_cost, selected = c, (length, distance)
            costs[i][state] = best_cost + (state == 0)
            row.append(selected)
        choices[i] = row
    packed = bytearray()
    i = state = 0
    while i < n:
        if state == 0:
            flag_index = len(packed)
            packed.append(0)
        length, distance = choices[i][state]
        if distance:
            packed[flag_index] |= 1 << state
            packed += struct.pack('<H', ((length - 3) << 12) | (distance - 1))
        else:
            packed.append(data[i])
        i += length
        state = (state + 1) & 7
    return bytes(packed)

def decompress(data, size):
    out = bytearray()
    i = 0
    while len(out) < size:
        flags = data[i]
        i += 1
        for bit in range(8):
            if flags & (1 << bit):
                token = struct.unpack_from('<H', data, i)[0]
                i += 2
                length, distance = (token >> 12) + 3, (token & 4095) + 1
                for _ in range(length):
                    out.append(out[-distance])
            else:
                out.append(data[i])
                i += 1
            if len(out) == size:
                return bytes(out)
    raise ValueError('bad stream')

def main():
    source, target, size = Path(sys.argv[1]), Path(sys.argv[2]), int(sys.argv[3])
    payload = compact_elf(source.read_bytes())
    source.write_bytes(payload)
    source.chmod(0o755)
    if size == 65536:
        result = payload
        packed_size = len(result)
    else:
        packed = compress(payload)
        assert decompress(packed, len(payload)) == payload
        packed_path = source.with_suffix('.lz')
        packed_path.write_bytes(packed)
        obj = source.with_suffix('.stub.o')
        elf = source.with_suffix('.stub.elf')
        raw = source.with_suffix('.stub.bin')
        subprocess.run(['gcc', '-c', 'src/unpack.S', '-o', str(obj),
                        f'-DPAYLOAD_LENGTH={len(payload)}',
                        f'-DPAYLOAD_FILE="{packed_path}"'], check=True, cwd=ROOT)
        subprocess.run(['ld', '--build-id=none', '-Ttext=0', '-e', 'elf', '-o', str(elf), str(obj)], check=True, cwd=ROOT)
        subprocess.run(['objcopy', '-O', 'binary', '-j', '.text', str(elf), str(raw)], check=True, cwd=ROOT)
        result = raw.read_bytes()
        packed_size = len(packed)
    if len(result) > size:
        raise SystemExit(f'{target}: {len(result)} bytes exceeds {size} (payload {len(payload)}, stream {packed_size})')
    target.write_bytes(result + bytes(size - len(result)))
    target.chmod(0o755)
    print(f'{target}: ELF {len(payload)}; stream {packed_size}; executable {len(result)}; final {size}')

if __name__ == '__main__':
    main()

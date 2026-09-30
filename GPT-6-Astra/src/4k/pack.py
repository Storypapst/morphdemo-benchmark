#!/usr/bin/env python3
"""Deterministic native LZSS self-extractor; no runtime unpacker dependency."""
import pathlib,struct,subprocess
root=pathlib.Path(__file__).resolve().parents[2]
build=root/'build/4k'
raw=(build/'payload').read_bytes()
# ELF section tables and names are not needed by the dynamic loader.
phoff=struct.unpack_from('<Q',raw,32)[0]
phnum=struct.unpack_from('<H',raw,56)[0]
end=max(struct.unpack_from('<QQ',raw,phoff+i*56+8)[0]+struct.unpack_from('<Q',raw,phoff+i*56+32)[0] for i in range(phnum))
raw=bytearray(raw[:end])
struct.pack_into('<Q',raw,40,0)
struct.pack_into('<HHH',raw,58,0,0,0)
data=bytes(raw)
(build/'payload.trim').write_bytes(data)
matches=[]
for pos in range(len(data)):
    best,dist=0,0
    if pos+3<=len(data):
        needle=data[pos:pos+3]
        found=data.rfind(needle,max(0,pos-4096),pos)
        while found>=0:
            length=3
            while length<273 and pos+length<len(data) and data[found+length]==data[pos+length]: length+=1
            if length>best: best,dist=length,pos-found
            if best==273:break
            found=data.rfind(needle,max(0,pos-4096),found)
    matches.append((best,dist))
# An optimal parse avoids greedy long matches hiding cheaper later matches.
cost=[0]*(len(data)+1)
choice=[1]*len(data)
for pos in range(len(data)-1,-1,-1):
    cost[pos]=9+cost[pos+1]
    for length in range(3,matches[pos][0]+1):
        value=(17 if length<18 else 25)+cost[pos+length]
        if value<cost[pos]:cost[pos],choice[pos]=value,length
tokens=[]
pos=0
while pos<len(data):
    best=choice[pos]
    dist=matches[pos][1]
    if best>=3:
        word=((min(best,18)-3)<<12)|(dist-1)
        tokens.append((1,struct.pack('<H',word)+(bytes([best-18]) if best>=18 else b'')))
        pos+=best
    else:
        tokens.append((0,data[pos:pos+1]));pos+=1
packed=bytearray()
for i in range(0,len(tokens),8):
    group=tokens[i:i+8]
    packed.append(sum(flag<<j for j,(flag,_) in enumerate(group)))
    for _,token in group: packed+=token
# Verify the format independently before generating machine code. Overlapping
# copies deliberately repeat previously emitted bytes, exactly as REP MOVSB.
decoded=bytearray()
cursor=0
while len(decoded)<len(data):
    flag=packed[cursor];cursor+=1
    for bit in range(8):
        if len(decoded)==len(data):break
        if flag>>bit&1:
            word=int.from_bytes(packed[cursor:cursor+2],'little');cursor+=2
            length=(word>>12)+3
            distance=(word&4095)+1
            if length==18:length+=packed[cursor];cursor+=1
            for _ in range(length):decoded.append(decoded[-distance])
        else:
            decoded.append(packed[cursor]);cursor+=1
assert decoded==data and cursor==len(packed), 'LZSS round-trip failed'
(build/'payload.lz').write_bytes(packed)
asm=f'''.global _start
.section .text
_start:
    mov %rsp,%r15
    lea packed(%rip),%rsi
    mov $0x500000,%edi
nextgroup:
    lodsb
    mov %eax,%r8d
    mov $8,%r9d
nexttoken:
    shr $1,%r8b
    jc match
    movsb
    jmp tokenend
match:
    xor %eax,%eax
    lodsw
    mov %eax,%edx
    shr $12,%eax
    lea 3(%rax),%ecx
    cmp $18,%ecx
    jne shortmatch
    lodsb
    movzbl %al,%eax
    add %eax,%ecx
shortmatch:
    and $4095,%edx
    inc %edx
    push %rsi
    mov %rdi,%rsi
    sub %rdx,%rsi
    rep movsb
    pop %rsi
tokenend:
    cmp ${0x500000+len(data)},%edi
    jae unpacked
    dec %r9d
    jnz nexttoken
    jmp nextgroup
unpacked:
    lea empty(%rip),%rdi
    xor %esi,%esi
    mov $319,%eax
    syscall
    mov %eax,%r12d
    mov %eax,%edi
    mov $0x500000,%esi
    mov ${len(data)},%edx
    mov $1,%eax
    syscall
    mov %r12d,%edi
    lea empty(%rip),%rsi
    lea 8(%r15),%rdx
    mov (%r15),%rax
    lea 16(%r15,%rax,8),%r10
    mov $4096,%r8d
    mov $322,%eax
    syscall
    mov $60,%eax
    mov $127,%edi
    syscall
empty:.byte 0
packed:.incbin "build/4k/payload.lz"
'''
(build/'unpack.S').write_text(asm)
subprocess.run(['gcc','-c','-nostdlib','-o',str(build/'unpack.o'),str(build/'unpack.S')],cwd=root,check=True)
subprocess.run(['objcopy','-O','binary','-j','.text',str(build/'unpack.o'),str(build/'unpack.bin')],check=True)
code=(build/'unpack.bin').read_bytes()
header=struct.pack('<16sHHIQQQIHHHHHH',b'\x7fELF\x02\x01\x01'+bytes(9),2,62,1,0x400078,64,0,0,64,56,1,0,0,0)
size=len(header)+56+len(code)
header+=struct.pack('<IIQQQQQQ',1,7,0,0x400000,0x400000,size,0x100000+len(data),0x1000)
print(f'4k: payload {len(data)}; compressed {len(packed)}; native stub {len(code)-len(packed)+120}; total {size}/4096')
if size>4096:raise SystemExit('4k size budget exceeded')
output=root/'morphdemo-4k'
output.write_bytes(header+code+bytes(4096-size))
output.chmod(0o755)

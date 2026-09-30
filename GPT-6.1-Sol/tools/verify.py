#!/usr/bin/env python3
"""Exercise each final ELF, with real graphics and real system audio output.
All observations and test working directories stay under verification/.
"""
import array
from datetime import datetime, timezone
import gzip
import hashlib
import json
import math
import os
from pathlib import Path
import re
import subprocess
import time
import wave
from x11 import Display

ROOT=Path(__file__).resolve().parent.parent
OUTPUT=ROOT/'verification'
EXPECTED={'morphdemo-64k':65536,'morphdemo-16k':16384,'morphdemo-4k':4096}
ENV=dict(os.environ,XDG_CACHE_HOME=str(ROOT/'.cache'),__GL_SHADER_DISK_CACHE='0')
# The executables choose the supplied XWayland interface themselves. These
# tests do not provide windowing or audio configuration to the productions.
ENV.pop('SDL_VIDEODRIVER',None)
ENV.pop('SDL_AUDIODRIVER',None)

def check(condition,message):
    if not condition:raise AssertionError(message)

def audio_stats(path):
    with wave.open(str(path),'rb') as w:
        rate,channels=w.getframerate(),w.getnchannels()
        check(w.getsampwidth()==2,'Expected signed 16-bit monitor capture')
        samples=array.array('h',w.readframes(w.getnframes()))
    windows={}
    for label,start,end in [('emergence',6,10),('transformation',20,24),('resolution',44,48),('coda',60,62)]:
        section=samples[start*rate*channels:end*rate*channels]
        rms=math.sqrt(sum(float(x)*x for x in section)/len(section))/32768
        peak=max(abs(x) for x in section)/32768
        check(rms>.0005,f'{label}: inaudible/empty output capture ({rms})')
        windows[label]={'rms':round(rms,6),'peak':round(peak,6)}
    peak=max(abs(x) for x in samples)/32768
    check(peak<.999,'Clipped monitor capture')
    difference=sum(abs(samples[i]-samples[i+1]) for i in range(0,len(samples)-1,2))/len(samples)
    check(difference>1,'Stereo output did not differ between channels')
    return {'rate_hz':rate,'channels':channels,'peak':round(peak,6),'phases':windows}

def audit(trace_path,executable):
    trace=trace_path.read_text()
    check(not re.search(r'socket\(AF_INET6?[,)]',trace),'Internet socket opened')
    executions=[line for line in trace.splitlines() if re.search(r'\bexecve(?:at)?\(',line)]
    unexpected=[line for line in executions if 'execveat(' not in line and str(executable) not in line]
    check(not unexpected,f'External executable invoked: {unexpected}')
    memfd=[line for line in executions if 'execveat(' in line]
    if executable.name in ('morphdemo-4k','morphdemo-16k'):
        check(len(memfd)==1 and 'AT_EMPTY_PATH' in memfd[0],'Missing internal memfd execution')
        check('memfd_create("MORPH", 0)' in trace,'Missing internal memfd creation')
    # Ignore the initial execve of the final ELF. None of the other filesystem
    # operations may load source, shaders, captures, or build intermediates.
    reads=[line for line in trace.splitlines() if 'execve' not in line and str(ROOT) in line and re.search(r'\b(?:open|openat|access|stat|statx|newfstatat)\(',line)]
    forbidden=[line for line in reads if any(part in line for part in ['/src/','/tools/','/build/','/verification/'])]
    check(not forbidden,f'Production assets/intermediates accessed: {forbidden[:4]}')
    result={'internet_sockets':0,'external_executables':0,'internal_memfd_execs':len(memfd),'source_or_asset_reads':0}
    # Socket creation/connection already proves the absence of Internet
    # access. Keep operational traces compact and omit local IPC bodies.
    retained='\n'.join(line for line in trace.splitlines()
                       if not re.search(r'\b(?:sendmsg|recvmsg|sendto|recvfrom)\b',line))+'\n'
    with gzip.open(str(trace_path)+'.gz','wb',compresslevel=6) as f:f.write(retained.encode())
    trace_path.unlink()
    return result

def run_complete(name,display):
    executable=ROOT/name
    out=OUTPUT/name
    out.mkdir(parents=True,exist_ok=True)
    empty=out/'empty-working-directory'
    empty.mkdir(exist_ok=True)
    check(not list(empty.iterdir()),'The asset-free working directory is not empty')
    audio_path=out/'output.wav'
    capture_log=(out/'audio-capture.log').open('w')
    capture=subprocess.Popen(['ffmpeg','-y','-hide_banner','-loglevel','error','-f','pulse','-i','@DEFAULT_MONITOR@',
                              '-t','66','-ar','48000','-ac','2','-c:a','pcm_s16le',str(audio_path)],
                             cwd=empty,env=ENV,stdout=capture_log,stderr=capture_log)
    time.sleep(.3)
    check(capture.poll() is None,'Audio monitor failed to start')
    trace_path=out/'runtime.trace'
    log_path=out/'runtime.log'
    phases=[]
    targets=[(5,True),(5.5,False),(20,True),(20.5,False),(42,True),(42.5,False),(56,True),(63,True)]
    with log_path.open('w') as log:
        process=subprocess.Popen(['strace','-f','-qq','-e','trace=file,process,socket,connect,memfd_create',
                                  '-o',str(trace_path),str(executable)],cwd=empty,env=ENV,stdout=log,stderr=log)
        start=time.monotonic();window=None;index=0
        try:
            while process.poll() is None:
                elapsed=time.monotonic()-start
                check(elapsed<70,f'{name}: exceeded 70 seconds')
                if window is None:
                    found=display.find(f'MORPH / {name.split("-")[1].upper()} /')
                    if found:window=found[0]
                    elif elapsed>10:raise AssertionError(f'{name}: no visible window')
                if window is not None and index<len(targets) and elapsed>=targets[index][0]:
                    t,save=targets[index]
                    w,h,rgb=display.capture(window,out/f'frame-{t:g}.png' if save else None)
                    check((w,h)==(1920,1080),f'{name}: wrong drawable size {w}x{h}')
                    mean=sum(rgb)/len(rgb)
                    check(mean>1,f'{name}: blank frame at {t} s')
                    phases.append({'at_seconds':t,'width':w,'height':h,'mean_rgb':round(mean,4),
                                   'sha256':hashlib.sha256(rgb).hexdigest()})
                    print(f'{name}: visible frame at {t:g} s',flush=True)
                    index+=1
                time.sleep(.05)
            duration=time.monotonic()-start
            check(process.returncode==0,f'{name}: exit code {process.returncode}: {log_path.read_text()}')
            check(50<=duration<=70,f'{name}: duration {duration}')
            check(index==len(targets),f'{name}: incomplete sequence')
            for a,b in zip(phases[0:6:2],phases[1:6:2]):
                check(a['sha256']!=b['sha256'],f'{name}: static animation at {a["at_seconds"]} s')
        finally:
            if process.poll() is None:process.terminate();process.wait(timeout=10)
            if capture.poll() is None:
                try:capture.wait(timeout=10)
                except subprocess.TimeoutExpired:capture.terminate();capture.wait(timeout=5)
            capture_log.close()
    check(capture.returncode==0,f'{name}: audio capture failed')
    check(not list(empty.iterdir()),f'{name}: wrote to its working directory')
    stats=audio_stats(audio_path)
    trace=audit(trace_path,executable)
    log=log_path.read_text()
    match=re.search(r'Ended cleanly: ([0-9.]+) s, ([0-9]+) frames',log)
    result={'file_bytes':executable.stat().st_size,'sha256':hashlib.sha256(executable.read_bytes()).hexdigest(),
            'exit_code':0,'wall_seconds':round(duration,3),'empty_cwd':True,'frames':phases,'audio':stats,'trace':trace}
    if match:
        result['playback_seconds']=float(match[1])
        result['rendered_frames']=int(match[2])
        result['average_fps']=round(int(match[2])/float(match[1]),2)
    print(f'{name}: complete, {duration:.3f} s; audio peak {stats["peak"]:.3f}',flush=True)
    return result

def run_escape(name,display):
    out=OUTPUT/name
    with (out/'escape.log').open('w') as log:
        process=subprocess.Popen([str(ROOT/name)],cwd=out/'empty-working-directory',env=ENV,stdout=log,stderr=log)
        start=time.monotonic();window=None
        try:
            while time.monotonic()-start<10:
                found=display.find(f'MORPH / {name.split("-")[1].upper()} /')
                if found:window=found[0];break
                check(process.poll() is None,f'{name}: failed to start Escape test')
                time.sleep(.05)
            check(window is not None,f'{name}: Escape test window absent')
            time.sleep(3)
            sent=time.monotonic();display.escape(window)
            code=process.wait(timeout=5)
            latency=time.monotonic()-sent
            check(code==0,f'{name}: Escape exit code {code}')
            check(display.find(f'MORPH / {name.split("-")[1].upper()} /') is None,'Window persisted after Escape')
            print(f'{name}: Escape exited cleanly in {latency:.3f} s',flush=True)
            return {'exit_code':code,'latency_seconds':round(latency,3)}
        finally:
            if process.poll() is None:process.terminate();process.wait(timeout=10)

def main():
    OUTPUT.mkdir(exist_ok=True)
    report={'verified_at_utc':datetime.now(timezone.utc).isoformat(),'environment':'ENVIRONMENT.json',
            'master_clock':'Generated stereo audio, 48000 Hz, 120 BPM, 64 seconds','executables':{}}
    for name,size in EXPECTED.items():
        data=(ROOT/name).read_bytes()
        check(len(data)==size,f'{name}: size {len(data)} != {size}')
        check(data[:4]==b'\x7fELF' and data[4]==2 and data[18:20]==b'\x3e\x00',f'{name}: not x86-64 ELF')
    display=Display()
    try:
        for name in EXPECTED:
            result=run_complete(name,display)
            result['escape']=run_escape(name,display)
            report['executables'][name]=result
            (OUTPUT/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    finally:display.close()
    print('All three final native executables passed.',flush=True)

if __name__=='__main__':main()

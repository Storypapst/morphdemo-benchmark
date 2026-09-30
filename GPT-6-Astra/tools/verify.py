#!/usr/bin/env python3
"""Run final demos in an asset-free working directory and record their output.
Usage: python3 tools/verify.py [4k|16k|64k] [--escape]
Full checks last about three minutes. All generated files remain in this tree.
"""
import argparse, array, hashlib, json, math, os, pathlib, re, shutil, signal, subprocess, time, wave
ROOT = pathlib.Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('classes', nargs='*', metavar='CLASS', help='4k, 16k, or 64k; defaults to all three')
p.add_argument('--escape', action='store_true')
a=p.parse_args()
classes=a.classes or ['4k','16k','64k']
if any(c not in ['4k','16k','64k'] for c in classes):p.error('CLASS must be 4k, 16k, or 64k')
lib=ROOT/'build/tools/audit.so'
lib.parent.mkdir(parents=True,exist_ok=True)
(ROOT/'build/tmp').mkdir(parents=True,exist_ok=True)
os.environ['TMPDIR']=str(ROOT/'build/tmp')
subprocess.run(['gcc','-O2','-shared','-fPIC',str(ROOT/'tools/audit.c'),'-o',str(lib),'-ldl','-lm'],check=True,cwd=ROOT)
for cls in classes:
    binary=ROOT/f'morphdemo-{cls}'
    data=binary.read_bytes()
    expected={'4k':4096,'16k':16384,'64k':65536}[cls]
    assert len(data)==expected,(cls,len(data),expected)
    assert data[:4]==b'\x7fELF',f'{cls}: not ELF'
    target=ROOT/'verification'/(cls+('-escape' if a.escape else ''))
    target.mkdir(parents=True,exist_ok=True)
    for pattern in ['frame-*.ppm','frame-*.png','audio.f32','audio.wav','metrics.json','report.json','contact-sheet.png']:
        for old in target.glob(pattern):old.unlink()
    isolated=target/'isolated'
    isolated.mkdir(exist_ok=True)
    assert not list(isolated.iterdir()),f'{isolated} is not empty'
    executable=isolated/binary.name
    shutil.copy2(binary,executable)
    env=os.environ.copy()
    env['LD_PRELOAD']=str(lib)
    cache=ROOT/'build/cache'
    cache.mkdir(parents=True,exist_ok=True)
    env['__GL_SHADER_DISK_CACHE_PATH']=str(cache)
    env['MESA_SHADER_CACHE_DIR']=str(cache)
    env['MORPH_AUDIT_DIR']=str(target)
    if a.escape:env['MORPH_AUDIT_ESCAPE']='2'
    else:env.pop('MORPH_AUDIT_ESCAPE',None)
    command=['strace','-f','-qq','-e','trace=openat,execve,execveat,socket,connect','-o',str(target/'syscalls.log'),str(executable)]
    print(f'Running {binary.name} ({"Escape" if a.escape else "complete production"})',flush=True)
    start=time.monotonic()
    try:
        with (target/'runtime.log').open('w') as log:
            process=subprocess.Popen(command,cwd=isolated,env=env,stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
            try:
                process.wait(timeout=80)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid,signal.SIGKILL)
                process.wait()
                raise
            result=process
        elapsed=time.monotonic()-start
    finally:
        executable.unlink(missing_ok=True)
        isolated.rmdir()
    report={'binary':binary.name,'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest(),'wall_seconds':round(elapsed,6),'exit_code':result.returncode,'test':'escape' if a.escape else 'complete','asset_free_working_directory':True}
    metrics=target/'metrics.json'
    if metrics.exists():report.update(json.loads(metrics.read_text()))
    raw=target/'audio.f32'
    if raw.exists() and report.get('audio_format')==0x8120:
        samples=array.array('f');samples.frombytes(raw.read_bytes())
        rate=report['audio_rate'];channels=report['audio_channels']
        report['audio_seconds']=round(len(samples)/rate/channels,6)
        segment=rate*channels*5
        report['audio_rms_5s']=[round(math.sqrt(sum(v*v for v in samples[i:i+segment])/len(samples[i:i+segment])),6) for i in range(0,len(samples),segment)]
        pcm=array.array('h',(int(max(-1,min(1,v))*32767) for v in samples))
        with wave.open(str(target/'audio.wav'),'wb') as wav:
            wav.setnchannels(channels);wav.setsampwidth(2);wav.setframerate(rate);wav.writeframes(pcm.tobytes())
        raw.unlink()
    try:
        from PIL import Image, ImageDraw
        files=sorted(target.glob('frame-*.ppm'))
        if files:
            sheet=Image.new('RGB',(960,300*math.ceil(len(files)/2)),(8,10,16))
            draw=ImageDraw.Draw(sheet)
            for i,path in enumerate(files):
                frame=Image.open(path)
                frame.save(path.with_suffix('.png'))
                frame.thumbnail((480,270))
                x=i%2*480;y=i//2*300
                sheet.paste(frame,(x,y))
                draw.text((x+8,y+275),f'{cls}  { [3,12,23,34,44,53,57,59][i] } s',fill=(230,230,230))
                path.unlink()
            sheet.save(target/'contact-sheet.png')
    except ImportError:pass
    syscalls=(target/'syscalls.log').read_text()
    report['internet_sockets']=len(re.findall(r'socket\(AF_INET',syscalls))
    report['exec_calls']=[line for line in syscalls.splitlines() if re.search(r'\bexecve(?:at)?\(',line)]
    assert report['internet_sockets']==0,f'{cls}: internet socket observed'
    expected_exec=2 if cls=='4k' else 1
    assert len(report['exec_calls'])==expected_exec,f'{cls}: unexpected executable delegation'
    (target/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2),flush=True)
    assert result.returncode==0,f'{cls}: exit {result.returncode}'
    if a.escape:
        assert report.get('escape_injected'),f'{cls}: Escape was not injected'
        assert elapsed<12,f'{cls}: Escape took too long'
    else:
        assert 50<=elapsed<=70,f'{cls}: wall time outside specification: {elapsed}'
        assert 50<=report.get('render_seconds',0)<=70,f'{cls}: render duration outside specification'
        assert report.get('audio_rms',0)>0.001,f'{cls}: silent audio'
        assert report.get('audio_clipped',-1)==0,f'{cls}: clipped synthesized audio'
        assert report.get('frames',0)>1000,f'{cls}: unexpectedly low frame count'

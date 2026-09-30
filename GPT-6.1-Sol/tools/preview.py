#!/usr/bin/env python3
"""Capture selected points of a real-time run, and optionally test Escape."""
import os
from pathlib import Path
import subprocess
import sys
import time
from x11 import Display

root=Path(__file__).resolve().parent.parent
name=sys.argv[1]
times=[float(x) for x in sys.argv[2:] if x!='escape'] or [5,22,44,56]
stop='escape' in sys.argv
env=dict(os.environ,XDG_CACHE_HOME=str(root/'.cache'),__GL_SHADER_DISK_CACHE='0',SDL_VIDEODRIVER='x11')
out=root/'verification'/name
out.mkdir(parents=True,exist_ok=True)
with (out/'preview.log').open('w') as log:
    p=subprocess.Popen([str(root/name)],env=env,stdout=log,stderr=log,cwd=root)
    start=time.monotonic();display=Display();window=None
    try:
        while time.monotonic()-start<10:
            found=display.find()
            if found:window=found[0];print(found,flush=True);break
            if p.poll() is not None:raise RuntimeError(f'Exited: {p.returncode}: {(out/"preview.log").read_text()}')
            time.sleep(.1)
        if window is None:raise RuntimeError('No window')
        for t in times:
            while time.monotonic()-start<t:time.sleep(.1)
            w,h,rgb=display.capture(window,out/f'preview-{t:g}.png')
            print(t,w,h,'mean',sum(rgb)/len(rgb),flush=True)
        if stop:display.escape(window)
        result=p.wait(timeout=70)
        print('exit',result,'seconds',time.monotonic()-start,flush=True)
    finally:
        if p.poll() is None:p.terminate();p.wait()
        display.close()

"""Small X11 capture/event helper for verification, using installed libX11.
This is a development tool. None of it is loaded by a production executable.
"""
import ctypes as C
import struct
import zlib
from pathlib import Path

class Image(C.Structure):
    _fields_=[('width',C.c_int),('height',C.c_int),('xoffset',C.c_int),('format',C.c_int),
              ('data',C.c_void_p),('byte_order',C.c_int),('bitmap_unit',C.c_int),
              ('bitmap_bit_order',C.c_int),('bitmap_pad',C.c_int),('depth',C.c_int),
              ('bytes_per_line',C.c_int),('bits_per_pixel',C.c_int),
              ('red_mask',C.c_ulong),('green_mask',C.c_ulong),('blue_mask',C.c_ulong)]

class Key(C.Structure):
    _fields_=[('type',C.c_int),('serial',C.c_ulong),('send_event',C.c_int),
              ('display',C.c_void_p),('window',C.c_ulong),('root',C.c_ulong),
              ('subwindow',C.c_ulong),('time',C.c_ulong),('x',C.c_int),('y',C.c_int),
              ('x_root',C.c_int),('y_root',C.c_int),('state',C.c_uint),
              ('keycode',C.c_uint),('same_screen',C.c_int)]

def png(path,width,height,rgb):
    def chunk(kind,data):
        return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)
    raw=b''.join(b'\0'+rgb[i*width*3:(i+1)*width*3] for i in range(height))
    Path(path).write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',width,height,8,2,0,0,0))
                         +chunk(b'IDAT',zlib.compress(raw,6))+chunk(b'IEND',b''))

class Display:
    def __init__(self):
        self.x=C.CDLL('/run/current-system/sw/share/nix-ld/lib/libX11.so.6')
        self.x.XOpenDisplay.argtypes=[C.c_char_p];self.x.XOpenDisplay.restype=C.c_void_p
        self.x.XDefaultRootWindow.argtypes=[C.c_void_p];self.x.XDefaultRootWindow.restype=C.c_ulong
        self.x.XQueryTree.argtypes=[C.c_void_p,C.c_ulong,C.POINTER(C.c_ulong),C.POINTER(C.c_ulong),C.POINTER(C.POINTER(C.c_ulong)),C.POINTER(C.c_uint)]
        self.x.XFetchName.argtypes=[C.c_void_p,C.c_ulong,C.POINTER(C.c_void_p)]
        self.x.XGetGeometry.argtypes=[C.c_void_p,C.c_ulong,C.POINTER(C.c_ulong),C.POINTER(C.c_int),C.POINTER(C.c_int),C.POINTER(C.c_uint),C.POINTER(C.c_uint),C.POINTER(C.c_uint),C.POINTER(C.c_uint)]
        self.x.XGetImage.argtypes=[C.c_void_p,C.c_ulong,C.c_int,C.c_int,C.c_uint,C.c_uint,C.c_ulong,C.c_int]
        self.x.XGetImage.restype=C.POINTER(Image)
        self.x.XDestroyImage.argtypes=[C.POINTER(Image)]
        self.x.XSendEvent.argtypes=[C.c_void_p,C.c_ulong,C.c_int,C.c_long,C.c_void_p]
        self.x.XFlush.argtypes=[C.c_void_p]
        self.x.XFree.argtypes=[C.c_void_p]
        self.x.XCloseDisplay.argtypes=[C.c_void_p]
        self.d=self.x.XOpenDisplay(None)
        if not self.d:raise RuntimeError('No X11 display')
        self.root=self.x.XDefaultRootWindow(self.d)

    def children(self,window):
        root,parent=C.c_ulong(),C.c_ulong();children=C.POINTER(C.c_ulong)();n=C.c_uint()
        self.x.XQueryTree(self.d,window,C.byref(root),C.byref(parent),C.byref(children),C.byref(n))
        result=[children[i] for i in range(n.value)]
        if children:self.x.XFree(children)
        return result

    def find(self,prefix='MORPH /'):
        candidates=self.children(self.root)
        for window in candidates:
            name=C.c_void_p()
            if self.x.XFetchName(self.d,window,C.byref(name)) and name.value:
                value=C.string_at(name).decode(errors='replace');self.x.XFree(name)
                if value.startswith(prefix):return window,value
        return None

    def capture(self,window,path=None):
        root=C.c_ulong();x,y=C.c_int(),C.c_int();w,h,border,depth=[C.c_uint() for _ in range(4)]
        self.x.XGetGeometry(self.d,window,C.byref(root),C.byref(x),C.byref(y),C.byref(w),C.byref(h),C.byref(border),C.byref(depth))
        image=self.x.XGetImage(self.d,window,0,0,w,h,C.c_ulong(-1),2)
        if not image:raise RuntimeError('Cannot capture window')
        im=image.contents
        if im.bits_per_pixel!=32 or im.red_mask!=0xff0000:raise RuntimeError('Unexpected XImage layout')
        bgra=C.string_at(im.data,im.bytes_per_line*im.height)
        if im.bytes_per_line!=im.width*4:
            bgra=b''.join(bgra[i*im.bytes_per_line:i*im.bytes_per_line+im.width*4] for i in range(im.height))
        rgb=bytearray(im.width*im.height*3)
        rgb[0::3]=bgra[2::4];rgb[1::3]=bgra[1::4];rgb[2::3]=bgra[0::4]
        width,height=im.width,im.height
        self.x.XDestroyImage(image)
        if path:png(path,width,height,rgb)
        return width,height,rgb

    def escape(self,window):
        # XSendEvent targets this window, without changing global focus.
        event=C.create_string_buffer(192)
        key=Key.from_buffer(event)
        key.type=2;key.display=self.d;key.window=window;key.root=self.root
        key.same_screen=1;key.keycode=9
        self.x.XSendEvent(self.d,window,0,1,event)
        key.type=3;self.x.XSendEvent(self.d,window,0,2,event)
        self.x.XFlush(self.d)

    def close(self):self.x.XCloseDisplay(self.d)

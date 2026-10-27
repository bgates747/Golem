#!/usr/bin/env python3
"""Convert this fixture's binary PPM captures using only the standard library."""
from pathlib import Path
import struct
import zlib
import sys

def read(path):
 data=Path(path).read_bytes();magic,size,scale,pixels=data.split(b'\n',3)
 assert magic==b'P6' and scale==b'255';w,h=map(int,size.split());assert len(pixels)==w*h*3
 return w,h,pixels

def convert(path):
 w,h,rgb=read(path)
 def chunk(kind,data):return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data))
 png=b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',w,h,8,2,0,0,0))
 scan=b''.join(b'\0'+rgb[y*w*3:(y+1)*w*3] for y in range(h))
 png+=chunk(b'IDAT',zlib.compress(scan,9))+chunk(b'IEND',b'')
 target=Path(path).with_suffix('.png');target.write_bytes(png);return target
if __name__=='__main__':print(convert(sys.argv[1]))

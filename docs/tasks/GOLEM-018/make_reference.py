#!/usr/bin/env python3
"""Draw a dependency-free layout reference, not an emulator screenshot."""
from pathlib import Path
import struct
import zlib

TASK = Path(__file__).resolve().parent
WIDTH, HEIGHT = 1320, 520
pixels = bytearray(bytes((28,32,38)) * WIDTH * HEIGHT)
SCALE = 2


def put(x,y,colour):
    if 0 <= x < WIDTH and 0 <= y < HEIGHT:
        offset = (y*WIDTH+x)*3
        pixels[offset:offset+3] = bytes(colour)


def scene(origin,colour,clipped):
    ox,oy = origin
    def dot(x,y,c=colour):
        if 0 <= x < 320 and 0 <= y < 240:
            for sy in range(SCALE):
                for sx in range(SCALE): put(ox+x*SCALE+sx,oy+y*SCALE+sy,c)
    for y in range(240):
        for x in range(320): dot(x,y,(0,0,0))
    point = (-1,2) if clipped else (24,50)
    line = (-10,30,24,46) if clipped else (60,50,130,100)
    quad = [(-8,4),(16,4),(20,20),(-12,20)] if clipped else [(180,60),(260,60),(280,120),(160,120)]
    # Ideal convex fill; stock triangle edge rules can differ by a boundary pixel.
    for y in range(max(0,min(p[1] for p in quad)),min(239,max(p[1] for p in quad))+1):
        for x in range(max(0,min(p[0] for p in quad)),min(319,max(p[0] for p in quad))+1):
            sides = [(b[0]-a[0])*(y-a[1])-(b[1]-a[1])*(x-a[0]) for a,b in zip(quad,quad[1:]+quad[:1])]
            if all(v>=0 for v in sides) or all(v<=0 for v in sides): dot(x,y)
    x,y,x2,y2 = line
    dx,dy = abs(x2-x),-abs(y2-y)
    sx,sy = (1 if x<x2 else -1),(1 if y<y2 else -1)
    error = dx+dy
    while True:
        dot(x,y)
        if (x,y)==(x2,y2): break
        twice = 2*error
        if twice>=dy: error+=dy;x+=sx
        if twice<=dx: error+=dx;y+=sy
    for x,y in [point,(27,50),(30,50),(24,53),(27,53),(30,53),(24,56),(27,56),(30,56)]: dot(x,y)
    # Thin frame marks the actual screen limits, including the clipped left edge.
    for x in range(ox-1,ox+641): put(x,oy-1,(150,160,175));put(x,oy+480,(150,160,175))
    for y in range(oy-1,oy+481): put(ox-1,y,(150,160,175));put(ox+640,y,(150,160,175))


scene((10,20),(0,170,170),True) # Logical 6 -> physical 0x0A in stock mode 8.
scene((670,20),(255,255,0),False) # Logical 11 -> physical 0x3C.
def chunk(kind,data):
    return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)
scan = b''.join(b'\0'+pixels[y*WIDTH*3:(y+1)*WIDTH*3] for y in range(HEIGHT))
png = b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',WIDTH,HEIGHT,8,2,0,0,0))
png += chunk(b'IDAT',zlib.compress(scan))+chunk(b'IEND',b'')
(TASK/'reference-layout.png').write_bytes(png)
print('Wrote reference-layout.png: left cyan clipping test; right final yellow geometry; text omitted.')

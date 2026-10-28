"""Convert a 240x240 24-bit BMP into the FM-1's five-colour RLE boot splash."""
import struct, sys
src, dst = sys.argv[1:3]
b = open(src, 'rb').read()
off = struct.unpack_from('<I', b, 10)[0]
w, hs, depth = struct.unpack_from('<ii', b, 18)[0], struct.unpack_from('<i', b, 22)[0], struct.unpack_from('<H', b, 28)[0]
h, row = abs(hs), (w * 3 + 3) & ~3
if b[:2] != b'BM' or (w, h, depth) != (240, 240, 24): raise SystemExit('need a 240x240 24-bit BMP')
pal = [(0,0,0),(0,28,54),(0,220,235),(224,28,190),(116,56,185)]
px=[]
for y in range(h):
    sy = h-1-y if hs > 0 else y
    for x in range(w):
        p = off + sy*row + x*3; c=(b[p+2],b[p+1],b[p])
        px.append(min(range(len(pal)), key=lambda k: sum((c[i]-pal[k][i])**2 for i in range(3))))
rle=[]; i=0
while i < len(px):
    j=i+1
    while j < len(px) and px[j] == px[i] and j-i < 16: j+=1
    rle.append(((j-i-1)<<4)|px[i]); i=j
head=['/* Custom cassette boot splash. */','#pragma once','#include <stdint.h>','#define SLOOP_SPLASH_W 240','#define SLOOP_SPLASH_H 240',
      'static const uint16_t SLOOP_SPLASH_PAL[16] = {0x0000, 0x00E6, 0x06FD, 0xD8F7, 0x71D6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};',f'static const uint8_t SLOOP_SPLASH_RLE[{len(rle)}] = {{']
head += ['    '+', '.join(map(str,rle[i:i+24]))+',' for i in range(0,len(rle),24)]
open(dst,'w').write('\n'.join(head+['};','']))
print(f'boot splash: {len(rle)} B RLE')

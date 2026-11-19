#!/usr/bin/env python3
"""assets/container.png (512x512, RGB) жасайды: тек стандартты кітапхана.

Тапсырмамен берілген container.png болмағандықтан, оның орнына ағаш жәшік
суреті салынады. Сурет әдейі симметриялы емес (сол жақ жоғарыда "CG 11"
жазуы бар), сондықтан төңкерілгені/айнадағы көрінісі бірден көрінеді.
Жұқа тақтай сызықтары алыстағы кубтарда aliasing/жыпылықты көрсетуге де қажет.
"""
import struct
import sys
import zlib

W = H = 512
img = [[(0, 0, 0)] * W for _ in range(H)]

FONT = {  # 5x7 bitmap
    "C": ["01110", "10001", "10000", "10000", "10000", "10001", "01110"],
    "G": ["01110", "10001", "10000", "10111", "10001", "10001", "01111"],
    "1": ["00100", "01100", "00100", "00100", "00100", "00100", "01110"],
    " ": ["00000"] * 7,
}


def px(x, y, c):
    if 0 <= x < W and 0 <= y < H:
        img[y][x] = c


def rect(x0, y0, x1, y1, c):
    for y in range(y0, y1):
        for x in range(x0, x1):
            px(x, y, c)


def wood(x, y, base, seed):
    g = (((x * 7 + seed) % 23) < 2) * -18 + ((y * 3 + seed * 5) % 11 < 1) * -10
    r, gr, b = base
    return (max(0, r + g), max(0, gr + g), max(0, b + g))


# фон: тік тақтайлар (жұқа айырма сызықтарымен)
for y in range(H):
    for x in range(W):
        plank = x // 64
        c = wood(x, y, (176 - plank % 2 * 14, 122 - plank % 2 * 10, 66), plank * 13)
        if x % 64 in (0, 63):
            c = (70, 42, 20)
        img[y][x] = c

# қалың рама
FR = 44
frame = (120, 78, 38)
rect(0, 0, W, FR, frame); rect(0, H - FR, W, H, frame)
rect(0, 0, FR, H, frame); rect(W - FR, 0, W, H, frame)
for t in range(0, 4):  # рама шекарасындағы қараңғы сызық
    rect(t, 0, t + 1, H, (50, 30, 14)); rect(W - 1 - t, 0, W - t, H, (50, 30, 14))
    rect(0, t, W, t + 1, (50, 30, 14)); rect(0, H - 1 - t, W, H - t, (50, 30, 14))
rect(FR - 3, FR - 3, W - FR + 3, FR, (60, 36, 16)); rect(FR - 3, H - FR, W - FR + 3, H - FR + 3, (60, 36, 16))
rect(FR - 3, FR - 3, FR, H - FR + 3, (60, 36, 16)); rect(W - FR, FR - 3, W - FR + 3, H - FR + 3, (60, 36, 16))

# диагональ белгі (X)
for i in range(FR, W - FR):
    for k in range(-7, 8):
        for (xx, yy) in ((i + k, i), (W - 1 - i + k, i)):
            if FR < xx < W - FR and FR < yy < H - FR:
                px(xx, yy, (96, 60, 28))

# бұрыш шегелері
for cx, cy in ((22, 22), (W - 23, 22), (22, H - 23), (W - 23, H - 23)):
    for dy in range(-6, 7):
        for dx in range(-6, 7):
            if dx * dx + dy * dy <= 36:
                px(cx + dx, cy + dy, (200, 200, 205) if dx + dy < 2 else (110, 110, 118))

# "CG 11" жазуы сол жақ жоғарыда (асимметрия белгісі)
text, scale, ox, oy = "CG 11", 7, FR + 10, FR + 10
cx0 = ox
for ch in text:
    for ry, row in enumerate(FONT[ch]):
        for rx, bit in enumerate(row):
            if bit == "1":
                rect(cx0 + rx * scale, oy + ry * scale, cx0 + (rx + 1) * scale, oy + (ry + 1) * scale, (250, 235, 190))
    cx0 += 6 * scale

# PNG жазу
raw = b"".join(b"\x00" + bytes(v for p in row for v in p) for row in img)


def chunk(t, d):
    c = struct.pack(">I", len(d)) + t + d
    return c + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)


png = (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", W, H, 8, 2, 0, 0, 0))
       + chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b""))
out = sys.argv[1] if len(sys.argv) > 1 else "assets/container.png"
open(out, "wb").write(png)
print("жазылды:", out, len(png), "байт")

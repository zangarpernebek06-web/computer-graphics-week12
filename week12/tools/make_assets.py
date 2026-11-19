#!/usr/bin/env python3
"""assets/pattern.png (512x512, RGB) және assets/logo.png (512x512, RGBA) жасайды.

Тапсырмамен берілген файлдар қолда болмағандықтан, олардың орнына өзім салған суреттер:
  * pattern.png - түрлі-түсті ромб/диагональ өрнек (жәшіктен айқын ерекшеленеді);
  * logo.png    - мөлдір фонды призма белгісі. Шеттері жұмсақ (supersampling),
                  сондықтан альфасы 0 мен 255 арасындағы көптеген аралық пикселден тұрады.
Тек стандартты кітапхана. Іске қосу:  python3 tools/make_assets.py assets
"""
import math
import struct
import sys
import zlib

W = H = 512


def png(path, w, h, rows, color_type, channels):
    raw = b"".join(b"\x00" + bytes(r) for r in rows)

    def chunk(t, d):
        c = struct.pack(">I", len(d)) + t + d
        return c + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)

    data = (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, color_type, 0, 0, 0))
            + chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b""))
    open(path, "wb").write(data)
    print("жазылды:", path, len(data), "байт,", channels, "арна")


def pattern():
    rows = []
    for y in range(H):
        row = []
        for x in range(W):
            u, v = x / W, y / H
            # ромб торы
            d = abs((u * 8) % 1 - 0.5) + abs((v * 8) % 1 - 0.5)
            ring = int(d * 8) % 2
            r = 40 + 180 * ((u + v) % 1)
            g = 70 + 120 * (1 - u)
            b = 190 + 60 * v
            if ring:
                r, g, b = 255 - r * 0.5, 255 - g * 0.6, 255 - b * 0.3
            if x % 64 < 3 or y % 64 < 3:
                r, g, b = 255, 255, 255
            row += [int(max(0, min(255, c))) for c in (r, g, b)]
        rows.append(row)
    return rows


# призма: тең қабырғалы үшбұрыш + оған түскен сәуле мен кемпірқосақ
TRI = [(256, 90), (400, 360), (112, 360)]
BEAM = [(30, 330), (150, 290)]


def inside_tri(x, y):
    def s(p, q):
        return (x - q[0]) * (p[1] - q[1]) - (p[0] - q[0]) * (y - q[1])
    a, b, c = TRI
    d1, d2, d3 = s(a, b), s(b, c), s(c, a)
    neg = d1 < 0 or d2 < 0 or d3 < 0
    pos = d1 > 0 or d2 > 0 or d3 > 0
    return not (neg and pos)


def dist_edge(x, y):
    best = 1e9
    for i in range(3):
        (x1, y1), (x2, y2) = TRI[i], TRI[(i + 1) % 3]
        dx, dy = x2 - x1, y2 - y1
        t = max(0, min(1, ((x - x1) * dx + (y - y1) * dy) / (dx * dx + dy * dy)))
        best = min(best, math.hypot(x - (x1 + t * dx), y - (y1 + t * dy)))
    return best


RAINBOW = [(255, 60, 60), (255, 160, 40), (250, 230, 60), (70, 210, 90), (60, 150, 255), (150, 80, 240)]


def sample(x, y):
    """(r,g,b,a) 0..1 - нүктедегі түс және альфа (қатты шекара)."""
    # ішкі призма
    if inside_tri(x, y):
        e = dist_edge(x, y)
        if e < 9:
            return (1.0, 1.0, 1.0, 1.0)  # ақ жиек
        t = (y - 90) / 270.0
        return (0.25 + 0.3 * t, 0.55 + 0.25 * (1 - t), 0.95, 0.92)
    # кіретін ақ сәуле
    if 30 <= x <= 190 and abs((y - (330 - (x - 30) * 0.25)) ) < 7:
        return (1.0, 1.0, 1.0, 1.0)
    # шығатын кемпірқосақ сәулелері
    for i, col in enumerate(RAINBOW):
        cy = 300 + (x - 330) * (0.05 + 0.07 * (i - 2.5)) + (i - 2.5) * 6
        if 330 <= x <= 490 and abs(y - cy) < 6:
            return (col[0] / 255, col[1] / 255, col[2] / 255, 1.0)
    return (0.0, 0.0, 0.0, 0.0)


def logo():
    SS = 3  # 3x3 supersampling -> жұмсақ шеттер (аралық альфа)
    rows = []
    for y in range(H):
        row = []
        for x in range(W):
            r = g = b = a = 0.0
            for sy in range(SS):
                for sx in range(SS):
                    cr, cg, cb, ca = sample(x + (sx + 0.5) / SS, y + (sy + 0.5) / SS)
                    r += cr * ca; g += cg * ca; b += cb * ca; a += ca
            if a > 0:
                r, g, b = r / a, g / a, b / a   # straight (premultiplied емес) түс
            else:
                r = g = b = 1.0                 # мөлдір пиксельдің түсі ақ: сүзгіде қара жиек шықпасын
            a /= SS * SS
            row += [int(r * 255 + .5), int(g * 255 + .5), int(b * 255 + .5), int(a * 255 + .5)]
        rows.append(row)
    return rows


if __name__ == "__main__":
    out = sys.argv[1] if len(sys.argv) > 1 else "assets"
    png(out + "/pattern.png", W, H, pattern(), 2, 3)
    rows = logo()
    png(out + "/logo.png", W, H, rows, 6, 4)
    mid = sum(1 for r in rows for i in range(3, len(r), 4) if 0 < r[i] < 255)
    print("аралық альфасы бар пиксель саны:", mid)

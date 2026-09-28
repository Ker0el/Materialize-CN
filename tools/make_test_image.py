# -*- coding: utf-8 -*-
"""Write a small test image (stdlib only, no Pillow) so the map tools in
Materialize can actually be opened and their dialogs inspected."""
import os, struct, zlib

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, '..', 'test')
W = H = 256


def png(path, w, h, rows):
    raw = b''.join(b'\x00' + r for r in rows)

    def chunk(tag, data):
        c = tag + data
        return struct.pack('>I', len(data)) + c + struct.pack('>I', zlib.crc32(c) & 0xffffffff)

    hdr = struct.pack('>IIBBBBB', w, h, 8, 0, 0, 0, 0)   # 8-bit greyscale
    body = (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', hdr)
            + chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b''))
    open(path, 'wb').write(body)


def main():
    os.makedirs(OUT, exist_ok=True)
    # height-like: smooth radial gradient + a few blobs
    rows = []
    for y in range(H):
        row = bytearray()
        for x in range(W):
            dx, dy = x - W / 2.0, y - H / 2.0
            v = 255.0 * (1.0 - min(1.0, (dx * dx + dy * dy) ** 0.5 / (W * 0.5)))
            v += 40.0 * ((x // 16 + y // 16) % 2)
            row.append(max(0, min(255, int(v))))
        rows.append(bytes(row))
    png(os.path.join(OUT, 'height.png'), W, H, rows)

    # diffuse-like: smooth colour blobs
    rows = []
    for y in range(H):
        row = bytearray()
        for x in range(W):
            r = int(127 + 120 * ((x / W - 0.5) * 2))
            g = int(127 + 120 * ((y / H - 0.5) * 2))
            b = int(127 + 100 * (((x + y) / (W + H) - 0.5) * 2))
            row += bytes((max(0, min(255, r)), max(0, min(255, g)), max(0, min(255, b))))
        rows.append(bytes(row))
    raw = b''.join(b'\x00' + r for r in rows)

    def chunk(tag, data):
        c = tag + data
        return struct.pack('>I', len(data)) + c + struct.pack('>I', zlib.crc32(c) & 0xffffffff)

    hdr = struct.pack('>IIBBBBB', W, H, 8, 2, 0, 0, 0)   # 8-bit RGB
    body = (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', hdr)
            + chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b''))
    open(os.path.join(OUT, 'diffuse.png'), 'wb').write(body)

    for f in ('height.png', 'diffuse.png'):
        p = os.path.join(OUT, f)
        print('%s  %d bytes' % (p, os.path.getsize(p)))


if __name__ == '__main__':
    main()

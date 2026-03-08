#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""
convert_texture.py - Convert PNG/BMP/JPEG to KoiloTexture (.ktex) format.

KoiloTexture binary format:
    Header (16 bytes):
        [0-3]   Magic: "KTEX"
        [4-7]   Width: uint32
        [8-11]  Height: uint32
        [12]    Channels: uint8 (3=RGB, 4=RGBA)
        [13-15] Reserved (zeros)
    Pixel data: width * height * channels bytes (row-major, top-to-bottom)

Usage:
    python convert_texture.py image.png -o texture.ktex
    python convert_texture.py image.png -o texture.ktex --resize 256 256
"""

import struct
import sys
import os
import argparse

def load_image(filepath):
    """Load image using Pillow, return (width, height, channels, bytes)."""
    from PIL import Image
    img = Image.open(filepath)
    if img.mode == 'RGBA':
        channels = 4
    elif img.mode == 'RGB':
        channels = 3
    else:
        img = img.convert('RGB')
        channels = 3
    return img.size[0], img.size[1], channels, img.tobytes(), img

def write_ktex(filepath, width, height, channels, pixel_data):
    """Write .ktex binary file."""
    with open(filepath, 'wb') as f:
        f.write(b'KTEX')
        f.write(struct.pack('II', width, height))
        f.write(struct.pack('B', channels))
        f.write(b'\x00' * 3)  # reserved
        f.write(pixel_data)

    size = os.path.getsize(filepath)
    print(f"Wrote {filepath}")
    print(f"  {width}x{height}, {channels} channels")
    print(f"  {size} bytes ({size/1024:.1f} KB)")

def main():
    parser = argparse.ArgumentParser(description='Convert images to KoiloTexture (.ktex)')
    parser.add_argument('input', help='Input image file (PNG, BMP, JPEG)')
    parser.add_argument('-o', '--output', help='Output .ktex file')
    parser.add_argument('--resize', nargs=2, type=int, metavar=('W', 'H'),
                        help='Resize to WxH pixels')
    parser.add_argument('--force-rgb', action='store_true',
                        help='Force RGB (drop alpha channel)')
    args = parser.parse_args()

    if not os.path.exists(args.input):
        print(f"Error: Input file not found: {args.input}")
        sys.exit(1)

    output = args.output or os.path.splitext(args.input)[0] + '.ktex'

    width, height, channels, data, img = load_image(args.input)

    if args.resize:
        from PIL import Image
        img = img.resize((args.resize[0], args.resize[1]), Image.LANCZOS)
        width, height = img.size
        data = img.tobytes()

    if args.force_rgb and channels == 4:
        from PIL import Image
        img = img.convert('RGB')
        channels = 3
        data = img.tobytes()

    write_ktex(output, width, height, channels, data)

if __name__ == '__main__':
    main()

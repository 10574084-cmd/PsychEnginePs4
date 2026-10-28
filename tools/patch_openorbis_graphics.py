#!/usr/bin/env python3
"""Patch the OpenOrbis sample Scene2D with a clipped, alpha-aware fast RGBA blitter."""
from __future__ import annotations
import argparse
from pathlib import Path

DECLARATION = "void DrawPixel(int x, int y, Color color);"
NEW_DECLARATION = DECLARATION + "\n\tvoid DrawRGBAImage(int startX, int startY, int imgWidth, int imgHeight, const uint8_t *rgba);"
ANCHOR = "void Scene2D::DrawRectangle(int x, int y, int w, int h, Color color)"
IMPLEMENTATION = r'''
void Scene2D::DrawRGBAImage(int startX, int startY, int imgWidth, int imgHeight, const uint8_t *rgba)
{
    if (rgba == NULL || imgWidth <= 0 || imgHeight <= 0)
        return;

    const int xBegin = startX < 0 ? 0 : startX;
    const int yBegin = startY < 0 ? 0 : startY;
    const int xEnd = startX + imgWidth > this->width ? this->width : startX + imgWidth;
    const int yEnd = startY + imgHeight > this->height ? this->height : startY + imgHeight;
    if (xBegin >= xEnd || yBegin >= yEnd)
        return;

    uint32_t *frame = (uint32_t *)this->frameBuffers[this->activeFrameBufferIdx];
    for (int y = yBegin; y < yEnd; ++y)
    {
        const uint8_t *src = rgba + (((y - startY) * imgWidth) + (xBegin - startX)) * 4;
        uint32_t *dst = frame + y * this->width + xBegin;
        for (int x = xBegin; x < xEnd; ++x, src += 4, ++dst)
        {
            const uint32_t alpha = src[3];
            if (alpha == 0)
                continue;

            uint32_t r = src[0];
            uint32_t g = src[1];
            uint32_t b = src[2];
            if (alpha < 255)
            {
                const uint32_t old = *dst;
                const uint32_t dr = (old >> 16) & 0xFF;
                const uint32_t dg = (old >> 8) & 0xFF;
                const uint32_t db = old & 0xFF;
                const uint32_t inv = 255 - alpha;
                r = (r * alpha + dr * inv + 127) / 255;
                g = (g * alpha + dg * inv + 127) / 255;
                b = (b * alpha + db * inv + 127) / 255;
            }
            *dst = 0x80000000u | (r << 16) | (g << 8) | b;
        }
    }
}
'''

def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("header", type=Path)
    parser.add_argument("source", type=Path)
    args = parser.parse_args()
    header = args.header.read_text(encoding="utf-8")
    source = args.source.read_text(encoding="utf-8")

    if "DrawRGBAImage(" not in header:
        if header.count(DECLARATION) != 1:
            raise RuntimeError(f"Could not find unique declaration anchor in {args.header}")
        header = header.replace(DECLARATION, NEW_DECLARATION, 1)
        args.header.write_text(header, encoding="utf-8")

    if "Scene2D::DrawRGBAImage(" not in source:
        if source.count(ANCHOR) != 1:
            raise RuntimeError(f"Could not find unique implementation anchor in {args.source}")
        source = source.replace(ANCHOR, IMPLEMENTATION + "\n" + ANCHOR, 1)
        args.source.write_text(source, encoding="utf-8")

    if "Scene2D::DrawRGBAImage(" not in args.source.read_text(encoding="utf-8"):
        raise RuntimeError("DrawRGBAImage patch verification failed")
    print("Patched OpenOrbis Scene2D with alpha-aware RGBA blitting.")

if __name__ == "__main__":
    main()

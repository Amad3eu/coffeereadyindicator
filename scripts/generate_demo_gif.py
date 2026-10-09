#!/usr/bin/env python3
"""Generate docs/images/coffee-flow.gif: a minimal, technical animation of how
Coffee Sentinel works.

Frames
------
1. Coffee machine brewing, LED off
2. LED turns green (coffee ready)
3. Phototransistor detects the light
4. ESP32 confirms (DETECTING -> READY)
5. Buzzer beeps
6. Webhook -> Alexa: "Coffee is ready!"

Only Pillow (>= 9.0) is required:

    python3 -m pip install pillow
    python3 scripts/generate_demo_gif.py            # writes docs/images/coffee-flow.gif
    python3 scripts/generate_demo_gif.py --frames   # also writes the individual PNG frames

The output is byte-for-byte reproducible for a given Pillow version; different
Pillow releases encode the same frames slightly differently.
"""

from __future__ import annotations

import argparse
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parent.parent
OUTPUT = ROOT / "docs" / "images" / "coffee-flow.gif"
FRAMES_DIR = ROOT / "docs" / "images" / "frames"

# Pillow >= 9.1 exposes these as enums; older releases use module-level constants.
QUANTIZE_MEDIANCUT = Image.Quantize.MEDIANCUT if hasattr(Image, "Quantize") else Image.MEDIANCUT
DITHER_NONE = Image.Dither.NONE if hasattr(Image, "Dither") else Image.NONE

WIDTH, HEIGHT = 720, 360
FRAME_MS = 1400
LAST_FRAME_MS = 2600

# Palette (muted, readable on light and dark GitHub themes since the GIF has its own background)
BG = (250, 249, 246)
INK = (40, 40, 44)
MUTED = (150, 150, 156)
LINE = (190, 190, 196)
ACCENT = (28, 158, 90)       # green LED
ACCENT_SOFT = (190, 236, 208)
BLUE = (52, 110, 190)
BLUE_SOFT = (205, 222, 245)
ORANGE = (230, 140, 40)
ORANGE_SOFT = (250, 225, 190)
OFF = (120, 120, 126)


def font(size: int, bold: bool = False) -> ImageFont.FreeTypeFont | ImageFont.ImageFont:
    names = ["DejaVuSans-Bold.ttf" if bold else "DejaVuSans.ttf", "Arial.ttf", "LiberationSans-Regular.ttf"]
    for name in names:
        try:
            return ImageFont.truetype(name, size)
        except OSError:
            continue
    try:
        return ImageFont.load_default(size=size)  # Pillow >= 10.1 ships a scalable default font
    except TypeError:
        return ImageFont.load_default()


F_TITLE = font(22, bold=True)
F_LABEL = font(15)
F_SMALL = font(12)
F_CAPTION = font(16, bold=True)


class Scene:
    """Draws the fixed layout; `state` selects what is highlighted."""

    def __init__(self, step: int) -> None:
        self.step = step
        self.image = Image.new("RGB", (WIDTH, HEIGHT), BG)
        self.draw = ImageDraw.Draw(self.image)

    # -- helpers ----------------------------------------------------------
    def text_center(self, xy: tuple[int, int], text: str, fnt, fill=INK) -> None:
        x, y = xy
        bbox = self.draw.textbbox((0, 0), text, font=fnt)
        w, h = bbox[2] - bbox[0], bbox[3] - bbox[1]
        self.draw.text((x - w / 2, y - h / 2), text, font=fnt, fill=fill)

    def box(self, x: int, y: int, w: int, h: int, label: str, active: bool, color=BLUE, soft=BLUE_SOFT) -> None:
        self.draw.rounded_rectangle((x, y, x + w, y + h), radius=10,
                                    fill=soft if active else BG, outline=color if active else LINE, width=3 if active else 2)
        self.text_center((x + w // 2, y + h // 2), label, F_LABEL, INK if active else MUTED)

    def arrow(self, start: tuple[int, int], end: tuple[int, int], active: bool) -> None:
        color = INK if active else LINE
        self.draw.line([start, end], fill=color, width=3 if active else 2)
        x, y = end
        dx = 1 if end[0] >= start[0] else -1
        if start[1] == end[1]:  # horizontal
            self.draw.polygon([(x, y), (x - 10 * dx, y - 6), (x - 10 * dx, y + 6)], fill=color)
        else:  # vertical (downwards)
            self.draw.polygon([(x, y), (x - 6, y - 10), (x + 6, y - 10)], fill=color)

    # -- elements ---------------------------------------------------------
    def coffee_machine(self, led_on: bool) -> None:
        x, y = 40, 90
        self.draw.rounded_rectangle((x, y, x + 110, y + 150), radius=12, fill=BG, outline=INK, width=3)
        self.draw.rectangle((x + 20, y + 20, x + 90, y + 55), fill=(230, 230, 232), outline=INK, width=2)
        # carafe
        self.draw.rounded_rectangle((x + 28, y + 75, x + 82, y + 135), radius=8, fill=(110, 70, 40), outline=INK, width=2)
        # LED
        cx, cy = x + 95, y + 12
        if led_on:
            self.draw.ellipse((cx - 16, cy - 16, cx + 16, cy + 16), fill=ACCENT_SOFT)
            self.draw.ellipse((cx - 7, cy - 7, cx + 7, cy + 7), fill=ACCENT, outline=INK)
        else:
            self.draw.ellipse((cx - 7, cy - 7, cx + 7, cy + 7), fill=OFF, outline=INK)
        self.text_center((x + 55, y + 170), "coffee machine", F_SMALL, MUTED)
        self.text_center((cx + 2, cy - 26), "ready LED", F_SMALL, ACCENT if led_on else MUTED)

    def sensor(self, active: bool) -> None:
        x, y = 190, 95
        self.draw.rounded_rectangle((x, y, x + 34, y + 20), radius=5,
                                    fill=ORANGE_SOFT if active else BG, outline=ORANGE if active else LINE, width=3 if active else 2)
        self.text_center((x + 17, y + 36), "TEPT4400", F_SMALL, INK if active else MUTED)
        if active:  # light rays from the LED
            for dy in (-10, 0, 10):
                self.draw.line([(153, 102 + dy // 2), (185, 105 + dy)], fill=ACCENT, width=2)

    def esp32(self, active: bool, state: str) -> None:
        self.box(270, 80, 150, 70, "ESP32", active)
        self.text_center((345, 165), f"state: {state}", F_SMALL, INK if active else MUTED)

    def buzzer(self, active: bool) -> None:
        cx, cy = 520, 70
        self.draw.ellipse((cx - 22, cy - 22, cx + 22, cy + 22), fill=ORANGE_SOFT if active else BG,
                          outline=ORANGE if active else LINE, width=3 if active else 2)
        self.draw.ellipse((cx - 5, cy - 5, cx + 5, cy + 5), fill=INK if active else MUTED)
        if active:
            for r in (32, 42):
                self.draw.arc((cx - r, cy - r, cx + r, cy + r), start=-40, end=40, fill=ORANGE, width=3)
        self.text_center((cx, cy + 38), "buzzer", F_SMALL, INK if active else MUTED)

    def alexa(self, active: bool) -> None:
        cx, cy = 520, 190
        self.draw.rounded_rectangle((cx - 24, cy - 30, cx + 24, cy + 30), radius=12,
                                    fill=BLUE_SOFT if active else BG, outline=BLUE if active else LINE, width=3 if active else 2)
        self.draw.ellipse((cx - 18, cy - 36, cx + 18, cy - 24), fill=BLUE if active else LINE)
        self.text_center((cx, cy + 46), "Alexa", F_SMALL, INK if active else MUTED)
        self.text_center((cx, cy - 56), "Wi-Fi → webhook →", F_SMALL, INK if active else MUTED)
        if active:
            bx, by = 585, 170
            self.draw.rounded_rectangle((bx, by - 22, bx + 118, by + 22), radius=10, fill=BG, outline=INK, width=2)
            self.draw.polygon([(bx, by - 4), (bx - 12, by + 2), (bx, by + 8)], fill=BG, outline=INK)
            self.text_center((bx + 59, by - 7), "“Coffee is", F_LABEL, INK)
            self.text_center((bx + 59, by + 9), "ready!”", F_LABEL, INK)

    def caption(self, text: str) -> None:
        self.draw.line([(30, 290), (WIDTH - 30, 290)], fill=LINE, width=1)
        self.text_center((WIDTH // 2, 318), text, F_CAPTION, INK)

    def title(self) -> None:
        self.draw.text((30, 22), "Coffee Sentinel", font=F_TITLE, fill=INK)
        self.draw.text((30, 50), f"step {self.step}/6", font=F_SMALL, fill=MUTED)


FRAMES = [
    # (led_on, sensor, esp, state, buzzer, alexa, caption)
    (False, False, False, "IDLE", False, False, "1. The machine is brewing. Ready LED off."),
    (True, False, False, "IDLE", False, False, "2. Brewing done: the green LED turns on."),
    (True, True, False, "DETECTING", False, False, "3. The phototransistor sees the LED."),
    (True, True, True, "READY", False, False, "4. ESP32 confirms 3 s of steady light."),
    (True, True, True, "READY", True, False, "5. Local alert: BEEP · pause · BEEP."),
    (True, True, True, "NOTIFIED", False, True, "6. Webhook → Alexa: “Coffee is ready!”"),
]


def render_frame(index: int) -> Image.Image:
    led_on, sensor, esp, state, buzzer, alexa, caption = FRAMES[index]
    scene = Scene(index + 1)
    scene.title()
    scene.coffee_machine(led_on)
    scene.arrow((190 + 34 + 6, 105), (266, 105), sensor)       # sensor -> esp32
    scene.sensor(sensor)
    scene.esp32(esp, state)
    scene.draw.line([(420, 100), (470, 100), (470, 70), (494, 70)], fill=INK if buzzer else LINE, width=3 if buzzer else 2)
    scene.draw.line([(470, 100), (470, 190), (494, 190)], fill=INK if alexa else LINE, width=3 if alexa else 2)
    scene.buzzer(buzzer)
    scene.alexa(alexa)
    scene.caption(caption)
    return scene.image.quantize(colors=64, method=QUANTIZE_MEDIANCUT, dither=DITHER_NONE)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--output", type=Path, default=OUTPUT)
    parser.add_argument("--frames", action="store_true", help="also write individual PNG frames")
    args = parser.parse_args()

    frames = [render_frame(i) for i in range(len(FRAMES))]
    durations = [FRAME_MS] * (len(frames) - 1) + [LAST_FRAME_MS]

    args.output.parent.mkdir(parents=True, exist_ok=True)
    frames[0].save(args.output, save_all=True, append_images=frames[1:], duration=durations, loop=0,
                   optimize=True, disposal=2)
    print(f"wrote {args.output.relative_to(ROOT)} ({args.output.stat().st_size // 1024} KiB, {len(frames)} frames)")

    if args.frames:
        FRAMES_DIR.mkdir(parents=True, exist_ok=True)
        for i, frame in enumerate(frames, start=1):
            path = FRAMES_DIR / f"frame-{i}.png"
            frame.convert("RGB").save(path)
            print(f"wrote {path.relative_to(ROOT)}")


if __name__ == "__main__":
    main()

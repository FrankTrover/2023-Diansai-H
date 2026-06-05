from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[1]
FIG = ROOT / "figures"
FIG.mkdir(exist_ok=True)


def font(size: int, bold: bool = False):
    candidates = [
        r"C:\Windows\Fonts\msyhbd.ttc" if bold else r"C:\Windows\Fonts\msyh.ttc",
        r"C:\Windows\Fonts\simhei.ttf",
        r"C:\Windows\Fonts\simsun.ttc",
    ]
    for name in candidates:
        try:
            return ImageFont.truetype(name, size)
        except OSError:
            pass
    return ImageFont.load_default()


F_BIG = font(44)
F_MID = font(34)
F_SMALL = font(28)
F_BLUE = font(42)


def centered(draw, box, text, fnt, fill=(0, 0, 0), spacing=8):
    x1, y1, x2, y2 = box
    lines = text.split("\n")
    heights = []
    widths = []
    for line in lines:
        bb = draw.textbbox((0, 0), line, font=fnt)
        widths.append(bb[2] - bb[0])
        heights.append(bb[3] - bb[1])
    total_h = sum(heights) + spacing * (len(lines) - 1)
    y = y1 + (y2 - y1 - total_h) / 2
    for line, w, h in zip(lines, widths, heights):
        draw.text((x1 + (x2 - x1 - w) / 2, y), line, font=fnt, fill=fill)
        y += h + spacing


def rect(draw, box, text, fnt=F_MID, width=4):
    draw.rectangle(box, outline=(0, 0, 0), width=width)
    centered(draw, box, text, fnt)


def arrow(draw, start, end, width=5, fill=(0, 0, 0), both=False, dash=False):
    if dash:
        x1, y1 = start
        x2, y2 = end
        steps = 18
        for i in range(0, steps, 2):
            a = i / steps
            b = min((i + 1) / steps, 1)
            p1 = (x1 + (x2 - x1) * a, y1 + (y2 - y1) * a)
            p2 = (x1 + (x2 - x1) * b, y1 + (y2 - y1) * b)
            draw.line([p1, p2], fill=fill, width=width)
    else:
        draw.line([start, end], fill=fill, width=width)
    draw.polygon(
        [
            end,
            (end[0] - 22, end[1] - 12) if end[0] >= start[0] else (end[0] + 22, end[1] - 12),
            (end[0] - 22, end[1] + 12) if end[0] >= start[0] else (end[0] + 22, end[1] + 12),
        ],
        fill=fill,
    )
    if both:
        draw.polygon(
            [
                start,
                (start[0] + 22, start[1] - 12) if end[0] >= start[0] else (start[0] - 22, start[1] - 12),
                (start[0] + 22, start[1] + 12) if end[0] >= start[0] else (start[0] - 22, start[1] + 12),
            ],
            fill=fill,
        )


def make_system():
    img = Image.new("RGB", (2520, 1350), "white")
    d = ImageDraw.Draw(img)
    boxes = {
        "src": (40, 470, 250, 640),
        "adder": (410, 470, 630, 640),
        "adc": (780, 470, 1000, 640),
        "mcu": (1160, 465, 1445, 650),
        "lcd": (1175, 170, 1430, 310),
        "dds": (1650, 450, 1888, 670),
        "amp": (2025, 470, 2245, 640),
        "pwr1": (400, 975, 650, 1165),
        "pwr2": (1165, 975, 1425, 1165),
    }
    rect(d, boxes["src"], "信号源\nA, B", F_MID)
    rect(d, boxes["adder"], "TL072\n加法器", F_MID)
    rect(d, boxes["adc"], "AD7616\n采集", F_MID)
    rect(d, boxes["mcu"], "STM32F407\n识别控制", F_MID)
    rect(d, boxes["lcd"], "LCD", F_BIG)
    rect(d, boxes["dds"], "AD9833\n/\nAD9834\n\nA', B'", F_MID)
    rect(d, boxes["amp"], "后级放大\n测试端", F_MID)
    rect(d, boxes["pwr1"], "TPS5430\n±5 V", F_MID)
    rect(d, boxes["pwr2"], "TPS5450\n+5 V", F_MID)

    arrow(d, (250, 555), (410, 555))
    d.text((295, 500), "A, B", font=F_SMALL, fill=(0, 0, 0))
    arrow(d, (630, 555), (780, 555))
    d.text((690, 500), "C", font=F_SMALL, fill=(0, 0, 0))
    arrow(d, (1000, 555), (1160, 555))
    d.text((1010, 503), "并口数据", font=F_SMALL, fill=(0, 0, 0))
    arrow(d, (1302, 465), (1302, 310), both=True)
    arrow(d, (1445, 555), (1650, 555), both=True)
    d.text((1482, 495), "SPI/GPIO", font=F_SMALL, fill=(0, 0, 0))
    arrow(d, (1888, 555), (2025, 555))
    arrow(d, (2245, 555), (2325, 555))
    d.text((2335, 532), "A’, B’", font=F_MID, fill=(0, 0, 0))

    arrow(d, (525, 975), (525, 640), both=True, dash=True, width=3)
    d.text((545, 780), "运放供电", font=F_SMALL, fill=(0, 0, 0))
    arrow(d, (1295, 975), (1295, 650), both=True, dash=True, width=3)
    d.text((1315, 780), "系统供电", font=F_SMALL, fill=(0, 0, 0))

    img.save(FIG / "system_block.png", quality=95)


def make_adder():
    src = Path(r"C:\Users\THTGZ\Downloads\ChatGPT Image 2026年6月1日 19_09_45.png")
    if src.exists():
        img = Image.open(src).convert("RGB")
        img.save(FIG / "adder_tl071.png", quality=95)
    else:
        img = Image.new("RGB", (1800, 750), "white")
        d = ImageDraw.Draw(img)
        d.text((60, 170), "A", font=F_BLUE, fill=(0, 0, 220))
        d.text((60, 520), "B", font=F_BLUE, fill=(0, 0, 220))
        rect(d, (430, 120, 590, 300), "U1\nTL072", F_SMALL, width=3)
        rect(d, (430, 470, 590, 650), "U2\nTL072", F_SMALL, width=3)
        rect(d, (1230, 300, 1420, 500), "U3\nTL072", F_SMALL, width=3)
        arrow(d, (110, 205), (430, 205))
        arrow(d, (110, 555), (430, 555))
        arrow(d, (590, 210), (840, 330))
        arrow(d, (590, 555), (840, 420))
        rect(d, (840, 315, 930, 360), "R1\n510", F_SMALL, width=3)
        rect(d, (840, 405, 930, 450), "R2\n510", F_SMALL, width=3)
        arrow(d, (930, 338), (1230, 380))
        arrow(d, (930, 428), (1230, 420))
        arrow(d, (1420, 400), (1700, 400))
        d.text((1710, 375), "C", font=F_BLUE, fill=(0, 0, 220))
        img.save(FIG / "adder_tl072.png", quality=95)


def ground(draw, x, y, width=90):
    draw.line([(x, y), (x, y + 32)], fill=(0, 0, 0), width=4)
    draw.line([(x - width // 2, y + 32), (x + width // 2, y + 32)], fill=(0, 0, 0), width=4)
    draw.line([(x - width // 3, y + 48), (x + width // 3, y + 48)], fill=(0, 0, 0), width=4)
    draw.line([(x - width // 6, y + 64), (x + width // 6, y + 64)], fill=(0, 0, 0), width=4)


def zigzag(draw, start, end, segments=8, amplitude=16):
    x1, y1 = start
    x2, y2 = end
    pts = [start]
    horizontal = abs(x2 - x1) >= abs(y2 - y1)
    for i in range(1, segments):
        t = i / segments
        if horizontal:
            x = x1 + (x2 - x1) * t
            y = y1 + (amplitude if i % 2 else -amplitude)
        else:
            x = x1 + (amplitude if i % 2 else -amplitude)
            y = y1 + (y2 - y1) * t
        pts.append((x, y))
    pts.append(end)
    draw.line(pts, fill=(0, 0, 0), width=4)


def make_output_amp():
    img = Image.new("RGB", (1800, 1050), "white")
    d = ImageDraw.Draw(img)

    # Input module.
    d.rectangle((45, 330, 250, 575), outline=(0, 0, 0), width=4)
    centered(d, (45, 330, 250, 455), "VOUT", F_MID)
    centered(d, (45, 455, 250, 575), "AGND", F_MID)
    d.line([(250, 380), (470, 380)], fill=(0, 0, 0), width=4)
    d.line([(250, 505), (310, 505), (310, 630)], fill=(0, 0, 0), width=4)
    ground(d, 310, 630)

    # AC coupling capacitor and input bias.
    d.line([(470, 330), (470, 430)], fill=(0, 0, 0), width=4)
    d.line([(495, 330), (495, 430)], fill=(0, 0, 0), width=4)
    d.text((455, 260), "C1", font=F_BIG, fill=(0, 0, 0))
    d.text((430, 455), "0.1μF", font=F_MID, fill=(0, 0, 0))
    d.line([(495, 380), (720, 380)], fill=(0, 0, 0), width=4)
    d.ellipse((712, 372, 728, 388), fill=(0, 0, 0))
    d.line([(720, 380), (720, 585)], fill=(0, 0, 0), width=4)
    zigzag(d, (720, 430), (720, 560))
    d.text((610, 445), "Rin\n100kΩ", font=F_SMALL, fill=(0, 0, 0), spacing=8)
    ground(d, 720, 585)

    # Op amp body.
    tri = [(940, 260), (940, 680), (1240, 470)]
    d.polygon(tri, outline=(0, 0, 0), fill=(255, 255, 255))
    d.line([tri[0], tri[1], tri[2], tri[0]], fill=(0, 0, 0), width=4)
    d.text((1018, 440), "TL072", font=F_MID, fill=(0, 0, 0))
    d.text((910, 362), "+", font=F_BIG, fill=(0, 0, 0))
    d.text((915, 560), "-", font=F_BIG, fill=(0, 0, 0))
    d.text((878, 345), "3", font=F_SMALL, fill=(0, 0, 0))
    d.text((878, 550), "2", font=F_SMALL, fill=(0, 0, 0))
    d.text((1020, 254), "7", font=F_SMALL, fill=(0, 0, 0))
    d.text((1020, 655), "4", font=F_SMALL, fill=(0, 0, 0))
    d.text((1278, 445), "6", font=F_SMALL, fill=(0, 0, 0))

    d.line([(720, 380), (940, 380)], fill=(0, 0, 0), width=4)
    d.line([(1075, 260), (1075, 155)], fill=(0, 0, 0), width=4)
    d.ellipse((1065, 130, 1085, 150), outline=(0, 0, 0), width=4)
    d.text((1040, 78), "+5V", font=F_MID, fill=(0, 0, 0))
    d.line([(1075, 680), (1075, 785)], fill=(0, 0, 0), width=4)
    d.ellipse((1065, 785, 1085, 805), outline=(0, 0, 0), width=4)
    d.text((1040, 825), "-5V", font=F_MID, fill=(0, 0, 0))

    # Gain network and output.
    d.line([(940, 590), (850, 590), (850, 705)], fill=(0, 0, 0), width=4)
    d.ellipse((840, 695, 860, 715), fill=(0, 0, 0))
    d.line([(850, 705), (850, 885)], fill=(0, 0, 0), width=4)
    zigzag(d, (850, 725), (850, 855))
    d.text((895, 765), "Rg\n33kΩ", font=F_SMALL, fill=(0, 0, 0), spacing=8)
    ground(d, 850, 885)

    d.line([(850, 705), (1255, 705)], fill=(0, 0, 0), width=4)
    zigzag(d, (1255, 705), (1365, 705))
    d.text((1280, 600), "Rf\n22kΩ", font=F_SMALL, fill=(0, 0, 0), spacing=8)
    d.line([(1365, 705), (1485, 705), (1485, 470)], fill=(0, 0, 0), width=4)
    d.line([(1240, 470), (1620, 470)], fill=(0, 0, 0), width=4)
    d.ellipse((1475, 460, 1495, 480), fill=(0, 0, 0))
    d.ellipse((1615, 460, 1635, 480), outline=(0, 0, 0), width=4)
    d.text((1660, 445), "Vout", font=F_MID, fill=(0, 0, 0))
    d.line([(1595, 470), (1595, 585)], fill=(0, 0, 0), width=4)
    ground(d, 1595, 585)

    img.save(FIG / "output_amp_tl072.png", quality=95)


if __name__ == "__main__":
    make_system()
    make_adder()
    make_output_amp()

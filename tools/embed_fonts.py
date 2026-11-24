#!/usr/bin/env python3
"""
assets/fonts/*.ttf -> src/fonts_data.c

Shriftlar dastur ichiga joylanadi: .exe va .apk yonida qo'shimcha fayl
kerak bo'lmaydi. Shriftlar oldindan lotin belgilari bilan qisqartirilgan
(pyftsubset), shuning uchun har biri ~15 KB.
"""
import os

HERE = os.path.dirname(os.path.abspath(__file__))
FONTS = [
    ("FONT_UI_MEDIUM",   "Poppins-Medium.ttf"),
    ("FONT_UI_SEMIBOLD", "Poppins-SemiBold.ttf"),
    ("FONT_UI_BOLD",     "Poppins-Bold.ttf"),
    ("FONT_MONO",        "JetBrainsMono-Regular.ttf"),
    ("FONT_MONO_BOLD",   "JetBrainsMono-Bold.ttf"),
]

def main():
    out_c = os.path.join(HERE, "..", "src", "fonts_data.c")
    out_h = os.path.join(HERE, "..", "src", "fonts_data.h")
    with open(out_h, "w") as h:
        h.write("/* AVTOMATIK YARATILGAN - tools/embed_fonts.py. Shriftlar: Poppins, JetBrains Mono (SIL OFL 1.1). */\n")
        h.write("#ifndef FONTS_DATA_H\n#define FONTS_DATA_H\n\n")
        for name, _ in FONTS:
            h.write(f"extern const unsigned char {name}[];\nextern const int {name}_SIZE;\n")
        h.write("\n#endif\n")
    with open(out_c, "w") as c:
        c.write("/* AVTOMATIK YARATILGAN - tools/embed_fonts.py. Shriftlar: Poppins, JetBrains Mono (SIL OFL 1.1). */\n")
        c.write('#include "fonts_data.h"\n\n')
        for name, fn in FONTS:
            data = open(os.path.join(HERE, "..", "assets", "fonts", fn), "rb").read()
            c.write(f"/* {fn} */\nconst int {name}_SIZE = {len(data)};\nconst unsigned char {name}[] = {{\n")
            for i in range(0, len(data), 24):
                c.write("    " + ",".join(str(b) for b in data[i:i + 24]) + ",\n")
            c.write("};\n\n")
    print("src/fonts_data.c yaratildi")

if __name__ == "__main__":
    main()

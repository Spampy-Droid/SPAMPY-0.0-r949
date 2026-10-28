# SPDX-License-Identifier: GPL-3.0-only
# P5 lookup tables from Melodee 1.4.1, Copyright (C) 2026 Kerem Kilic.
import math,sys
from pathlib import Path
FS=44100
CTL=32
def arr(name, ctype, vals, per=12, ram=False):
    """a table in flash (XIP, through the cache); ram: in RAM (.data), for lookups every sample"""
    out = [f"static {'TABLE_RAM ' if ram else 'const '}{ctype} {name}[{len(vals)}] = {{"]
    for i in range(0, len(vals), per):
        out.append("    " + ", ".join(str(v) for v in vals[i:i + per]) + ",")
    out.append("};")
    return out


def main(path):
    L=["#pragma once"]
    fc=[30.0*((16000/30)**(v/127)) for v in range(128)]
    L += [f"#define P5_KEYTRACK_HALF_Q7 {round(128 * 256 * 127 * math.log(2) / (2 * 192 * math.log(fc[-1] / fc[0])))}"]
    # LFO rate, raw 0..127. Measured on a Rev-4 factory demo (rates 74..97 -> 2.4..8.4 Hz,
    # within 2 %): ln Hz = -3.1005 + 0.05387 raw, bent at the ends to the documented .022 / 500 Hz.
    def p5_lfo_hz(v):
        x = -3.1005 + 0.05387 * v
        if v > 100:
            x += (math.log(500) - (-3.1005 + 0.05387 * 127)) * ((v - 100) / 27) ** 2
        if v < 20:
            x -= (-3.1005 - math.log(.022)) * ((20 - v) / 20) ** 2
        return math.exp(x)
    L += arr("P5_LFO_INC", "uint32_t", [int(p5_lfo_hz(v) / FS * 2**32) for v in range(128)], 8)
    # LFO amount response (initial + wheel + pressure), normalized. Rev-4 vibrato depth measured at
    # 18..55 fits 0.0519 * raw^1.737 cents; the full scale (235 cents) lives in the engine.
    L += arr("P5_AMOUNT", "float", [f"{(v / 127) ** 1.737:.9e}f" for v in range(128)])
    # Keep the calibrated times, without quantizing rates to Q24/Q16.
    p5ms = [10000 ** (v / 127) for v in range(128)]
    L += arr("P5_ENV_ATK", "float", [f"{CTL / (m * FS / 1000):.9e}f" for m in p5ms])
    L += arr("P5_ENV_EXP", "float", [f"{1 - math.exp(-CTL / (m / 1000 * FS)):.9e}f" for m in p5ms])
    L += arr("P5_ENV_SSM", "float", [f"{CTL / (3.6 * m * FS / 1000):.9e}f" for m in p5ms])
    # TPT coefficients at the selected oversampling rate. Keep this small
    # RAM table for audio-rate Poly-Mod; feedback division and tanh use fp32.
    for os in (2, 1):
        L += ["#if P5_OVERSAMPLE == 2" if os == 2 else "#else"]
        vals = [math.tan(math.pi * f / (os * FS)) / (1 + math.tan(math.pi * f / (os * FS))) for f in fc]
        g = arr("P5_TPT_G", "float", [f"{v:.9e}f" for v in vals])
        g[0] = g[0].replace(" =", ' __attribute__((section(".p5_tables"))) =')
        L += g
    L += ["#endif"]  # oversampling
    tanh = arr("P5_TANH", "float", [f"{math.tanh(i / 128):.9e}f" for i in range(1025)])
    tanh[0] = tanh[0].replace(" =", ' __attribute__((section(".p5_tables"))) =')
    L += tanh
    exp2 = arr("P5_EXP2", "float", [f"{2 ** (i / 512):.9e}f" for i in range(514)])
    exp2[0] = exp2[0].replace(" =", ' __attribute__((section(".p5_tables"))) =')
    L += exp2

    Path(path).write_text("\n".join(L)+"\n")

if __name__=="__main__":main(sys.argv[1])

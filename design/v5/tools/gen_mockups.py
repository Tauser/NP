#!/usr/bin/env python3
"""Gera os mockups SVG das telas v5 (1024x600, RGB565-friendly).

Fidelidade ao desenho aprovado. Os numeros de grade/cor vem de
core/np_tokens.h; se mudar la, mude aqui -- tools/check.sh compara a
grade entre v5 e firmware, mas nao consegue comparar SVG com C.

Uso:
    python3 tools/gen_mockups.py        # escreve ../mockups/*.svg
"""

import os
from xml.sax.saxutils import escape

W, H = 1024, 600

# ---------------------------------------------------------------- cores
BG        = "#0E1015"
CARD      = "#14171E"
CARD_2    = "#181C24"
HAIR      = "#232833"
TEXT      = "#F2F4F8"
TEXT_2    = "#9BA2B0"
TEXT_3    = "#6A7180"
TEXT_DIM  = "#454B58"
AMBER     = "#E8A83C"
AMBER_DIM = "#3A2E12"
GREEN     = "#3FBF7F"
RED       = "#E2565B"
BLUE      = "#5B8FD6"
ON_AMBER  = "#12140F"

FONT = "Inter, 'Segoe UI', Roboto, Helvetica, sans-serif"


class Svg:
    def __init__(self, bg=BG):
        self.p = []
        self.bg = bg

    # ---- primitivas ----
    def rect(self, x, y, w, h, fill, r=0, opacity=None, stroke=None, sw=1):
        o = f' opacity="{opacity}"' if opacity is not None else ""
        s = f' stroke="{stroke}" stroke-width="{sw}"' if stroke else ""
        self.p.append(f'<rect x="{x:.1f}" y="{y:.1f}" width="{w:.1f}" '
                      f'height="{h:.1f}" rx="{r}" fill="{fill}"{o}{s}/>')

    def text(self, x, y, s, size=16, fill=TEXT, weight=400, anchor="start",
             spacing=None, opacity=None):
        ls = f' letter-spacing="{spacing}"' if spacing else ""
        o = f' opacity="{opacity}"' if opacity is not None else ""
        self.p.append(f'<text x="{x:.1f}" y="{y:.1f}" font-family="{FONT}" '
                      f'font-size="{size}" font-weight="{weight}" fill="{fill}" '
                      f'text-anchor="{anchor}"{ls}{o}>{escape(s)}</text>')

    def circle(self, cx, cy, r, fill=None, stroke=None, sw=2, opacity=None):
        f = f' fill="{fill}"' if fill else ' fill="none"'
        s = f' stroke="{stroke}" stroke-width="{sw}"' if stroke else ""
        o = f' opacity="{opacity}"' if opacity is not None else ""
        self.p.append(f'<circle cx="{cx:.1f}" cy="{cy:.1f}" r="{r:.1f}"{f}{s}{o}/>')

    def path(self, d, stroke=None, fill="none", sw=2, cap="round"):
        s = f' stroke="{stroke}" stroke-width="{sw}" stroke-linecap="{cap}"' if stroke else ""
        self.p.append(f'<path d="{d}" fill="{fill}"{s}/>')

    def line(self, x, y, w, color=HAIR):
        self.rect(x, y, w, 1, color)

    def vline(self, x, y, h, color=HAIR):
        self.rect(x, y, 1, h, color)

    def poly(self, pts, stroke, sw=2, fill="none"):
        d = " ".join(f"{x:.1f},{y:.1f}" for x, y in pts)
        self.p.append(f'<polyline points="{d}" fill="{fill}" stroke="{stroke}" '
                      f'stroke-width="{sw}" stroke-linejoin="round" stroke-linecap="round"/>')

    def render(self):
        body = "\n  ".join(self.p)
        return (f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" '
                f'viewBox="0 0 {W} {H}">\n'
                f'  <rect width="{W}" height="{H}" fill="{self.bg}"/>\n  {body}\n</svg>\n')


# ---------------------------------------------------------------- icones
def icon_menu(s, x, y, c=TEXT_2):
    for i in range(3):
        s.rect(x, y + i * 5, 16, 1.6, c, 1)


def icon_wifi(s, x, y, c=TEXT_2):
    cx, cy = x + 9, y + 11
    for r, o in ((9, 1), (6, 1), (3, 1)):
        s.path(f"M {cx - r} {cy - r * 0.55} A {r} {r} 0 0 1 {cx + r} {cy - r * 0.55}",
               stroke=c, sw=1.6)
    s.circle(cx, cy + 1.5, 1.4, fill=c)


def icon_bell(s, x, y, c=TEXT_2, badge=False):
    cx = x + 8
    s.path(f"M {cx - 6} {y + 12} L {cx - 6} {y + 7} A 6 6 0 0 1 {cx + 6} {y + 7} "
           f"L {cx + 6} {y + 12} L {cx + 7.5} {y + 14} L {cx - 7.5} {y + 14} Z",
           stroke=c, sw=1.4)
    s.path(f"M {cx - 2} {y + 16} A 2 2 0 0 0 {cx + 2} {y + 16}", stroke=c, sw=1.4)
    if badge:
        s.circle(cx + 7, y + 1, 3, fill=AMBER)


def topbar(s, x_right=968, y=34, badge=True):
    icon_bell(s, x_right - 16, y, badge=badge)
    icon_wifi(s, x_right - 62, y)
    icon_menu(s, x_right - 108, y + 4)


def icon_sun(s, cx, cy, r=11, c=AMBER):
    s.circle(cx, cy, r * 0.55, fill=c)
    for i in range(8):
        import math
        a = i * math.pi / 4
        x1, y1 = cx + math.cos(a) * r * 0.85, cy + math.sin(a) * r * 0.85
        x2, y2 = cx + math.cos(a) * r * 1.3, cy + math.sin(a) * r * 1.3
        s.path(f"M {x1:.1f} {y1:.1f} L {x2:.1f} {y2:.1f}", stroke=c, sw=1.8)


def icon_signal(s, x, y, bars=3, c=TEXT_2):
    cx, cy = x + 8, y + 10
    levels = [(8, 1), (5.5, 2), (3, 3)]
    for i, (r, need) in enumerate(levels):
        col = c if bars >= need else TEXT_DIM
        s.path(f"M {cx - r} {cy - r * 0.5} A {r} {r} 0 0 1 {cx + r} {cy - r * 0.5}",
               stroke=col, sw=1.6)
    s.circle(cx, cy + 1, 1.3, fill=c if bars >= 1 else TEXT_DIM)


def tri(s, x, y, up=True, c=GREEN, w=9, h=7):
    if up:
        s.p.append(f'<path d="M {x} {y + h} L {x + w / 2} {y} L {x + w} {y + h} Z" fill="{c}"/>')
    else:
        s.p.append(f'<path d="M {x} {y} L {x + w / 2} {y + h} L {x + w} {y} Z" fill="{c}"/>')


def check(s, x, y, c=GREEN):
    s.path(f"M {x} {y + 5} L {x + 4} {y + 9} L {x + 11} {y}", stroke=c, sw=2.2)


# ---------------------------------------------------------------- blocos
def card(s, x, y, w, h, fill=CARD, r=14, stroke=HAIR):
    s.rect(x, y, w, h, fill, r, stroke=stroke)


def section(s, x, y, label):
    s.text(x, y, label, 12, TEXT_3, spacing=0.3)


def kv(s, x, y, label, value, vsize=17, vcolor=TEXT, lsize=12):
    s.text(x, y, label, lsize, TEXT_3)
    s.text(x, y + 24, value, vsize, vcolor, 700)


def dots(s, active, count=7, y=578):
    wide, small, gap = 22, 6, 7
    total = sum((wide if i == active else small) + (gap if i + 1 < count else 0)
                for i in range(count))
    cx = (W - total) / 2
    for i in range(count):
        w = wide if i == active else small
        s.rect(cx, y, w, 5, AMBER if i == active else "#2A2F3A", 3)
        cx += w + gap


def toggle(s, x, y, on, w=38, h=20):
    s.rect(x, y, w, h, AMBER if on else "#2C313C", h / 2)
    s.circle(x + (w - h / 2 - 2 if on else h / 2 + 2), y + h / 2, h / 2 - 3,
             fill=ON_AMBER if on else TEXT_3)


def slider(s, x, y, w, pct, label=None, value=None):
    if label:
        s.text(x, y - 8, label, 14, TEXT)
        s.text(x + w, y - 8, value, 13, TEXT_2, anchor="end")
    s.rect(x, y + 4, w, 3, "#2C313C", 2)
    s.rect(x, y + 4, w * pct, 3, AMBER, 2)
    s.circle(x + w * pct, y + 5.5, 6, fill=AMBER)


def spark(s, x, y, w, h, vals, color=GREEN, dot_end=True):
    lo, hi = min(vals), max(vals)
    span = (hi - lo) or 1
    pts = [(x + i * w / (len(vals) - 1), y + h - (v - lo) * h / span)
           for i, v in enumerate(vals)]
    s.poly(pts, color, 1.8)
    if dot_end:
        s.circle(pts[-1][0], pts[-1][1], 3, fill=color)


BTC_SERIES = [101398, 101620, 101480, 101950, 101810, 102260, 102090, 102540,
              102380, 102810, 102660, 103080, 102940, 103310, 103180, 103520,
              103400, 103690, 103560, 103800, 103700, 103880, 103760, 103842]


def list_row(s, x, y, w, lead, title, meta, accent, lead_c=None, dim=False):
    s.rect(x, y, 3, 34, accent, 2)
    s.text(x - 74, y + 22, lead, 19, lead_c or (TEXT_DIM if dim else TEXT_2), 600)
    s.text(x + 16, y + 14, title, 15, TEXT_DIM if dim else TEXT, 600)
    s.text(x + 16, y + 31, meta, 12, TEXT_3)


# ================================================================ telas
def home():
    s = Svg()
    topbar(s)

    # --- coluna esquerda ---
    s.text(60, 152, "00:55", 108, TEXT, 300, spacing=-2)
    s.text(64, 210, "Sábado, 25 de julho", 17, TEXT_3)
    s.line(64, 248, 440)

    icon_sun(s, 82, 330)
    s.text(112, 352, "24°", 54, TEXT, 300)
    s.text(216, 336, "São Paulo", 15, TEXT)
    s.text(216, 356, "Parcial nublado", 13, TEXT_3)

    for i, (lab, val) in enumerate([("Vento", "12 km/h"), ("Umidade", "54%"),
                                    ("Sensação", "26°"), ("UV", "5 · mod.")]):
        kv(s, 64 + i * 100, 404, lab, val, 15)

    s.line(64, 462, 440)
    s.text(64, 492, "Dólar", 12, TEXT_3)
    s.text(64, 518, "5,42", 19, TEXT, 700)
    tri(s, 118, 508, False, RED)
    s.text(133, 517, "0,18%", 12, RED)

    s.text(212, 492, "Ibovespa", 12, TEXT_3)
    s.text(212, 518, "138.540", 19, TEXT, 700)
    tri(s, 300, 508, False, RED)
    s.text(315, 517, "0,60%", 12, RED)

    # --- coluna direita: proximo compromisso ---
    RX = 578
    s.text(RX, 78, "A seguir", 12, TEXT_3)
    s.text(RX, 122, "15:30", 36, AMBER, 600)
    s.text(RX, 156, "Reunião — NoiseBot team", 19, TEXT, 600)
    s.text(RX, 180, "Trabalho · 45 min · começa em 1 h 12", 12, TEXT_3)

    # --- card BTC ---
    cx, cy, cw, ch = RX - 12, 208, 410, 330
    card(s, cx, cy, cw, ch, CARD)
    ix = cx + 22
    s.circle(ix + 7, cy + 32, 8, fill=AMBER_DIM)
    s.text(ix + 7, cy + 37, "₿", 12, AMBER, 700, anchor="middle")
    s.text(ix + 24, cy + 37, "Bitcoin", 15, TEXT, 600)
    s.text(ix + 84, cy + 37, "BTC/USD", 12, TEXT_3)
    s.circle(cx + cw - 78, cy + 32, 3.5, fill=GREEN)
    s.text(cx + cw - 68, cy + 36, "ao vivo", 12, GREEN)

    s.text(ix, cy + 96, "103.842", 46, TEXT, 600, spacing=-1)
    s.text(ix + 214, cy + 96, "USD", 13, TEXT_3)

    tri(s, ix + 2, cy + 118, True, GREEN)
    s.text(ix + 18, cy + 128, "2,41%", 22, GREEN, 600)
    s.text(ix + 100, cy + 127, "+2.446", 13, GREEN)
    s.text(ix + 158, cy + 127, "em 24 h", 12, TEXT_3)

    spark(s, ix, cy + 148, cw - 44, 62, BTC_SERIES, GREEN)

    s.text(ix, cy + 232, "Últimos 72 min", 11, TEXT_3)
    s.text(cx + cw - 22, cy + 232, "24 amostras", 11, TEXT_3, anchor="end")
    s.line(ix, cy + 244, cw - 44)

    for i, (lab, val) in enumerate([("Máx 24 h", "104.521"), ("Mín 24 h", "100.841"),
                                    ("Volume", "28,4 bi")]):
        s.text(ix + i * 124, cy + 268, lab, 11, TEXT_3)
        s.text(ix + i * 124, cy + 292, val, 16, TEXT, 700)

    dots(s, 0)
    return s


def agenda():
    s = Svg()
    topbar(s)
    LX, RX = 64, 578

    s.text(LX, 78, "A seguir · em 1 h 12", 12, TEXT_3)
    s.text(LX, 138, "15:30", 46, AMBER, 600)
    s.text(LX, 176, "Reunião — NoiseBot team", 22, TEXT, 600)
    s.text(LX, 200, "Sala virtual · Trabalho · 45 min", 12, TEXT_3)

    section(s, LX, 248, "Hoje")
    items = [("15:30", "Reunião — NoiseBot team", "Trabalho · 45 min", AMBER),
             ("17:00", "Academia", "Pessoal · 1 h", GREEN),
             ("19:30", "Jantar com Marina", "Pessoal · 2 h", GREEN)]
    for i, (t, title, meta, c) in enumerate(items):
        y = 272 + i * 58
        s.text(LX, y + 22, t, 17, TEXT_2, 500)
        s.rect(LX + 74, y, 3, 34, c, 2)
        s.text(LX + 90, y + 14, title, 15, TEXT, 600)
        s.text(LX + 90, y + 31, meta, 12, TEXT_3)

    s.vline(RX - 40, 70, 400)

    s.text(RX, 78, "Resumo de hoje", 12, TEXT_3)
    s.text(RX, 148, "4", 44, TEXT, 600)
    s.text(RX + 84, 148, "1 h", 44, AMBER, 600)
    s.text(RX, 170, "eventos", 12, TEXT_3)
    s.text(RX + 84, 170, "até o próximo", 12, TEXT_3)

    s.text(RX, 216, "2 trabalho · 2 pessoal", 15, TEXT)
    s.line(RX, 244, 310)

    s.text(RX, 274, "Amanhã", 12, TEXT_3)
    s.text(RX, 302, "Nada agendado", 17, TEXT_3)

    s.text(RX, 396, "+", 16, AMBER, 700)
    s.text(RX + 18, 396, "Adicionar evento", 15, AMBER)

    dots(s, 1)
    return s


def market():
    s = Svg()
    topbar(s)
    LX = 64

    tabs = [("BTC/USD", True), ("USD/BRL", False), ("Ibovespa", False)]
    tx = LX
    for name, active in tabs:
        s.text(tx, 100, name, 15, AMBER if active else TEXT_3, 600 if active else 400)
        if active:
            s.rect(tx, 112, 62, 2, AMBER, 1)
        tx += len(name) * 9 + 46
    s.line(LX, 128, 630)

    s.text(LX, 226, "103.842", 88, TEXT, 600, spacing=-3)
    s.text(LX, 254, "dólares por bitcoin", 13, TEXT_2, 600)

    tri(s, LX + 2, 278, True, GREEN, 12, 9)
    s.text(LX + 20, 292, "2,41%", 26, GREEN, 600)
    s.text(LX + 118, 290, "nas últimas 24 horas", 14, TEXT_3)

    s.line(LX, 322, 570)
    for i, (lab, val) in enumerate([("Abertura", "101.398"), ("Máxima", "104.521"),
                                    ("Mínima", "100.841"), ("Volume", "28,4 bi")]):
        s.text(LX + i * 142, 350, lab, 11, TEXT_3)
        s.text(LX + i * 142, 376, val, 20, TEXT, 700)

    RX = 720
    s.vline(RX - 34, 120, 300)
    s.circle(RX + 4, 158, 3.5, fill=GREEN)
    s.text(RX + 14, 162, "ao vivo", 13, GREEN)
    s.text(RX, 192, "CoinGecko", 14, TEXT_2, 600)
    s.text(RX, 210, "atualizado 09:42", 14, TEXT_2, 600)
    s.line(RX, 232, 230)

    s.text(RX, 262, "Dólar", 11, TEXT_3)
    s.text(RX, 288, "5,42", 20, TEXT, 700)
    tri(s, RX + 62, 278, False, RED)
    s.text(RX + 77, 287, "0,18%", 12, RED)

    s.text(RX, 316, "Ibovespa", 11, TEXT_3)
    s.text(RX, 342, "138.540", 20, TEXT, 700)
    tri(s, RX + 102, 332, False, RED)
    s.text(RX + 117, 341, "0,60%", 12, RED)

    dots(s, 2)
    return s


def weather():
    s = Svg()
    topbar(s)
    LX = 64

    s.text(LX, 86, "São Paulo", 12, TEXT_3)
    icon_sun(s, LX + 22, 140, 15)
    s.text(LX, 268, "24°", 92, TEXT, 300, spacing=-2)
    s.text(LX, 300, "Parcial nublado", 17, TEXT, 600)
    s.text(LX, 324, "Máx 27° · Mín 18°", 14, TEXT_2, 600)
    s.text(LX, 344, "Sensação 26°", 14, TEXT_2, 600)
    s.line(LX, 372, 210)
    s.text(LX, 404, "↑ 06:18", 13, TEXT_3)
    s.text(LX + 76, 404, "↓ 18:43", 13, TEXT_3)

    MX = 330
    s.vline(MX - 26, 70, 420)
    s.text(MX, 86, "Próximas horas", 12, TEXT_3)
    hours = [("14 h", "24", "0%", TEXT_3), ("15 h", "25", "0%", TEXT_3),
             ("16 h", "24", "5%", TEXT_3), ("17 h", "22", "20%", BLUE),
             ("18 h", "20", "45%", BLUE), ("19 h", "19", "60%", BLUE)]
    for i, (hh, t, pp, pc) in enumerate(hours):
        x = MX + i * 52
        s.text(x, 112, hh, 11, TEXT_3)
        s.text(x, 140, t + "°", 19, TEXT, 600)
        s.text(x, 160, pp, 11, pc)

    s.line(MX, 186, 300)
    s.text(MX, 216, "Próximos dias", 12, TEXT_3)
    days = [("Sex", "Parc. nublado", "18°", "27°"),
            ("Sáb", "Chuva fraca", "17°", "23°"),
            ("Dom", "Nublado", "16°", "21°"),
            ("Seg", "Sol e nuvens", "15°", "25°")]
    for i, (d, desc, lo, hi) in enumerate(days):
        y = 250 + i * 32
        s.text(MX, y, d, 13, TEXT_3)
        s.text(MX + 44, y, desc, 13, TEXT)
        s.text(MX + 244, y, lo, 13, TEXT_3, anchor="end")
        s.text(MX + 284, y, hi, 13, TEXT, 600, anchor="end")

    RX = 690
    s.vline(RX - 26, 70, 420)
    s.text(RX, 86, "Detalhes", 12, TEXT_3)
    det = [("Vento", "12 km/h NE"), ("Umidade", "54%"),
           ("Índice UV", "5 · moderado"), ("Chuva hoje", "0 mm")]
    for i, (k, v) in enumerate(det):
        y = 122 + i * 40
        s.text(RX, y, k, 14, TEXT_2)
        s.text(RX + 268, y, v, 14, TEXT, 600, anchor="end")
        if i < 3:
            s.line(RX, y + 14, 268)

    s.text(RX, 372, "Open-Meteo · cache", 12, TEXT_2, 600)
    s.text(RX, 390, "atualizado 09:42", 12, TEXT_2, 600)

    dots(s, 3)
    return s


def devices():
    s = Svg()
    topbar(s)
    LX = 64

    section(s, LX, 86, "Cenas")
    scenes = [("Bom dia", "☀"), ("Foco", "◎"),
              ("Cinema", "▭"), ("Boa noite", "☾")]
    # Cenas sao atalhos compostos; dispositivos sao o estado real.
    sw = 216
    for i, (name, ic) in enumerate(scenes):
        x = LX + i * (sw + 14)
        card(s, x, 100, sw, 46, CARD, 10)
        s.text(x + 20, 129, ic, 15, TEXT_2)
        s.text(x + 44, 129, name, 15, TEXT)

    section(s, LX, 186, "Dispositivos")
    devs = [("Luz sala", "Ligado", True, "☀"),
            ("Luz cozinha", "Desligado", False, "☀"),
            ("Luz quarto", "Desligado", False, "☀"),
            ("Tomada TV", "Ligado", True, "▭"),
            ("Cafeteira", "Desligado", False, "☕"),
            ("Ventilador", "Desligado", False, "✳")]
    tw, th = 296, 84
    for i, (name, state, on, ic) in enumerate(devs):
        x = LX + (i % 3) * (tw + 14)
        y = 200 + (i // 3) * (th + 14)
        card(s, x, y, tw, th, CARD_2 if on else CARD, 10,
             stroke=HAIR)
        if on:
            s.rect(x, y, 3, th, AMBER, 2)
        s.text(x + 20, y + 30, ic, 14, AMBER if on else TEXT_3)
        s.text(x + 20, y + 62, name, 16, TEXT if on else TEXT_2, 600)
        s.text(x + tw - 20, y + 62, state, 13, AMBER if on else TEXT_3,
               anchor="end")

    s.line(LX, 396, 896)
    for i, (lab, val) in enumerate([("Temperatura", "22,4°"), ("Umidade", "54%"),
                                    ("CO₂", "480 ppm"), ("Presença", "Detectada")]):
        s.text(LX + i * 132, 424, lab, 11, TEXT_3)
        s.text(LX + i * 132, 448, val, 17, TEXT, 700)

    dots(s, 4)
    return s


def alarms():
    s = Svg()
    topbar(s)
    LX, RX = 64, 578

    s.text(LX, 82, "Próximo alarme · em 6 h 05", 12, TEXT_3)
    s.text(LX, 148, "07:00", 52, AMBER, 600)
    s.text(LX, 182, "Acordar", 20, TEXT, 600)
    s.text(LX, 204, "Segunda a sexta", 13, TEXT_2, 600)

    section(s, LX, 250, "Todos os alarmes")
    al = [("07:00", "Acordar", "Seg-Sex", True),
          ("09:15", "Reunião matinal", "Seg, Qua, Sex", True),
          ("12:30", "Almoço", "Seg-Sex", False),
          ("22:30", "Dormir", "Todos os dias", True)]
    for i, (t, name, rep, on) in enumerate(al):
        y = 268 + i * 52
        s.text(LX, y + 22, t, 22, AMBER if on else TEXT_DIM, 500)
        s.text(LX + 96, y + 16, name, 15, TEXT if on else TEXT_DIM, 600)
        s.text(LX + 96, y + 33, rep, 12, TEXT_3)
        toggle(s, LX + 356, y + 6, on)
        if i < 3:
            s.line(LX, y + 44, 420)

    s.vline(RX - 40, 70, 380)
    s.text(RX, 82, "Novo alarme", 12, TEXT_3)
    card(s, RX, 100, 148, 62, CARD, 10)
    s.text(RX + 16, 124, "Horário", 11, TEXT_3)
    s.text(RX + 16, 148, "07:00", 20, TEXT, 600)
    card(s, RX + 162, 100, 148, 62, CARD, 10)
    s.text(RX + 178, 124, "Repetir", 11, TEXT_3)
    s.text(RX + 178, 148, "Seg-Sex", 18, TEXT, 600)

    s.rect(RX, 182, 310, 48, AMBER, 10)
    s.text(RX + 155, 212, "+  Adicionar alarme", 16, ON_AMBER, 600, anchor="middle")

    dots(s, 5)
    return s


def timer():
    s = Svg()
    topbar(s)
    s.text(W / 2, 232, "25:00", 116, TEXT, 300, anchor="middle", spacing=-3)
    s.text(W / 2, 274, "Pomodoro", 15, TEXT_3, anchor="middle")
    s.rect(282, 310, 460, 2, AMBER, 1)

    presets = [("5 min", False), ("10 min", False), ("25 min", True),
               ("50 min", False), ("Personalizar", False)]
    px = 300
    for name, active in presets:
        s.text(px, 362, name, 15, AMBER if active else TEXT_3, 600 if active else 400)
        if active:
            s.rect(px, 374, 52, 2, AMBER, 1)
        px += len(name) * 8 + 48

    s.rect(392, 402, 190, 52, AMBER, 10)
    s.text(487, 434, "▷  Iniciar", 17, ON_AMBER, 600, anchor="middle")
    card(s, 596, 402, 52, 52, CARD, 10)
    s.text(622, 434, "↺", 20, TEXT_2, anchor="middle")

    dots(s, 6)
    return s


def settings():
    s = Svg(BG)
    topbar(s, badge=False)
    LX = 64

    s.text(LX, 90, "‹", 22, TEXT_2)
    s.circle(LX + 44, 84, 20, fill="#4A3A1A")
    s.text(LX + 44, 90, "RL", 14, AMBER, 700, anchor="middle")
    s.text(LX + 76, 82, "Rafael Lopes", 22, TEXT, 600)
    s.text(LX + 76, 104, "Perfil padrão · NovaPanel ESP32-P4", 12, TEXT_3)
    s.text(920, 90, "Editar", 14, AMBER, anchor="end")
    s.line(LX, 130, 856)

    # --- coluna 1: rede ---
    c1 = LX
    section(s, c1, 162, "Rede")
    icon_wifi(s, c1, 172, GREEN)
    s.text(c1 + 26, 190, "NovaNet 5G", 18, TEXT, 600)
    s.text(c1, 214, "192.168.0.114 · -52 dBm · WPA2", 12, TEXT_3)
    s.text(c1, 240, "Gerenciar redes ›", 13, AMBER)

    s.text(c1, 288, "Bluetooth", 15, TEXT)
    s.text(c1, 308, "Desativado", 12, TEXT_3)
    toggle(s, c1 + 200, 282, False)

    section(s, c1, 356, "Fuso horário")
    s.text(c1, 382, "America/Sao_Paulo", 16, TEXT, 600)
    s.text(c1, 404, "UTC-3 · NTP automático", 12, TEXT_3)
    s.text(c1, 430, "Trocar fuso ›", 13, AMBER)

    # --- coluna 2: tela e som ---
    c2 = 366
    s.vline(c2 - 30, 150, 330)
    section(s, c2, 162, "Tela e som")
    for i, (lab, val, pct) in enumerate([("Brilho", "78%", 0.78),
                                          ("Volume do sistema", "65%", 0.65),
                                          ("Volume da música", "80%", 0.80),
                                          ("Volume do alarme", "90%", 0.90)]):
        slider(s, c2, 196 + i * 46, 250, pct, lab, val)

    section(s, c2, 396, "Tema")
    themes = [("Grafite", True), ("Carvão", False), ("Âmbar", False)]
    tx = c2
    for name, active in themes:
        w = 78
        s.rect(tx, 408, w, 30, CARD_2 if active else CARD, 8,
               stroke=HAIR)
        s.text(tx + w / 2, 428, name, 13, TEXT if active else TEXT_3, anchor="middle")
        tx += w + 8

    s.text(c2, 476, "Modo noturno", 15, TEXT)
    s.text(c2, 496, "22:00 - 06:00", 12, TEXT_3)
    toggle(s, c2 + 224, 470, True)

    # --- coluna 3: sistema ---
    c3 = 690
    s.vline(c3 - 30, 150, 330)
    section(s, c3, 162, "Sistema")
    rows = [("Wi-Fi", "NovaNet 5G · -52 dBm"), ("IP", "192.168.0.114"),
            ("Display", "1024×600 · RGB565"), ("Touch", "Capacitivo · OK"),
            ("Ativo há", "14 d 06 h 22 min"), ("Temperatura", "48 °C"),
            ("Firmware", "NovaOS v1.3")]
    for i, (k, v) in enumerate(rows):
        y = 192 + i * 28
        s.text(c3, y, k, 13, TEXT_3)
        s.text(c3 + 230, y, v, 13, TEXT, 600, anchor="end")

    s.rect(c3, 400, 230, 44, AMBER, 10)
    s.text(c3 + 115, 428, "↻  Atualizar para a v1.4", 14, ON_AMBER, 600,
           anchor="middle")
    s.circle(c3 + 6, 468, 6, stroke=TEXT_2, sw=1.4)
    s.path(f"M {c3 + 6} {462} L {c3 + 6} {468}", stroke=TEXT_2, sw=1.4)
    s.text(c3 + 22, 472, "Reiniciar dispositivo", 13, TEXT_2)
    return s


def _settings_backdrop(s):
    """Fundo esmaecido reaproveitado pelos paineis laterais."""
    LX = 64
    s.text(LX, 90, "‹", 22, TEXT_DIM)
    s.circle(LX + 44, 84, 20, fill="#2A2418")
    s.text(LX + 44, 90, "RL", 14, TEXT_DIM, 700, anchor="middle")
    s.text(LX + 76, 82, "Rafael Lopes", 22, TEXT_DIM, 600)
    s.text(LX + 76, 104, "Perfil padrão · NovaPanel ESP32-P4", 12, TEXT_DIM)
    s.line(LX, 130, 500)
    section(s, LX, 162, "Rede")
    s.text(LX, 190, "NovaNet 5G", 18, TEXT_DIM, 600)
    s.text(LX, 214, "192.168.0.114 · -52 dBm", 12, TEXT_DIM)
    section(s, LX, 356, "Fuso horário")
    s.text(LX, 382, "America/Sao_Paulo", 16, TEXT_DIM, 600)
    section(s, 366, 162, "Tela e som")
    s.text(366, 190, "Brilho 78%", 15, TEXT_DIM)


def sheet_wifi():
    s = Svg()
    _settings_backdrop(s)
    pw = 440
    px = W - pw
    s.rect(0, 0, W, H, "#05070A", opacity="0.66")
    s.rect(px, 0, pw, H, CARD)
    s.vline(px, 0, H)
    pad = 32
    iw = pw - 2 * pad

    s.text(px + pad, 62, "Wi-Fi", 26, TEXT, 600)
    s.text(px + pw - pad, 62, "×", 22, TEXT_2, anchor="end")

    s.text(px + pad, 108, "Conectada", 11, TEXT_3)
    icon_wifi(s, px + pad, 122, GREEN)
    s.text(px + pad + 26, 140, "NovaNet 5G", 17, TEXT, 600)
    s.text(px + pad, 164, "192.168.0.114 · -52 dBm · WPA2", 12, TEXT_3)
    s.text(px + pw - pad, 140, "Desconectar", 13, RED, anchor="end")
    s.line(px + pad, 188, iw)

    s.text(px + pad, 216, "Disponíveis", 11, TEXT_3)
    nets = [("NovaNet 2.4G", "WPA2 · -61 dBm", 3),
            ("Vizinho_5G", "WPA3 · -74 dBm", 2),
            ("NET_8452", "WPA2 · -82 dBm", 1)]
    for i, (ssid, meta, bars) in enumerate(nets):
        y = 236 + i * 50
        icon_signal(s, px + pad, y + 8, bars)
        s.text(px + pad + 30, y + 18, ssid, 15, TEXT, 600)
        s.text(px + pad + 30, y + 35, meta, 12, TEXT_3)
        s.text(px + pw - pad, y + 24, "›", 16, TEXT_3, anchor="end")
        s.line(px + pad, y + 44, iw)

    s.text(px + pad, 412, "+", 15, AMBER, 700)
    s.text(px + pad + 16, 412, "Adicionar rede oculta", 14, AMBER)

    s.rect(px + pad, H - 92, iw, 46, AMBER, 10)
    s.text(px + pad + iw / 2, H - 62, "Concluído", 16, ON_AMBER, 600, anchor="middle")
    return s


def sheet_profile():
    s = Svg()
    _settings_backdrop(s)
    pw = 440
    px = W - pw
    s.rect(0, 0, W, H, "#05070A", opacity="0.66")
    s.rect(px, 0, pw, H, CARD)
    s.vline(px, 0, H)
    pad = 32
    iw = pw - 2 * pad

    s.text(px + pad, 62, "Perfil", 26, TEXT, 600)
    s.text(px + pw - pad, 62, "×", 22, TEXT_2, anchor="end")

    s.circle(px + pad + 26, 132, 26, fill="#4A3A1A")
    s.text(px + pad + 26, 140, "RL", 18, AMBER, 700, anchor="middle")

    s.text(px + pad + 68, 112, "Nome", 11, TEXT_3)
    card(s, px + pad + 68, 122, iw - 68, 40, CARD_2, 8)
    s.text(px + pad + 84, 148, "Rafael Lopes", 15, TEXT)

    s.text(px + pad, 202, "Saudação", 11, TEXT_3)
    opts = [("Automática", True), ("Bom dia", False), ("Olá", False)]
    ox = px + pad
    for name, active in opts:
        w = 108 if name == "Automática" else 86
        s.rect(ox, 214, w, 34, "#4A3A1A" if active else CARD_2, 8)
        s.text(ox + w / 2, 236, name, 13, AMBER if active else TEXT_2, anchor="middle")
        ox += w + 8

    s.text(px + pad, 288, "Idioma", 11, TEXT_3)
    langs = [("Português", True), ("English", False)]
    ox = px + pad
    for name, active in langs:
        w = 128
        s.rect(ox, 300, w, 34, "#3A2E12" if active else CARD_2, 8)
        s.text(ox + w / 2, 322, name, 13, AMBER if active else TEXT_2, anchor="middle")
        ox += w + 8

    s.rect(px + pad, H - 92, 130, 46, CARD_2, 10, stroke=HAIR)
    s.text(px + pad + 65, H - 62, "Cancelar", 15, TEXT_2, anchor="middle")
    s.rect(px + pad + 146, H - 92, iw - 146, 46, AMBER, 10)
    s.text(px + pad + 146 + (iw - 146) / 2, H - 62, "Salvar", 16, ON_AMBER, 600,
           anchor="middle")
    return s


def notifications():
    s = Svg()
    LX = 64
    s.text(LX, 76, "‹", 22, TEXT_2)
    s.text(LX + 30, 78, "Notificações", 26, TEXT, 600)
    s.text(LX + 224, 78, "4", 18, TEXT_3)
    s.text(960, 76, "Marcar todas como lidas", 14, AMBER, anchor="end")
    s.line(LX, 104, 896)

    items = [(AMBER, "Firmware v1.4 disponível", "Pronto para instalar · 2,1 MB", "há 5 min"),
             (GREEN, "Bitcoin acima de US$ 103 mil",
              "Variação de +2,41% nas últimas 24 h", "há 12 min"),
             (TEXT_3, "Sincronização concluída", "Agenda e clima atualizados", "há 32 min"),
             (RED, "Sensor da sala sem resposta", "Última leitura há 1 hora", "há 1 h")]
    for i, (c, title, meta, when) in enumerate(items):
        y = 148 + i * 76
        s.circle(LX + 5, y - 4, 4, fill=c)
        s.text(LX + 22, y, title, 17, TEXT if c != TEXT_3 else TEXT_2, 600)
        s.text(LX + 22, y + 22, meta, 12, TEXT_3)
        s.text(960, y, when, 12, TEXT_3, anchor="end")
        if i < 3:
            s.line(LX, y + 40, 896)

    s.text(LX, 496, "Notificações são apagadas após 24 horas", 12, TEXT_3)
    s.circle(W / 2, 556, 16, fill=CARD_2)
    s.text(W / 2, 562, "↓", 15, TEXT_2, anchor="middle")
    return s


def _setup_head(s, step, title, subtitle=None):
    LX = 72
    s.rect(LX, 58, 9, 9, AMBER, 2)
    s.text(LX + 18, 68, "NovaPanel", 13, TEXT_2, 600)
    s.text(LX, 104, f"Passo {step} de 3", 11, AMBER if step else TEXT_3)
    for i in range(3):
        s.rect(LX + i * 76, 114, 66, 2, AMBER if i < step else "#2C313C", 1)
    s.text(LX, 172, title, 40, TEXT, 500)
    if subtitle:
        s.text(LX, 202, subtitle, 14, TEXT_2, 600)
    return LX


def setup_1():
    s = Svg()
    LX = _setup_head(s, 1, "Escolha a rede Wi-Fi",
                     "O painel funciona sem internet, mas precisa de rede para clima,")
    s.text(LX, 222, "mercado e hora.", 14, TEXT_2, 600)

    nets = [("NovaNet 5G", "WPA2 · -52 dBm", 3, True),
            ("NovaNet 2.4G", "WPA2 · -61 dBm", 3, False),
            ("Vizinho_5G", "WPA3 · -74 dBm", 2, False),
            ("NET_8452", "WPA2 · -82 dBm", 1, False)]
    for i, (ssid, meta, bars, sel) in enumerate(nets):
        y = 262 + i * 48
        if sel:
            s.rect(LX, y, 560, 44, "#1E1A10", 8)
            s.rect(LX, y, 3, 44, AMBER, 2)
        icon_signal(s, LX + 20, y + 12, bars, AMBER if sel else TEXT_2)
        s.text(LX + 52, y + 28, ssid, 16, TEXT, 600 if sel else 400)
        s.text(LX + 540, y + 28, meta, 12, TEXT_3, anchor="end")

    s.text(LX, 486, "↻  Escanear novamente", 13, TEXT_2)

    RX = 690
    s.text(RX, 268, "Rede selecionada", 11, TEXT_3)
    s.text(RX, 296, "NovaNet 5G", 19, TEXT, 600)
    s.text(RX, 322, "Rede protegida.", 13, TEXT_3)
    s.text(RX, 342, "Vamos pedir a senha.", 13, TEXT_3)

    s.rect(RX, 456, 200, 48, AMBER, 10)
    s.text(RX + 100, 486, "Avançar", 16, ON_AMBER, 600, anchor="middle")
    return s


def setup_password():
    s = Svg()
    LX = 72
    s.rect(LX, 52, 9, 9, AMBER, 2)
    s.text(LX + 18, 62, "NovaPanel", 13, TEXT_2, 600)
    s.text(952, 62, "Cancelar", 14, TEXT_2, anchor="end")

    s.text(LX, 122, "Senha de NovaNet 5G", 34, TEXT, 500)

    card(s, LX, 148, 880, 48, CARD_2, 10)
    for i in range(11):
        s.circle(LX + 24 + i * 13, 172, 4, fill=TEXT)
    s.text(LX + 852, 178, "◉", 16, TEXT_2, anchor="end")
    s.text(LX, 216, "A senha fica só no painel, guardada com criptografia.", 13, TEXT_2, 600)

    rows = ["qwertyuiop", "asdfghjkl", "zxcvbnm"]
    kw, kh, gap = 78, 56, 8
    top = 248
    for r, row in enumerate(rows):
        n = len(row)
        row_w = n * kw + (n - 1) * gap
        x0 = (W - row_w) / 2 if r != 2 else (W - row_w) / 2
        if r == 2:
            x0 = (W - (row_w + 2 * (kw + gap))) / 2 + kw + gap
        y = top + r * (kh + gap)
        if r == 2:
            s.rect(x0 - kw - gap, y, kw, kh, CARD_2, 8)
            s.text(x0 - kw - gap + kw / 2, y + 36, "⇧", 16, TEXT_2, anchor="middle")
        for i, ch in enumerate(row):
            x = x0 + i * (kw + gap)
            s.rect(x, y, kw, kh, CARD_2, 8)
            s.text(x + kw / 2, y + 36, ch, 19, TEXT, anchor="middle")
        if r == 2:
            bx = x0 + n * (kw + gap)
            s.rect(bx, y, kw, kh, CARD_2, 8)
            s.text(bx + kw / 2, y + 36, "⌫", 16, TEXT_2, anchor="middle")

    y = top + 3 * (kh + gap)
    s.rect(72, y, 128, kh, CARD_2, 8)
    s.text(136, y + 36, "?123", 15, TEXT_2, anchor="middle")
    s.rect(208, y, 480, kh, CARD_2, 8)
    s.rect(696, y, 256, kh, AMBER, 8)
    s.text(824, y + 36, "Conectar", 17, ON_AMBER, 600, anchor="middle")
    return s


def setup_2():
    s = Svg()
    LX = _setup_head(s, 2, "Fuso horário e formato",
                     "A hora vem do NTP. O fuso só decide como ela aparece.")

    s.text(LX, 288, "Fuso horário", 11, TEXT_3)
    s.text(LX, 326, "America/Sao_Paulo", 30, TEXT, 500)
    s.text(LX, 352, "UTC-3 · detectado pela rede", 13, TEXT_3)
    s.text(LX, 378, "Trocar fuso ›", 13, AMBER)

    s.text(LX, 428, "Formato da hora", 11, TEXT_3)
    for i, (name, active) in enumerate([("24 h", True), ("12 h", False)]):
        x = LX + i * 104
        s.rect(x, 440, 96, 34, "#3A2E12" if active else CARD_2, 8)
        s.text(x + 48, 462, name, 14, AMBER if active else TEXT_3, anchor="middle")

    s.text(LX, 516, "‹ Voltar", 13, TEXT_2)

    RX = 600
    s.vline(RX - 40, 270, 220)
    s.text(RX, 288, "Como vai aparecer", 11, TEXT_3)
    s.text(RX, 366, "00:55", 68, TEXT, 300, spacing=-2)
    s.text(RX, 398, "Sábado, 25 de julho", 15, TEXT_2)

    s.rect(752, 452, 200, 48, AMBER, 10)
    s.text(852, 482, "Avançar", 16, ON_AMBER, 600, anchor="middle")
    return s


def setup_3():
    s = Svg()
    LX = _setup_head(s, 3, "Tudo pronto",
                     "Dá para mudar qualquer uma dessas coisas depois, em")
    s.text(LX, 222, "Configurações.", 14, TEXT_2, 600)

    items = [("Wi-Fi", "NovaNet 5G"), ("Fuso", "America/Sao_Paulo"),
             ("Hora", "24 h · sincronizada")]
    for i, (k, v) in enumerate(items):
        y = 292 + i * 54
        check(s, LX, y - 10)
        s.text(LX + 26, y, k, 15, TEXT_3)
        s.text(LX + 460, y, v, 15, TEXT, 600, anchor="end")
        if i < 2:
            s.line(LX, y + 22, 460)

    RX = 600
    s.vline(RX - 40, 274, 220)
    s.text(RX, 292, "O que já começa a funcionar", 11, TEXT_3)
    feats = ["Clima de São Paulo", "Bitcoin e dólar a cada 3 min",
             "Relógio sincronizado por NTP", "Tudo guardado para funcionar offline"]
    for i, f in enumerate(feats):
        s.text(RX, 324 + i * 30, f, 14, TEXT)

    s.text(LX, 516, "‹ Voltar", 13, TEXT_2)
    s.rect(752, 452, 200, 48, AMBER, 10)
    s.text(852, 482, "Concluir", 16, ON_AMBER, 600, anchor="middle")
    return s


def boot():
    s = Svg()
    LX = 100
    s.rect(LX, 232, 11, 11, AMBER, 2)
    s.text(LX, 330, "Nova", 76, TEXT, 300)
    s.text(LX + 196, 330, "Panel", 76, TEXT_3, 300)

    stages = [("Display", True), ("Armazenamento", True), ("Rede", "active"),
              ("Hora", False), ("Dados", False)]
    sw, gap = 148, 12
    for i, (name, state) in enumerate(stages):
        x = LX + i * (sw + gap)
        if state is True:
            c, tc = AMBER, TEXT_2
        elif state == "active":
            c, tc = AMBER, AMBER
        else:
            c, tc = "#2C313C", TEXT_DIM
        s.rect(x, 380, sw, 2, c, 1)
        s.text(x, 402, name, 12, tc, 600 if state == "active" else 400)

    s.text(LX, 452, "Conectando a NovaNet 5G", 17, TEXT, 600)
    s.text(LX, 540, "NovaOS v1.3 · ESP32-P4 + C6 · 6f:2a:19:c4", 11, TEXT_DIM)
    return s


SCREENS = [
    ("home", home), ("agenda", agenda), ("market", market), ("weather", weather),
    ("devices", devices), ("alarms", alarms), ("timer", timer),
    ("settings", settings), ("sheet_wifi", sheet_wifi),
    ("sheet_profile", sheet_profile), ("notifications", notifications),
    ("setup_1", setup_1), ("setup_password", setup_password),
    ("setup_2", setup_2), ("setup_3", setup_3), ("boot", boot),
]


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    out = os.path.join(here, "..", "mockups")
    os.makedirs(out, exist_ok=True)
    for name, fn in SCREENS:
        with open(os.path.join(out, f"{name}.svg"), "w", encoding="utf-8") as fh:
            fh.write(fn().render())
        print(f"  OK  mockups/{name}.svg")
    print(f"{len(SCREENS)} mockups gerados")


if __name__ == "__main__":
    main()

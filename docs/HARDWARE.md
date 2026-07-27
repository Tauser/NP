# NovaPanel — Hardware

> Fatos validados em bancada, herdados dos baselines anteriores.
> **Este patrimônio não se refaz; se reusa.** Regras derivadas:
> `RESOURCE-BUDGET.md`. Não afirma estado do firmware — ver `STATUS.md`.

## Placa

**Waveshare ESP32-P4-WIFI6-Touch-LCD-7B** — BSP oficial no registry
(`waveshare/esp32_p4_wifi6_touch_lcd_7b`). **Usar o BSP; não reimplementar.**

## Fatos confirmados

```text
SoC        ESP32-P4NRW32 — flash NOR 32 MB externa + PSRAM 32 MB no package
Silício    revisão v1.3 → exige CONFIG_ESP32P4_SELECTS_REV_LESS_V3=y
                          e CONFIG_ESP32P4_REV_MIN_100=y, senão o
                          bootloader rejeita o chip
Display    EK79007, MIPI-DSI 2 lanes, 1024×600, RGB565,
           PHY via LDO canal 3 @ 2500 mV
Touch      GT911 por I2C: SCL=GPIO8, SDA=GPIO7
           LCD_RST=GPIO33, BACKLIGHT=GPIO32
           TOUCH_RST/INT = não conectados (só I2C)
Rádio      ESP32-C6-MINI-1 por SDIO (ESP-Hosted):
           CLK[18] CMD[19] D0[14] D1[15] D2[16] D3[17] Slave_Reset[54]
RTC        SEM chip dedicado — domínio RTC interno do P4 com backup de
           bateria no pino ESP_VBAT
Áudio      ES8311 (codec) + ES7210 (AEC) + microfones onboard
           ES8311 divide o I2C com o GT911
SD         SDMMC 4-bit: CLK=43 CMD=44 D0=39 D1=40 D2=41 D3=42
USB        duas portas Type-C, AMBAS no P4 (mesmo MAC); nenhuma chega ao C6
           · "USB1.1 FS" = USB nativo (JTAG/Serial) → flash e monitor
           · "USB TO UART" = ponte CH343 para o mesmo P4
```

## Avisos que já custaram caro

> **Bateria do RTC é recarregável (ML1220, 3–3,3 V).**
> **NÃO** usar CR1220 comum: não é recarregável e há risco no circuito de
> carga.

### Fim de vida da bateria do RTC — comportamento definido

A ML1220 acaba. Sem comportamento definido, isso vira falha silenciosa: o
painel mostra data errada e ninguém entende por quê. O contrato é:

| Situação | Comportamento obrigatório |
|---|---|
| RTC com hora plausível | usa imediatamente, sem rede (ADR-015) |
| RTC com hora **implausível** (epoch abaixo do limiar) | UI mostra estado **não-sincronizado**; nunca inventa data |
| Sem hora e sem rede | relógio exibe placeholder explícito; o resto do painel segue operando |
| NTP sincroniza depois | hora aparece e o estado sai de não-sincronizado |
| RTC implausível em **todo** boot recente | sinalizar suspeita de bateria na tela de sistema |

O último caso é o que fecha o buraco: perder a hora **uma vez** é acidente;
perder em todo boot é bateria no fim, e o produto deve dizer isso em vez de
deixar o usuário adivinhar. O contador de boots com RTC implausível é
persistido e exposto junto das demais métricas.

> **O P4 não tem rádio.** Toda rede — HTTPS, NTP, LAN futura — passa pelo
> link P4↔C6 via ESP-Hosted/SDIO. Falha ou saturação desse link **é** falha
> de rede do produto; o firmware trata o transporte como recurso
> compartilhado e degradável, nunca como garantido.

> **Firmware do C6 se atualiza por Slave OTA via SDIO** (método "Partition
> OTA" do exemplo `host_performs_slave_ota` do componente `esp_hosted`). O
> header "ESP32-C6 UART Terminal" é plano B **não testado**.

## Gotchas de display validados

- **O EK79007 não gira por MADCTL.** O comando é aceito (`ESP_OK`) e **ignorado**;
  `esp_lcd_panel_swap_xy` retorna `not supported by this panel`. Confirmado por
  dois caminhos em 2026-07-26, e o mesmo sintoma está reportado em
  `espressif/esp-bsp#172`. Em painel DPI/vídeo o controlador só faz *streaming*
  do framebuffer, o que explica o MADCTL não ter efeito.
- **No `esp_lvgl_port`, `avoid_tearing` é incompatível com `sw_rotate`** — ou
  seja, aquele backend obrigava a escolher entre rotação e ausência de tearing.
  **Foi por isso que o backend foi trocado** pelo `esp_lvgl_adapter` (ADR-026),
  que faz as duas coisas no mesmo pipeline. **Configuração vigente:**
  `esp_lvgl_adapter`, modo `TRIPLE_PARTIAL`, 3 framebuffers, rotação 180°.
- **No `esp_lvgl_adapter` 0.5.3, os modos `DOUBLE_*` não funcionam com rotação**
  (inconsistência interna do componente): falham em silêncio e o LVGL entra em
  loop de assert, que aparece como *task watchdog*. Há `static_assert` na board
  impedindo essa combinação.
- **`bsp_display_new_with_handles()` já chama `bsp_display_brightness_init()`.**
  Chamar de novo reconfigura o LEDC e gera `ledc: GPIO 32 is not usable` — aviso
  auto-infligido que já foi confundido com evidência de defeito de backlight.
- **O "flicker" histórico era o backlight (GPIO32)**, não tearing. Antes de
  investigar pipeline gráfico, descarte o backlight
  (`GLITCH-PROTOCOLO.md` §2.2).
- **Render LVGL em modo parcial, nunca FULL.**
- **Backlight só liga depois do primeiro frame** — senão aparece tela
  branca no boot.

## Particionamento

Flash de 32 MB permite A/B desde o primeiro dia, e **reparticionar depois
de haver unidade em campo é proibitivo**. A tabela nasce completa:

```text
nvs, nvs_keys, phy_init, otadata,
ota_0 (app 8 MB), ota_1 (app 8 MB),
storage (LittleFS 10 MB), coredump (1 MB),
c6_ota (1 MB, reserva para o slave-OTA do C6)
```

Motivo dos tamanhos: 8 MB por app dá margem para LVGL, BSP e assets sem
consumir o flash todo; 10 MB de cache sustentam uso offline sem pressionar
os slots OTA; reservar `c6_ota` agora elimina migração destrutiva depois.

## Headers de expansão (fases futuras)

```text
PH2.0 I2C   BH1750 (luz), BME280 (temp/umidade/pressão)
            — compartilham o barramento do GT911: confirmar endereços e
              passar pelo lock semântico da HAL
PH2.0 UART  LD2410C (presença mmWave)
USB-A OTG   2.0 HS — não usado
```

Sensor entra por módulo (provider + service + tela registrada), com custo
declarado no `RESOURCE-BUDGET.md` antes de entrar no roadmap.

## Referências

```text
Wiki        https://docs.waveshare.com/ESP32-P4-WIFI6-Touch-LCD-7B/Development-Environment-Setup-IDF
Exemplos    https://github.com/waveshareteam/ESP32-P4-WIFI6-Touch-LCD-7B
BSP         https://github.com/waveshareteam/Waveshare-ESP32-components/tree/main/bsp/esp32_p4_wifi6_touch_lcd_7b
Esquemático https://files.waveshare.com/wiki/ESP32-P4-WIFI6-Touch-LCD-7B/ESP32-P4-WIFI6-Touch-LCD-7B.pdf
ESP-Hosted  componente espressif/esp_hosted (exemplo host_performs_slave_ota)
```

Em conflito de pinos entre PDF e BSP, **o BSP oficial vence**.

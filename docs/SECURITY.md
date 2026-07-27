# NovaPanel — Segurança

> Não afirma estado — ver `STATUS.md`.

## 1. Modelo de ameaça

Produto pessoal, um usuário, rede doméstica. O que importa, em ordem:

| Risco | Por que importa | Mitigação |
|---|---|---|
| Firmware adulterado | acesso físico ou OTA maliciosa | Secure Boot + assinatura de OTA |
| Extração do flash | credenciais Wi-Fi em claro | criptografia de flash + NVS cifrada |
| Unidade sem caminho de atualização | vira tijolo com bug conhecido | OTA A/B provado **antes** de lacrar |
| Vazamento por log | senha/token em serial | proibição de logar segredo |
| Dado pessoal saindo de casa | agenda, presença, sensores | tudo local; sem backend obrigatório |

**Fora do modelo:** atacante com acesso físico prolongado e equipamento de
laboratório. Não é produto de alta segurança; é produto honesto.

## 2. Segredos

- Credencial de Wi-Fi e token vivem **só em NVS** (cifrada em produção).
- **Nenhuma chave no repositório.** Nem exemplo, nem "temporária".
- **Nunca logar segredo.** Nem mascarado, nem em nível debug.
- Log em produção fica em WARN.

## 3. Cadeia de suprimentos

O firmware puxa BSP, LVGL, ESP-Hosted e drivers de um registry externo.
Sem controle disso, "seguro" é meia verdade: um componente comprometido
entra com privilégio total.

- **`dependencies.lock` é versionado.** Ele carrega versão exata e hash de
  cada componente; é o que torna o build reprodutível.
- **Versão exata no manifesto, nunca `*`.** Declarar `"*"` significa que o
  build de amanhã pode não ser o de hoje. Baseline de referência:

  ```text
  lvgl/lvgl                              9.4.0
  espressif/esp_lvgl_port                2.8.0
  waveshare/esp32_p4_wifi6_touch_lcd_7b  1.0.4
  espressif/esp_hosted                   2.12.11
  espressif/esp_wifi_remote              1.6.2
  espressif/esp_lcd_ek79007              1.0.4
  espressif/esp_lcd_touch_gt911          1.2.0
  joltwallet/littlefs                    1.20.4
  ```

- **Subir versão é mudança revisada**, com o mesmo peso de mudança de
  código: diff lido, motivo declarado, gates rodados. Nunca "atualizei tudo
  para resolver um bug".
- **`managed_components/` não é commitado**; o `.lock` é a fonte da verdade.
- Atualização de componente do caminho gráfico é gatilho de reavaliação do
  glitch (`ROADMAP.md` Onda 0, caminho B).

## 4. Comunicação

- HTTPS com validação de certificado via bundle — **sempre**. Sem
  "ignorar certificado" nem em desenvolvimento.
- **NTP é pré-requisito funcional**: sem hora plausível, a validação de
  certificado falha (`ADR-015`).
- Uma conexão HTTPS por vez, com corpo limitado; resposta acima do teto é
  falha (`RESOURCE-BUDGET.md` §2).
- O firmware não abre porta de escuta no MVP.

## 5. OTA — pré-requisito de qualquer unidade lacrada

Ordem obrigatória, e ela não é negociável:

```text
1. Partições A/B + otadata  (já na primeira onda, para não reparticionar)
2. OTA local com assinatura verificada
3. Health-check pós-boot + rollback automático
4. ≥ 3 ciclos de OTA aplicada E revertida com sucesso
5. SÓ ENTÃO: Secure Boot, criptografia de flash em release, anti-rollback
```

Inverter essa ordem produz unidade lacrada sem caminho de atualização — o
erro que a `ADR-016` existe para impedir.

**Rollback automático** significa: se o novo firmware não confirmar saúde
após o boot, o bootloader volta para o slot anterior sem intervenção.

## 6. Provisionamento

Irreversível. Ver `OPERATIONS.md` §4. Resumo:

- Ensaiar em unidade sacrificável **antes** da primeira unidade real.
- Chaves geradas fora do repo, guardadas fora do repo.
- Coredump **permanece ligado** em produção; a partição é apagada no
  provisionamento.
- Anti-rollback só depois do OTA provado.

## 7. Superfície de ataque que fica de fora

Cada item aqui é uma decisão consciente de **não** expandir superfície:

- Sem servidor obrigatório, sem nuvem para dado pessoal.
- Sem porta de escuta, sem shell remoto, sem telemetria externa.
- Sem microfone ativo no MVP (o hardware tem; o produto não usa).
- Sem multiusuário — não há sessão, credencial de app nem permissão para
  administrar.

Reduzir superfície é mais barato que defendê-la.

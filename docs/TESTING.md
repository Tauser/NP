# NovaPanel — Testes e gates

> Não afirma estado — ver `STATUS.md`.
>
> **Princípio (ADR-012):** gate é o que uma máquina verifica. Critério que
> depende de alguém olhar a tela por dias não é gate. O hardware já acumula
> meses sem falha — **soak não é o teste que falta**; atribuição de causa é.

## 1. Pirâmide

```text
        bancada roteirizada   ← curta, com números registrados no PR
      ┌──────────────────────┐
      │  build no alvo       │  idf.py build
    ┌─┴──────────────────────┴─┐
    │  fixtures de provider    │  payload real + malformado + truncado
  ┌─┴──────────────────────────┴─┐
  │  testes de host (core, VM)   │  g++ + shims, roda em CI Linux
┌─┴──────────────────────────────┴─┐
│  gates estáticos                 │  camadas, tamanho, regras de UI
└──────────────────────────────────┘
```

Quanto mais embaixo, mais rápido e mais barato. Se algo **só** pode ser
testado com a placa na mão, provavelmente está na camada errada
(`ARCHITECTURE.md` §13).

## 2. Gates estáticos

Rodam em segundos, em qualquer máquina, e falham o build.

| Gate | Verifica |
|---|---|
| `arch_check.sh` | direção de dependência entre camadas (lê os `REQUIRES` do build) e includes proibidos |
| `size_check.sh` | limites de arquivo, função, parâmetros e aninhamento (`ARCHITECTURE.md` §11) |
| `ui_check.sh` | regras R1/R2/R4/R5/R10 e `static std::function` (`UI-LAYOUT-SYSTEM.md` §6) |
| `font_budget.sh` | soma dos símbolos de fonte no `.map` contra o teto |
| `hygiene.sh` | nada de `build/`, `sdkconfig` gerado, temporário ou segredo no commit |

Esses gates existem porque **cada regra que eles checam já regrediu em
silêncio** em um baseline anterior.

## 3. Testes de host

`host_check.sh --app --tests` compila com `-Wall -Wextra -Werror` e roda o
binário nativo. Cobertura obrigatória:

- **`core/`**: fila com overflow contado, coalescing do despachante,
  orquestrador (intervalo, rate limit, breaker abrindo/fechando), estado.
- **`models/`**: structs puros, migração de schema.
- **Lógica pura de service**: política de retry/boot, decisão de cache,
  seleção de fetcher — via `MockBoard` e providers falsos.
- **View-model de cada tela**: os três estados — **válido, stale, ausente**.
- **Parsing de provider**: fixtures reais **e** malformadas/truncadas.

Regra: **toda correção de bug entra com o teste que a teria pego.** Sem
exceção; é o que impede a mesma classe de defeito de voltar.

## 4. Fault injection (sem hardware)

O worker de rede expõe uma unidade de execução chamável do host, para
simular falha sem FreeRTOS nem HTTP real. Cenários obrigatórios:

| Cenário | Comportamento esperado |
|---|---|
| Wi-Fi ausente | UI operável; dado com `stale`; sem travar |
| DNS falho / API 500 | breaker abre, backoff, half-open, recupera |
| Payload truncado | request **falha** e conta no breaker (nunca aceita meio dado) |
| Payload malformado | parse falha sem corromper estado anterior |
| Cache corrompido | descarte silencioso; segue com dado ausente |
| Versão futura de schema | ignora persistido; **não bricka** |
| Fila de intenções cheia | overflow logado e contado; sem descarte mudo |

## 5. Bancada roteirizada

Curta, com roteiro escrito e números registrados no PR. Nunca "olhei e
pareceu bom".

### 5.1 Roteiro de render (o que importa neste produto)

| Medida | Alvo |
|---|---|
| Flushes por `update()`, por evento | ≤ 4 |
| Duração de `update()` | ≤ 16 ms (p95) |
| Espera de flush/PPA | registrada, separada do tempo de render |
| Boot até primeiro frame | ≤ 2 s |
| Boot até tela inicial | ≤ 6 s |
| Toque até retorno visível | ≤ 100 ms |
| Heap interno livre em regime | > 80 KB |

### 5.2 Roteiro de artefato visual

O método de correlação (carimbo na tela + vídeo + log) está em
`GLITCH-PROTOCOLO.md` §3.1. Ele produz **frame, área e evento** — não
"apareceu em algum momento".

## 6. Instrumentação permanente

Fica no produto, exposta na tela de sistema e persistida: flushes e duração
por evento, espera de flush, watermarks de heap, erases de flash por hora,
reboots, aberturas de breaker, overflows de fila.

Consequência: **regressão de render vira falha de gate**, não descoberta em
bancada meses depois.

## 7. Definition of Done

1. Gates estáticos verdes.
2. `host_check.sh --app --tests` verde.
3. `idf.py build` verde.
4. Bug corrigido tem teste que o pegaria.
5. Mudança em provider tem fixture.
6. Mudança de custo tem número medido no PR.
7. `STATUS.md` atualizado se o estado mudou.
8. **O que não foi verificado está declarado como não verificado.**

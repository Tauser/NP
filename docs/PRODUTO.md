# NovaPanel — Produto

> Escopo e não-escopo. Não afirma estado — ver `STATUS.md`.

## 1. O que é

Um display de parede pessoal que responde, de relance e sem toque, a três
perguntas:

1. **Que horas e que dia são?**
2. **Como está o tempo agora?**
3. **Como estão meus números?** (BTC, dólar)

E que, ao toque, vira central de consulta e controle da casa.

A promessa central é **calma**: informação legível a dois metros, sem
animação disputando atenção, sem tela de erro genérica, sem esperar
carregamento.

## 2. Princípios de produto

| Princípio | Consequência prática |
|---|---|
| **Offline-first de verdade** | Sem rede, o painel continua útil: mostra o último dado com marca de cache. Rede é melhoria, não requisito. |
| **Honestidade de dado** | Todo valor carrega origem (`ao vivo`, `cache`, `indisponível`) **no ponto de uso**. Nunca inventar, nunca mostrar valor velho como atual. |
| **Um foco por cena** | Uma leitura dominante e no máximo duas de apoio. Parede de cards pequenos é anti-produto. |
| **Silêncio é o estado normal** | Notificação é exceção. O painel não pede atenção sem motivo. |
| **Conteúdo antes de moldura** | Separação vem de alinhamento, escala e espaço vazio — não de uma borda em volta de cada número. |
| **Controle por intenção** | Configuração é tarefa nomeada ("Wi-Fi e rede"), não matriz de switches. |
| **Degradação clara** | Falha vira estado explicado em uma linha, no lugar do dado. Nunca travamento, nunca tela em branco. |

## 3. Não-escopo (decisões conscientes)

Nada abaixo entra sem ADR novo:

- **Servidor obrigatório.** O firmware nunca depende de backend próprio.
- **Nuvem para dado pessoal.** Agenda, presença e sensores ficam locais.
- **Voz, câmera, reconhecimento.** Fora do produto atual.
- **Gráfico histórico pesado / candles.** Um indicador de direção resolve;
  série histórica custa parse, cache e render sem valor proporcional.
- **i18n completa.** Single-locale pt-BR/Brasil por escolha explícita, com
  strings em tabela única para que adotar i18n depois seja barato.
- **Animação contínua.** Custo de banda permanente; o produto é calmo.

## 4. Regra de ouro da interface

> **Não se desenha interface para dado que não existe.**

Se não há provider, service e campo de estado aprovados, a área não é
desenhada — nem como valor fixo, nem como "exemplo". Espaço vazio honesto é
preferível a moldura mentindo sobre capacidade.

Mockups exploratórios podem mostrar o produto completo (serve para
dimensionar trabalho); firmware, não.

## 5. Qualidade percebida — o que "premium" significa

Premium aqui não é quantidade de telas. É previsibilidade:

| Atributo | Como o usuário percebe |
|---|---|
| Liga rápido | Informação útil em segundos, não splash longo |
| Nunca pisca | Zero artefato visual em uso normal |
| Responde na hora | Toque tem retorno imediato, sempre |
| Continua útil | Cai a rede e o painel segue servindo |
| Atualiza sem medo | Update com volta automática se algo der errado |
| Conta a verdade | Estado do sistema visível e legível quando algo falha |

As metas numéricas correspondentes estão em `ROADMAP.md` §1.

## 6. Público e contexto de uso

Um usuário, uma casa, uma parede. Sem multiusuário, sem perfis
concorrentes, sem administração remota. Isso simplifica segurança,
persistência e UX — e é premissa, não limitação temporária.

# Escopo atualizado

Entrega principal definida pelo usuário: Windows x64, VST3.
AU/macOS não fazem parte da entrega atual. Standalone é opcional.

# PROMPT PARA O CODEX — Suíte "Tonal Balance Control" (inspirada no iZotope TBC3)

## 0. PAPEL E OBJETIVO
Você é um engenheiro de DSP/áudio sênior (C++20 + JUCE 8) com mentalidade de engenheiro de mixagem.
Construa uma suíte de plugins de **balanço tonal** inspirada no iZotope Tonal Balance Control 3 (TBC3):
analisa o áudio como espectro, compara com **alvos (targets)** de referência e permite correção com um EQ híbrido embutido.

Produtos (mesmo código-base, dois plugins + standalone de cada):
1. **TBC Track** — para uso por canal/instrumento/vocal. Possui seletor de **Role** (papel do instrumento) que carrega alvos e faixas de tolerância específicos.
2. **TBC Master** — para a música inteira (master bus), com Extra Meters completos.

Formatos: VST3, AU, AAX (opcional/flag de build) e Standalone (Mac + Windows). Build com CMake + JUCE (FetchContent). Testes com Catch2 / JUCE UnitTest. CI GitHub Actions (macOS + Windows).

> Importante: NÃO copie código, assets, nomes de alvos proprietários ou dados da iZotope. Reproduza o *conceito e a organização de interface*. Os alvos de fábrica devem ser gerados por nós (ver seção 6).

## 1. ARQUITETURA
```
/core        (lib estática, sem JUCE GUI)
  SpectrumAnalyzer   – FFT, suavização, média temporal
  TargetModel        – curva média + bandas de tolerância (min/max) em N pontos log-espaçados
  TargetLibrary      – fábrica + usuário, favoritos, busca, filtro, serialização JSON
  TargetBuilder      – cria alvo a partir de arquivo, pasta, ou captura ao vivo
  TargetBlender      – interpolação entre 2 alvos (0–100%)
  HybridEQ           – bell/shelf/HP/LP, fase mínima + opção linear, dinâmico por banda
  Meters/            – VocalBalance, Dynamics(PSR), StereoWidth(correlação), BroadBands(4)
  RolePresets        – tabelas por instrumento
/plugin      (JUCE AudioProcessor + Editor)  → TBC_Track, TBC_Master
/standalone  (modo só análise, captura de áudio do sistema)
/tests
/resources   (alvos de fábrica .json, ícones SVG)
```
Regras: thread de áudio sem alocação/locks (use FIFO lock-free para enviar blocos ao analisador); análise FFT em thread separada; GUI a 30–60 fps via Timer/OpenGL opcional; parâmetros via AudioProcessorValueTreeState; estado completo salvo no projeto da DAW.

## 2. INTERFACE (5 áreas, como no manual do TBC3)
1. **Header**: logo; 3 botões de visualização (Fine / Leveled / Broad); botão abrir/fechar Target Library; botão **Capture**; Settings; Help; Reset.
2. **Target Library** (painel que desliza da esquerda).
3. **Main Meter + EQ**: espectro vs. alvo, escala dB à direita, eixo Hz log (20 Hz–20 kHz), divisões de região Low / Low-Mid / High-Mid / High, nós do EQ com popup (tipo, Freq, Gain, Q, Dynamic Thresh/Down).
4. **Footer**: botão Extra Meters, seletor de EQ (Built-in Equalizer), Gain de saída, botão mostrar/ocultar espectro+EQ, bypass do EQ.
5. **Extra Meters**: Vocal Balance, Dynamics, Stereo Width — cada um mostra uma **faixa sugerida** vinda do alvo selecionado.

Tema escuro, ciano/teal (#1FB5B5 aprox.) sobre azul-marinho (#0E1621), tipografia sem serifa, redimensionável (min 900×600), HiDPI.

### 2.1 Header — comportamento
- **Fine view** (padrão): curva FFT contínua em resolução total sobreposta à banda real do alvo (faixa ondulada). Para decisões cirúrgicas de EQ.
- **Leveled view**: subtrai a curva do alvo do espectro frequência a frequência; alvo vira linha 0 dB; tolerância = corredor estreito centrado em 0 dB. Acima de 0 = mais alto que o alvo.
- **Broad view**: 4 barras (Low, Low-Mid, High-Mid, High) com energia média por região vs. tolerância do alvo; barra muda de cor quando fora da tolerância (verde/ciano = ok, âmbar/vermelho = fora). Limites padrão: 20–200 Hz / 200 Hz–2 kHz / 2–7 kHz / 7–20 kHz (configuráveis em Settings).
- **Capture**: Click = começa a ouvir; click de novo = Stop; analisa e abre diálogo para nomear e salvar como alvo de usuário. Para automaticamente em 3,5 min. No plugin captura o áudio da DAW; no standalone captura a saída do sistema (macOS: Core Audio process tap/ScreenCaptureKit com permissão "Screen & System Audio Recording"; Windows: WASAPI loopback).
- **Settings**: visibilidade de tooltips, opções de saída, tempo de média (Fast/Medium/Slow), resolução de suavização (1/3, 1/6, 1/12 oitava), limites das bandas Broad, latência do EQ (zero/linear).
- **Reset**: zera média do medidor e picos.

### 2.2 Target Library
- Filtro: All Targets / Genre-Role (fábrica) / Custom (usuário).
- Busca por nome (Enter aplica; vazio restaura).
- Lista com alvo selecionado destacado; coração de favorito por item; botão "somente favoritos".
- Selecionar um alvo atualiza simultaneamente: banda no Main Meter **e** faixas sugeridas dos 3 Extra Meters.
- **Target Blender**: seletor de alvo secundário, "Blend with target: X %" (0% = primário, 100% = secundário), botão Save (pede nome → alvo de usuário). Preview ao vivo no Main e Extra Meters.
- **Add Target**: "Create target from audio file…", "Create target from folder…" (média de todos os arquivos), "Import legacy targets from folder…" (importador de formatos antigos, pode ser stub com TODO documentado).

## 3. MOTOR DE ANÁLISE
- Entrada: soma mono para curva tonal (L+R)/2; opção de analisar Mid ou Side separadamente.
- FFT 16384 pts @ 48 kHz (adaptar à taxa), janela Hann, overlap 75%.
- Converter para dB (potência), suavizar em fração de oitava (1/6 padrão), média temporal exponencial (tau configurável) + modo "Hold/Average since reset".
- **Normalização**: tanto o espectro quanto o alvo são comparados após alinhamento de nível médio na faixa 100 Hz–5 kHz (o alvo é de *forma*, não de volume absoluto). Mostrar o offset aplicado.
- Alvo guardado em ~200 pontos log-espaçados: `mean_dB[i]`, `lo_dB[i]`, `hi_dB[i]` (tolerância = percentis 10–90 entre as referências, ou ±1σ; mínimo de ±1,5 dB de largura).
- Banda Broad: média em dB de cada região, comparada com média de lo/hi do alvo na mesma região.

## 4. MAIN METER + HYBRID EQ (detalhado conforme o manual do TBC3)

### 4.1 Elementos do Main Meter
- **Spectrum Detail slider** (canto superior esquerdo do meter): controla suavização do espectro, de "detalhado" (ícone de barras finas) a "suave" (barras largas). Controla a resolução de suavização em fração de oitava (de 1/24 a 1 oitava). Padrão no centro.
- **Tonal Balance Target**: banda sombreada (tolerância do alvo). Dentro da banda = próximo do alvo; acima/abaixo = mais claro, escuro, pesado ou fino que a referência.
- **Your audio's tonal balance**: curva branca em tempo real.
- **Cabeçalho de regiões** (Low / Low-Mid / High-Mid / High) com linhas verticais divisórias. Cada nome de região tem um ícone **"S" (Band Solo)**: clicar passa SÓ aquela região do espectro para audição isolada (não existe no standalone). Implementar com filtros Linkwitz-Riley 4ª ordem nos mesmos cortes das regiões.
- **Alt-Solo**: segurar Alt/Option + clicar no espectro solo momentaneamente as frequências na posição do mouse (filtro band-pass); soltar o clique encerra. Largura de banda configurável em Settings ("Alt-Solo bandwidth").
- **Low End Crest Factor meter**: mede o fator de crista (pico/RMS) das frequências graves (low-pass ~ 20–150 Hz, configurável). Esquerda = grave com mais transientes/punch e mais dinâmica; direita = grave denso/comprimido. Útil para balancear kick e baixo (impacto vs. peso). **A faixa sugerida NÃO muda com o alvo** (valores fixos, mas por Role no TBC Track — ex.: Kick/Bass com faixa própria).
- **EQ Curve**: curva composta de todos os nós ativos, sobreposta ao espectro, atualizada em tempo real durante o arraste.
- Escala vertical em dB à direita; eixo de frequência 20 Hz–20 kHz logarítmico.

### 4.2 Hybrid EQ — nós estáticos + dinâmicos
- **Criar nó**: clicar na EQ Curve ou duplo clique em qualquer ponto do espectro → cria nó e abre o **HUD** (heads-up display) junto ao nó.
- **Nó (EQ node)**: ponto arrastável; esquerda/direita = frequência, cima/baixo = ganho; livre em todo o display.
- **Q handles**: duas alças (uma de cada lado do nó); arrastar na horizontal alarga/estreita o Q.
- **HUD** (da esquerda para a direita), conforme o manual:
  1. Node on/off (liga/desliga sem apagar; preserva ajustes).
  2. Delete node (X).
  3. Filter selector (menu de formas).
  4. Freq (Hz), Gain (dB) e Q — todos com entrada de texto direta. Em filtros de passagem (pass), Gain some e Q vira **Slope (dB/oitava)**.
  5. Seta/toggle para mostrar/ocultar os **Dynamic controls**.
- **Formas de filtro a implementar**: Bell, Proportional Q, Band Shelf, Baxandall Bass, Baxandall Treble, Analog Shelf, Resonant Shelf, Flat Pass, Resonant Pass, Brickwall Pass, Vintage Bell, Vintage Shelf, Surgical Bell, Surgical Shelf, Surgical Pass. (Fase 4a: Bell, Analog Shelf, Baxandall, Flat Pass com slopes 6/12/18/24/36/48 dB/oit; Fase 4b: demais; Brickwall/Surgical via FIR de fase linear.)
- Até 8 nós por instância (configurável). Máx. ±18 dB por nó.
- Fase: Minimum phase (padrão) ou Linear phase (reporta latência ao host); oversampling opcional para formas vintage.

### 4.3 Dynamic mode (por nó)
Cada nó pode virar dinâmico: o ganho responde ao nível do sinal NA BANDA do nó (detector em side-chain interno: band-pass centrado em Freq/Q do nó).
- **Upward Expansion** (triângulo ▲ acima do nó no HUD): ganho aplicado quando o nível da banda CAI ABAIXO do threshold. Uso: devolver presença/brilho em trechos baixos, destacar elementos mascarados.
- **Downward Compression** (triângulo ▼ abaixo do nó): ganho aplicado quando o nível da banda SOBE ACIMA do threshold. Uso: domar picos e frequências intermitentemente altas (ex.: sibilância, ressonância de sala).
- **Dynamic on/off** por nó.
- **Threshold** (dBFS): slider vertical com **medidor em tempo real** do nível da banda ao lado (indicador laranja de pico) para ajustar de ouvido enquanto toca.
- **Gain change readout** (ex.: "Down −1.7"): mostra em tempo real quanto ganho dinâmico está sendo aplicado; 0 dB quando não há ação.
- Parâmetros internos (podem ficar em Settings/avançado): attack, release, ratio/knee, lookahead opcional.
- Ganho final do nó = ganho estático + ganho dinâmico (suavizado, sem zipper noise).

### 4.4 Demais controles do EQ
- Band Solo, Alt-Solo, EQ Bypass, seletor de EQ ("Built-in Equalizer"), Gain de saída (±12 dB), botão de visibilidade espectro/EQ.
- **Suggest EQ**: propõe nós a partir da diferença espectro–alvo (Leveled), limite ±4 dB, Q 0.7–4; sempre editável. No TBC Track, prefira nós **dinâmicos downward** para áreas problemáticas da Role (lama, sibilância, ressonâncias).
- Standalone: sem EQ, sem Band Solo, sem Alt-Solo, sem Gain.
- (Futuro) `IEqBridge` para IPC com outro EQ, só interface.

## 4.5 FOOTER (6 controles, da esquerda para a direita)
1. **Extra Meters open/close**: abre/fecha o painel dos Extra Meters abaixo do display principal; ao abrir, a janela do plugin é redimensionada para acomodar o painel (animar a mudança de altura e avisar o host via `setSize`).
2. **EQ bypass**: contorna o EQ ativo (interno ou remoto). O espectro continua ativo para comparar com/sem correção. Crossfade de ~10 ms, sem clicks.
3. **EQ selector**: escolhe qual motor de EQ os nós controlam. Opções: "Built-in Equalizer" (padrão) e instâncias remotas detectadas via IPC. Com EQ remoto selecionado, os nós do TBC controlam o EQ do plugin conectado em vez do interno.
4. **Gain**: ganho de saída do caminho de sinal interno ou do plugin conectado (campo numérico em dB, ±12 dB).
5. **Spectrum hide/show**: oculta/mostra o espectro em tempo real; oculto = só alvo + curva do EQ (foco na forma do EQ).
6. **EQ hide/show**: oculta/mostra curva e nós do EQ; oculto = só espectro + alvo (uso como analisador puro).
No standalone: itens 2, 3, 4 e 6 não existem (somente análise); manter 1 e 5.

## 4.6 IPC SOURCES (comunicação entre plugins)
Objetivo: o TBC enxerga outros plugins da mesma sessão da DAW (mesmo processo), mostra o espectro de saída deles, controla o ganho de saída e, quando suportado, os parâmetros de EQ.
- **Arquitetura**: biblioteca leve `tbc_ipc` com registro estático/singleton por processo (`std::shared_ptr` em memória compartilhada do processo; `juce::SharedResourcePointer`) mais fallback opcional por memória compartilhada entre processos (para hosts que sandboxam plugins). Cada plugin "fonte" se registra com: id único, nome do track/instância, tipo, versão, capacidades.
- **Protocolo** (versionado, `protocolVersion`): mensagens lock-free (SPSC ring buffer) para (a) espectro de saída (blocos de magnitude por bin), (b) leitura/escrita de ganho de saída, (c) leitura/escrita de nós de EQ (freq, gain, Q, shape, ativo, dinâmico, threshold). Nenhum lock na thread de áudio.
- **Capacidades por fonte** (enum de flags `SpectrumMeter | OutputGain | EqControls`):
  | Fonte | Versão mín. | Espectro | Gain | EQ |
  |---|---|---|---|---|
  | Nectar | 3.1.0 | sim | sim | só se houver EQ 1 ou EQ 2 na cadeia |
  | Neverb | — | sim | sim | não |
  | Neutron | 3.1.0 | sim | sim | só se houver Equalizer na cadeia, ou plugin componente Equalizer |
  | Ozone | 9.0.0 | sim | sim | só se houver Equalizer 1/2 na cadeia, ou componente Equalizer |
  | Relay | 1.0.3 | sim | sim | não |
  | VocalSynth | — | NÃO suportado: não deve aparecer no menu |
  (Nomes de produtos iZotope aparecem aqui só como referência de compatibilidade futura. Como NÃO temos acesso ao protocolo proprietário, implemente apenas o lado do **nosso** protocolo: `IIpcSource`/`IIpcClient`, e adicione também o **nosso próprio plugin "TBC Send"** — um utilitário mínimo que se registra como fonte. Qualquer integração com plugins de terceiros fica atrás de uma interface `IExternalAdapter` e de uma flag CMake, só se houver SDK/documentação pública.)
- **Menu do EQ selector**: lista dinâmica de fontes detectadas (atualiza ao inserir/remover plugins), exibindo nome do track + tipo; itens sem capacidade de EQ ficam disponíveis só para espectro/gain, com os nós do EQ desabilitados e tooltip explicando o motivo.
- **Estados de erro**: fonte desaparece → voltar automaticamente para Built-in e avisar com toast; versão incompatível → item cinza com tooltip "versão mínima X".
- **Roles**: no TBC Track, permitir apontar o medidor para uma fonte IPC (útil para analisar o bus de bateria enquanto edita o EQ de outro plugin).

## 5. EXTRA METERS (painel inferior, 3 medidores lado a lado)
Layout: painel abaixo do Main Meter, aberto/fechado pelo botão Extra Meters do Footer. Cada medidor = ícone + nome + trilho horizontal com **faixa sugerida sombreada** (muda automaticamente conforme o alvo selecionado: de fábrica, capturado, criado de arquivo/pasta ou blend salvo — uma única seleção de alvo define a banda do Main Meter E as 3 faixas) e um marcador circular com o valor atual. Ícones nas extremidades indicam o sentido: Vocal Balance ↓ … ↑; Dynamics (ondas esparsas … ondas densas); Stereo Width (○ … ○○). Marcador dentro da faixa = ok; fora = cor de alerta. Cada alvo (JSON) guarda `suggested.vocalBalance`, `suggested.psr` e `suggested.stereoWidth` como [min,max]; ao criar um alvo a partir de áudio, esses intervalos são calculados dos mesmos arquivos (percentis 10–90).

1. **Vocal Balance** — posição do vocal em relação à mix, via separação de fontes em tempo real. Esquerda = vocal baixo na mix; direita = vocal proeminente/à frente. Valor = razão de energia (dB) vocal estimado ÷ resto, suavizada (~3 s). Fase 1: heurística (componente central + banda de voz) atrás de `IVocalSeparator`; Fase 2: modelo ONNX leve em thread separada. No TBC Master é o foco; no TBC Track fica ativo só nas Roles de vocal (Lead/Backing), onde passa a significar "vocal vs. a si mesmo/ambiente" e deve vir desativado por padrão com aviso — nos demais instrumentos fica oculto.
2. **Dynamics** — PSR (Peak to Short-term loudness Ratio) = true peak (oversample 4×) − loudness de curto prazo (K-weighted, janela 3 s, BS.1770/EBU R128). Esquerda = mais dinâmico, transientes, punch e headroom; direita = mais denso/comprimido. Faixa sugerida = típica do gênero/Role.
3. **Stereo Width** — correlação de fase broadband. Esquerda = imagem focada, compatível com mono; direita = mais larga/espaçosa. Mapear correlação (+1…−1) para a escala mono→largo. Em Roles mono (kick, baixo, lead vocal) a faixa sugerida fica estreita à esquerda e um aviso aparece se a correlação cair abaixo de 0,7.

Comportamento comum: ballistics suaves (attack rápido, release lento), botão Reset (o mesmo Reset do Header), tooltips explicando os sentidos esquerda/direita, e valores numéricos opcionais ao passar o mouse. Todos funcionam também no standalone (análise).
(O **Low End Crest Factor** NÃO pertence a este painel: fica no Main Meter, ver 4.1, e sua faixa sugerida não muda com o alvo.)

## 6. ALVOS DE FÁBRICA (a gerar por nós, sem usar dados de terceiros)
Crie o utilitário `tools/build_targets` (CLI) que lê uma pasta de WAVs de referência por categoria (fornecida pelo usuário), calcula média/percentis e exporta `.json` para `/resources/targets`. Enquanto não houver referências, inclua **alvos-semente paramétricos** (curvas sintéticas conforme a tabela abaixo) marcados `"provisional": true`, que o usuário substitui depois.

### 6.1 Roles do TBC Track (seletor "Role") — curvas-semente e pontos de atenção
| Role | Foco tonal (guia de engenheiro de mixagem) | Zonas a vigiar |
|---|---|---|
| Lead Vocal | Presença 2–5 kHz, ar 10–14 kHz, corpo 150–300 Hz | Lama 200–400 Hz, nasal 800 Hz–1 kHz, sibilância 5–8 kHz, rumble <80 Hz |
| Backing Vocal | Menos presença que o lead (recuar 1–3 dB em 3–5 kHz), mais largo | Colisão com lead em 2–4 kHz, lama 250 Hz |
| Kick | Sub 50–70 Hz, "punch" 80–120 Hz, click 3–5 kHz | Caixa 300–500 Hz, excesso 150–250 Hz |
| Snare | Corpo 180–250 Hz, estalo 4–6 kHz, ar 10 kHz | Caixa 400–600 Hz, ring 700 Hz–1 kHz |
| Toms | Fundamental 80–200 Hz, ataque 3–5 kHz | Caixa 300–500 Hz |
| Overheads/Cymbals | Brilho 8–16 kHz | Harshness 2–4 kHz, graves <200 Hz (HPF) |
| Drum Bus | Soma dos acima | Equilíbrio sub vs. punch |
| Bass (electric/synth) | Fundamental 40–100 Hz, definição 700 Hz–1 kHz | Mud 200–300 Hz, buraco 120–180 Hz |
| Electric Guitar | Corpo 100–250 Hz, mordida 2–4 kHz | Caixa 300–500 Hz, fizz >6 kHz, colisão com vocal |
| Acoustic Guitar | Corpo 100–200 Hz, brilho 5–12 kHz | Boominess 150–250 Hz |
| Keys/Piano | Corpo 200–500 Hz, brilho 3–8 kHz | Acúmulo em 250–400 Hz |
| Synth Pad | Largura e média-alta 1–6 kHz | Excesso de graves/lama |
| Strings/Brass | Calor 200–400 Hz, presença 2–5 kHz | Aspereza 2–4 kHz |
| Percussion | Ataque 2–8 kHz | Graves desnecessários (HPF) |
| FX/Ambience | Ar, altas | Acúmulo em médios |
| Custom | Alvo do usuário | — |
Cada Role carrega também: faixa sugerida de PSR, de Stereo Width, e se Vocal Balance está ativo.

### 6.2 TBC Master — alvos por gênero (provisórios)
All-Purpose, Pop, Dance Pop, Hip-Hop, Classic Rock, Alt Rock, Country, EDM, Drum & Bass, Dubstep, Ambient, Dark Cinematic, Acoustic/Singer-Songwriter, Funk/Soul, Sertanejo, Samba/Pagode, MPB, Forró, Gospel, Reggaeton, Jazz, Classical. (Gênero brasileiro incluído de propósito.) Cada um com curva-semente + PSR e Width sugeridos.

## 7. PERSISTÊNCIA
- Alvos de usuário em `~/Documents/<Vendor>/TBC/Targets/` (JSON versionado, com `schemaVersion`).
- Favoritos e último alvo/role em settings.json.
- Estado do plugin (nós de EQ, alvo selecionado, blend, view) salvo no projeto da DAW.

## 8. QUALIDADE E TESTES
- Unit tests: FFT vs. seno conhecido (±0,1 dB), PSR vs. sinal de referência, correlação (mono = +1, invertido = −1), blender (0%/50%/100%), serialização round-trip, normalização de nível.
- Teste de tempo real: sem alocações no `processBlock` (use verificação em debug), CPU < 5% por instância a 48 kHz/512 samples.
- `pluginval` nível 8 em CI. Validar AU com `auval`.
- Latência reportada corretamente, bypass sem clicks, automação de Gain/EQ suavizada.

## 9. ENTREGAS (em ordem — pare e valide a cada fase)
1. Esqueleto CMake/JUCE + CI + janela vazia com tema.
2. SpectrumAnalyzer + Fine/Leveled/Broad com alvo-semente.
3. TargetLibrary (lista, busca, filtro, favoritos) + TargetBuilder (arquivo/pasta/Capture) + Blender.
4. Main Meter completo (Spectrum Detail slider, Band Solo "S", Alt-Solo, Low End Crest Factor) + Hybrid EQ (nós estáticos → HUD → formas de filtro → Dynamic mode com Upward/Downward, threshold meter e gain readout → fase linear) + Suggest EQ.
5. Extra Meters (PSR, Width, Vocal Balance heurístico).
6. Roles do TBC Track + alvos do TBC Master + ferramenta `build_targets`.
7. Footer completo (Extra Meters open/close, bypass, selector, Gain, hide/show) + IPC (`tbc_ipc`, plugin TBC Send, menu de fontes).
8. Standalone com captura de áudio do sistema.
9. Testes, documentação (README, manual de usuário em Markdown) e empacotamento (instaladores).

Ao final de cada fase: liste o que foi feito, como testar e pendências. Não invente APIs; quando uma dependência (ONNX, captura do sistema, AAX SDK) exigir configuração do usuário, deixe o código atrás de uma flag CMake e documente.
# MAX Tonal Balance

Suíte original de análise tonal: **TBC Track** e **TBC Master**.
Entrega principal: **Windows x64 / VST3**, C++20 e JUCE 8.0.6.
Não inclui código, dados de alvos ou assets da iZotope.

## Fase 1 — base de desenvolvimento

Inclui dois processadores com passagem de áudio sem alteração, estado APVTS,
janela escura redimensionável (mínimo 900 × 600), seletores persistentes e
pipeline Windows. O preview mostra curvas-semente; FFT ao vivo, importação
de alvos e EQ pertencem às fases seguintes.

### Compilar no Windows

Instale Visual Studio 2022 com Desktop development with C++, Windows SDK,
Git e CMake >= 3.22. No Developer PowerShell x64:

```powershell
cmake -S . -B build -A x64 -DBUILD_TESTING=ON
cmake --build build --config Release --parallel 3
ctest --test-dir build -C Release --output-on-failure
```

Os bundles ficam em `build/TBC_Track_artefacts/Release/VST3/` e
`build/TBC_Master_artefacts/Release/VST3/`. Copie-os para
`C:\Program Files\Common Files\VST3` e refaça a busca de plugins na DAW.
O CI disponibiliza esses diretórios como artefatos quando executado no GitHub.
O build Windows é executado e testado pelo CI; a sessão Linux não valida
carregamento em DAW Windows.

### Validação manual da fase

Abra ambos em uma DAW Windows, envie áudio mono/estéreo e confirme passagem
inalterada. Redimensione a janela. Troque Fine/Leveled/Broad, salve e reabra o
projeto: a seleção deve persistir. Os testes automatizados verificam 1024
amostras de passagem estéreo, round-trip de estado, criação/redimensionamento
da janela e vínculo do seletor ao parâmetro.

### Opções

- `TBC_BUILD_STANDALONE=ON`: aplicativos opcionais de desenvolvimento; nesta
  fase passam áudio, ainda não são o modo de captura/análise de sistema.
- `TBC_BUILD_AAX=ON` e `TBC_AAX_SDK_PATH`: SDK licenciado obrigatório; não validado.
- `TBC_EXTERNAL_ADAPTERS`: reservado para adaptadores públicos futuros; sem implementação.

### Ambiente Linux de desenvolvimento

A compilação nativa Linux é auxiliar e não produz um VST3 Windows. Requer
CMake, Ninja, pkg-config, ALSA, FreeType/fontconfig e headers X11/Xrandr/
Xinerama/Xcursor/Xext/OpenGL. Configure com `cmake -S . -B build -G Ninja`,
compile e execute CTest com um display X11 ativo (Xvfb em sessões sem tela). Acesso ao GitHub é necessário para FetchContent.
Use o checkout existente: cada tarefa da nuvem já é isolada; não crie worktree.

## Pendências por fase

2. FFT, normalização, Fine/Leveled/Broad e alvo sintético.
3. Biblioteca, construção/captura e blend de alvos.
4. Medidor principal e EQ estático/dinâmico.
5. Medidores PSR, largura e estimativa vocal.
6. Roles, alvos sintéticos provisórios e CLI build_targets.
7. Footer completo, protocolo IPC próprio e TBC Send.
8. Standalone de análise e captura do sistema.
9. Testes DSP/tempo real, pluginval nível 8, manual e instalador Windows.

pluginval, testes DSP e instalador ainda não foram implementados. As pastas
core, standalone, resources e tools serão preenchidas nas fases correspondentes.

## Licenciamento

JUCE é uma dependência de terceiros sob os termos indicados no próprio projeto
(licença comercial ou AGPLv3 conforme aplicável). Antes de distribuir binários,
defina a licença do código deste repositório e cumpra a licença JUCE escolhida.
Nenhum SDK AAX ou biblioteca proprietária foi incluído.

## Prévia com todos os alvos-semente

Inclui **38 alvos provisórios originais**: 22 gêneros Master e 16 Roles Track,
com 200 pontos por curva e tolerâncias/faixas sugeridas sintéticas. Seleção e
preview da curva estão disponíveis no editor e são salvos no estado da DAW.
Não são presets medidos nem presets de EQ. O áudio continua inalterado:
FFT ao vivo, EQ e os medidores completos ainda não existem.

`resources/targets/factory.json` é embutido nos plugins, sem instalação externa.
`python tools/generate_seed_targets.py` reproduz os dados originais. Custom
é uma semente neutra, ainda sem importação de alvos do usuário.

O workflow Windows gera ZIP com os dois bundles VST3, os alvos JSON e README,
além de SHA256. Tags `v*-preview` publicam o ZIP como pré-lançamento **somente
após build e testes Windows aprovarem**. O CI precisa de GitHub Actions ativo e
permissão de escrita de conteúdo para a etapa de Release.

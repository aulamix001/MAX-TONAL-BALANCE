# Validação da fase 1

Escopo principal: Windows x64 / VST3. Implementação inicial, sem DSP de análise ou EQ.

Executado no ambiente Linux de onboarding:

- CMake 4.4.4 + Ninja 1.13.2, GNU C++ 14.2, JUCE 8.0.6.
- Configuração e build Release de TBC Track VST3, TBC Master VST3 e tbc_tests: passaram.
- JUCE UnitTest: 3 grupos, 1030 verificações passaram, 0 falhas.
  Passagem estéreo (1024 amostras), round-trip APVTS e criação/redimensionamento
  do editor com seleção restaurada.
- CTest: 1/1 executado e aprovado.
- Manifestos dos dois bundles: aprovados pelo `juce_vst3_helper -validate`.
  O formato gerado pelo SDK aceita vírgulas finais; use a ferramenta do SDK
  para validá-lo, não um parser JSON estrito.
- Script completo `/workspace/tooling/install.sh`: repetido com sucesso.
- Bibliotecas e ferramentas instaladas apenas em `/workspace/tooling`.

Ainda não executado: CI Windows, carregamento em DAW Windows, pluginval,
benchmark de CPU, testes DSP, captura de sistema ou instaladores.
Os bundles gerados nesta máquina são Linux e não podem ser usados no Windows.

A configuração reutilizável foi salva nos campos install_script e start_skill.
Publicação do ambiente e restauração em uma tarefa nova não foram verificadas.

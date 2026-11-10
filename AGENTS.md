# Agentes de programação — Modulock

Firmware de controle de porta: mudanças são de **alto impacto físico**.

- Leia src/main.cpp, platformio.ini, docs/REQUISITOS.md, docs/ARQUITETURA.md antes de mudar algo.
- Nunca expor credenciais reais em fonte, logs ou PRs.
- Manter fail-closed em erro de configuração ou Wi-Fi.
- Não reativar Bluetooth sem autorização forte, testes de ameaça e revisão.
- Não alegar que cookie sem TLS garante canal seguro.
- Não modificar acionamento do relé ou o circuito sem validar estado seguro.
- Executar pio run e testes no equipamento. 
- Preferir nomenclatura em português sem quebrar bibliotecas e APIs ESP32 existentes.

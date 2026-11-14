# Controle documental — Modulock

**MOD-DOC v1.0.0** · 11/11/2026 · base de código `a83b7fd3d97d5a356e869e172ccaf761aceb150b`.

| Documento | Versão | Natureza |
|---|---|---|
| [Especificação](../projeto/ESPECIFICACAO.md) | 1.0.0 | Firmware, GPIOs, sessão, rotas e armazenamento |
| [UML](../uml/README.md) | 1.0.0 | Componentes, abertura e estado do relé |
| [Testes](../testes/PLANO_DE_TESTES.md) | 1.0.0 | Matriz de 18 testes e riscos de segurança |

**MAJOR:** mudança de controle de acesso/política de autenticação. **MINOR:** protocolo, hardware, rotas ou testes novos. **PATCH:** documentação editorial. Toda revisão deve incluir commit, placa, firmware gravado e resultado HIL. Nenhum diagrama substitui ensaio com hardware.

## Revisão de segurança — 14/11/2026

- HTTP administrativo desligado por padrão em firmware compilável.
- Remoção da interface de comandos Bluetooth sem autenticação.
- Operação RFID local independente da conexão Wi-Fi.
- Montagem SPIFFS sem formatação automática.
- Regressões estáticas no Python e compilação PlatformIO em CI.

A origem está no PR de segurança com base na `main`. Esta revisão **não** aprova uso como fechadura real: faltam testes físicos e autenticação criptográfica adequada.

## MOD-TIMER v2.4.0 proposta — 14/11/2026

- Temporizadores para relé e LEDs deixam de usar `millis() + prazo` e `millis() >= prazo`, adotando delta modular com atividade explícita.
- Incluídos testes C++ host de overflow e verificações de wiring em Python, sem flash ou atuação de hardware.
- Evidência de validação automatizada deve acompanhar o PR; HIL permanece necessário para produto físico.

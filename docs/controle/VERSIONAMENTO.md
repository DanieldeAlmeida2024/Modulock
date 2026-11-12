# Controle documental — Modulock

**MOD-DOC v2.0.0 proposta** · 11/11/2026 · base de código `a83b7fd3d97d5a356e869e172ccaf761aceb150b`.

| Documento | Versão | Natureza |
|---|---|---|
| [Especificação](../projeto/ESPECIFICACAO.md) | 2.0.0 proposta | Firmware, GPIOs, sessão, rotas e armazenamento |
| [UML](../uml/README.md) | 2.0.0 proposta | Componentes, abertura e estado do relé |
| [Testes](../testes/PLANO_DE_TESTES.md) | 2.0.0 proposta | Matriz de 18 testes e riscos de segurança |

**MAJOR:** mudança de controle de acesso/política de autenticação. **MINOR:** protocolo, hardware, rotas ou testes novos. **PATCH:** documentação editorial. Toda revisão deve incluir commit, placa, firmware gravado e resultado HIL. Nenhum diagrama substitui ensaio com hardware.

## Histórico e evidência da revisão

| Versão | Origem | Alterações | Estado |
|---|---|---|---|
| 1.0.0 | main `e1b7f75fe6a1b51f85b4e96c28b5e0545bc7e792` | Snapshot de controle de acesso e plano de HIL | Inspeção estática, hardware não testado |
| 2.0.0 proposta | branch `fix/p0-uid-exposure-and-registration-20261111` | GET UID autenticado, sem cache; cadastro dependente de leitura RFID recente e consumível; comandos Bluetooth legados removidos; regras C++ e CI host/PlatformIO | CI #43089433513 host C++ e PlatformIO aprovados; HIL ainda pendente |

**Por que MAJOR:** `GET /getLastScannedUid` deixa de ser público; `POST /registerUser` recusa UID não lido recentemente; há alteração deliberada do contrato HTTP e retirada do canal de comandos não autenticado.

[Política de segurança](../projeto/POLITICA_SESSAO_UID.md) · [Testes](../testes/PLANO_DE_TESTES.md) · [UML](../uml/04_sequencia_registro_uid.puml). Não há autorização para instalar firmware em fechadura real.


### Evidência de CI

[GitHub Actions #43089433513](https://github.com/DanieldeAlmeida2024/Modulock/actions/runs/43089433513): `Guardas UID - C++ host` = **success** e `Compilar firmware ESP32` = **success**. Não houve flash, teste HIL nem liberação de fechadura física.

## MOD-TIME v2.1.0 proposta — auditoria de temporizadores (11/11/2026)

| ID | Artefato | Revisão |
|---|---|---|
| MOD-TIME | `include/ModulockTimingPolicy.h`, `src/main.cpp` | Expiração por delta uint32 e flags para relé/LEDs |
| MOD-PRIV | `src/main.cpp` | Remove RA, nome e UID do log serial de cadastro |
| MOD-HOST | `test/security_policy_test.cpp` | Testes de wrap, limite temporal, inativo e duração inválida |
| MOD-WIRE | `test/test_timing_privacy_wiring.py` | Guarda de regressão do uso das regras e não impressão de dados |
| MOD-CI | `.github/workflows/security-ci.yml` | G++ host, Python de wiring e PlatformIO |
| MOD-DOC | `docs/projeto/RELE_TEMPORIZACAO.md` e UML #05 | Riscos, estados, diagnóstico, HIL pendente |

Branch `fix/timing-rollover-and-serial-privacy-20261111`, empilhada sobre o PR #3. Código e diagrama propõem correção de software **sem homologação elétrica nem autorização de flash**. Não liberar antes de ensaio HIL e revisão de sincronização entre tarefas, rede HTTP, sessões/CSRF e segurança dos cartões.

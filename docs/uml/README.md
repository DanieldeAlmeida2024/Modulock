# Modulock — UML (firmware e hardware)

> **MOD-DOC v2.0.0 proposta** · 11/11/2026 · código auditado `a83b7fd3d97d5a356e869e172ccaf761aceb150b` · documentação por inspeção estática, sem validação do hardware.

- [Componentes reais](01_componentes.puml)
- [Sequência HTTP para abertura](02_fluxo_abrir_porta.puml)
- [Estados do relé](03_estados_rele.puml)
- [Nova sequência: leitura e cadastro RFID protegido](04_sequencia_registro_uid.puml)

Os diagramas descrevem o código fonte e a pinagem. Eles não demonstram que o mecanismo é resistente a ataques físicos ou Bluetooth.

- [Relé/LEDs e overflow de millis()](05_temporizadores_rele_wrap.puml) — MOD-TIME v2.1.0 proposta, testes HIL pendentes.

- [Sessão, CSRF e logout no servidor](06_sessao_revogacao_csrf.puml) — v2.2.0 proposta, HIL pendente.

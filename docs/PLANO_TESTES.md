# Plano de testes de segurança e hardware

- CT-01: sem credenciais → servidor não inicia, relé em estado bloqueado.
- CT-02: cookie inventado, expirado ou vazio → 401/redirect, nenhuma abertura.
- CT-03: login correto → sessão aleatória; login errado → rejeição.
- CT-04: Bluetooth desativado → comando OPEN DOOR não aciona o relé.
- CT-05: reboot → identificador anterior perde validade.
- CT-06: cadastro/remoção de cartões → apenas através de sessão legítima.
- CT-07: falha de Wi-Fi → relé permanece bloqueado.
- CT-08: compilação `pio run` e testes físicos com relé desconectado inicialmente.

A inspeção de texto não substitui os testes acima. Não acionar fechadura real com firmware não validado.

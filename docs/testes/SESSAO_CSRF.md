# Modulock — Plano de testes MOD-SESSION (v2.2.0 proposta)

| ID | Cobertura | Verificação |
|---|---|---|
| MOD-S01 | Host C++ | Sem sessão no boot, acesso negado. |
| MOD-S02 | Host C++ | Token válido/errado e CSRF recuperado apenas com digest certo. |
| MOD-S03 | Host C++ | Novo login revoga imediatamente cookie do anterior. |
| MOD-S04 | Host C++ | Mutação negada sem nonce ou com nonce diferente. |
| MOD-S05 | Host C++ | Logout remove a sessão em memória e impede replay. |
| MOD-S06 | Host C++ | Inatividade de 10 min e tempo absoluto de 30 min. |
| MOD-S07 | Host C++ | Rollover `millis()` e timestamp zero. |
| MOD-S08 | Python wiring | Rotas POST exigem CSRF antes do efeito, logout é server-side. |
| MOD-S09 | Python wiring | UI HTML e JS enviam nonce e não apagam só cookie. |
| MOD-S10 | PlatformIO | Build completo ESP32, sem flash. |
| MOD-S11 | HIL pendente | 401/403/200 em HTTP com browser, chamadas concorrentes, reinício. |
| MOD-S12 | Rede pendente | HTTPS, cookie `Secure`, proxy, brute-force, XSS, CSRF cross-site. |
| MOD-S13 | HIL pendente | Relé, RFID e GPIO no cenário de expiração e logout. |

Comandos: `g++ -std=c++11 -Wall -Wextra -Werror -Iinclude test/session_policy_test.cpp -o /tmp/modulock_session_test && /tmp/modulock_session_test`, `python3 test/test_session_wiring.py`, `pio run -e esp32dev`.

Os casos host usam digests sintéticos. Não provam entropia da RNG do ESP32, HTTPS nem funcionalidade no hardware. Consultar CI no SHA final antes de marcar PASS.

### Estado dos testes automatizados

Os comandos g++ C++11, verificações Python do wiring e build PlatformIO foram executados em [#43140037802](https://github.com/DanieldeAlmeida2024/Modulock/actions/runs/43140037802) no commit `62a1723266000d03b61e7bf5d78326a41a912cb8`, com **ambos os jobs SUCCESS**. MOD-S11..S13 (HIL, TLS, GPIO real) seguem **PENDENTES** e não podem ser marcados como aprovados. O primeiro CI falhou e as correções estão documentadas no histórico de versões.

### Testes complementares MOD-LOGIN v2.3.0

Os controles de sessão da v2.2.0 permanecem, agora precedidos de um limitador global de autenticação e do gate de HTTP em firmware. [Matriz de rate-limit e modo laboratório](LOGIN_RATE_LIMIT.md). Não atribuir PASS a rede/HIL/TLS com base no compilador.

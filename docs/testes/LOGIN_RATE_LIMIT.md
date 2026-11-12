# Modulock — Plano de testes: login throttle e gate de HTTP

**MOD-LOGIN-TST v2.3.0 (proposta)**. Testes C++/Python/PlatformIO sem dispositivo.

| ID | Camada | Cenário | Resultado esperado |
|---|---|---|---|
| MOD-L01 | C++ host | Primeira requisição quando `millis()==0` | Permitida, janela inicializada |
| MOD-L02 | C++ host | 2ª requisição antes de 2 segundos | Negada e `Retry-After` correto |
| MOD-L03 | C++ host | Cinco erros espaçados 2 segundos em 60 segundos | Bloqueio global de 300 segundos |
| MOD-L04 | C++ host | Durante o bloqueio/1 ms antes do fim | Negado |
| MOD-L05 | C++ host | Fim exato do bloqueio | Novo login possível |
| MOD-L06 | C++ host | Autenticação bem-sucedida | Limpa histórico de erros e cooldown |
| MOD-L07 | C++ host | Janela de 60 segundos expirada | Contagem de falhas reiniciada |
| MOD-L08 | C++ host | `millis()` cruza `0xFFFFFFFF` | Política respeita limite e bloqueio modular |
| MOD-L09 | Python wiring | Throttle antes de ler senha e `adminSession.issue` | Falha não alcança criação de sessão |
| MOD-L10 | Python wiring | Mutex em `POST /login` | Verificação e escrita serializadas |
| MOD-L11 | Python wiring | Macro de HTTP ausente no build padrão | `setup()` retorna antes do Wi-Fi e servidor |
| MOD-L12 | PlatformIO | `pio run -e esp32dev` e `esp32dev-insecure-lab` | Ambos compilam sem flash |
| MOD-L13 | HIL pendente | 429/Retry-After, logout, login válido, múltiplas conexões | Comportamento real avaliado em rede isolada |
| MOD-L14 | Segurança pendente | Reset/reboot, DoS por bloqueio global, IP spoof, password guessing | Limites e recuperação documentados/ensaiados |
| MOD-L15 | Infraestrutura pendente | TLS fim a fim, cookie Secure, gateway confiável | Nenhum HTTP aberto para cliente externo |
| MOD-L16 | HIL pendente | Default sem macro, RFID/relé sem atuação | Estado físico fail-closed confirmado com instrumento |

Comandos: `g++ -std=c++11 -Wall -Wextra -Werror -Iinclude test/login_throttle_test.cpp -o /tmp/modulock_login_throttle_test && /tmp/modulock_login_throttle_test`, `python3 test/test_login_throttle_wiring.py`, `pio run -e esp32dev`, `pio run -e esp32dev-insecure-lab`.

**Observação:** a macro `MODULOCK_ALLOW_INSECURE_HTTP_LAB` habilita deliberadamente um serviço HTTP **não seguro** apenas no build de laboratório. Testes host e builds do firmware não certificam resistência de segurança de produção nem acessibilidade da fechadura.

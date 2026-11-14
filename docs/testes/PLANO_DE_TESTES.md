# Modulock — plano de validação de firmware e segurança

> **MOD-DOC v2.0.0 proposta** · 11/11/2026 · código auditado `a83b7fd3d97d5a356e869e172ccaf761aceb150b` · documentação por inspeção estática, sem validação do hardware.

## Preparação de bancada
Executar inicialmente **sem fechadura energizada**. Usar uma carga de teste adequada ao relé e fonte separada quando necessário, com avaliação elétrica e proteção de cabos. Documentar versão de placa, fonte, firmware, UID de cartões sintéticos e cenário de falha. Não automatizar abertura real de porta sem supervisão.

## Matriz de aceitação
| ID | Camada | Cenário | Aceitação / evidência |
|---|---|---|---|
| MOD-T01 | Compilação | `pio run -e esp32dev` | Binário gerado na placa especificada |
| MOD-T02 | Boot | SPIFFS presente e `users.json` válido | Usuários carregados sem corromper arquivo |
| MOD-T03 | Resiliência | JSON ausente ou inválido | Array vazio e sem liberação indevida |
| MOD-T04 | Negócio | Cadastrar RA/UID únicos | Registro salvo e recuperado |
| MOD-T05 | Negócio | Repetir RA ou UID | Recusa de duplicidade |
| MOD-T06 | Sessão | Cookie `session_id=qualquer` | Resposta não autorizada |
| MOD-T07 | Sessão | Cookie válido, valor alterado ou múltiplos cookies | Apenas sessão exata permite acesso |
| MOD-T08 | HTTP | `POST /openDoor` sem sessão | 401, relé permanece HIGH |
| MOD-T09 | HTTP | `GET /getUsers` sem sessão | Lista não revelada |
| MOD-T10 | Física | Acionar relé autorizado | LOW por ~1 segundo e retorno HIGH |
| MOD-T11 | Física | Reiniciar durante acionamento | Falha segura, porta não permanece energizada |
| MOD-T12 | Física | Ler UID desconhecido | Relé não é acionado |
| MOD-T13 | Bluetooth | Comando `OPEN DOOR` sem autenticação forte | **Bloqueante:** não pode liberar a porta |
| MOD-T14 | Segurança | Dump serial e logs | Nenhuma senha ou lista de usuários sensível |
| MOD-T15 | Falha elétrica | Queda/retorno de energia | Estado seguro e sem corrupção de usuários |
| MOD-T16 | Persistência | Remoção de RA e reboot | UID deixa de autorizar acesso |
| MOD-T17 | Rede | Usuário não autenticado em cada endpoint | Sem mutação ou exposição de cadastro |
| MOD-T18 | Resiliência | `millis()` próximo de overflow | Agendamento temporizado correto |

## Classificação
- **P0 bloqueante:** MOD-T06–T13, T15–T17. Falhar impede uso em fechadura real.
- **P1:** MOD-T01–T05, T14, T18. Necessários para repetibilidade e manutenção.

## Automação disponível e pendências
`platformio.ini` define o alvo `esp32dev`; não foi localizada suíte de firmware/HIL. Este documento especifica **casos a implementar**, não resultados PASS. A verificação física exige equipamento e registro de evidências. Como risco conhecido, o tratamento de Bluetooth deve ser revisado antes de considerar o sistema seguro.

## Casos adicionais — revisão 2.0.0 proposta (não executados até CI/HIL)

| ID | Cenário | Aceitação | Camada |
|---|---|---|---|
| MOD-T19 | GET último UID sem cookie ou cookie inválido | 401, sem exposição | HTTP/HIL |
| MOD-T20 | GET UID autenticado recente versus expirado | UID somente recente; `No UID` após timeout; `Cache-Control: no-store` | C++/HTTP |
| MOD-T21 | Cadastro com UID digitado e não escaneado | 409, nenhum usuário criado | HTTP/HIL |
| MOD-T22 | Cadastro com UID expirado ou diferente | 409, sem escrita SPIFFS | C++/HTTP/HIL |
| MOD-T23 | Cadastro legítimo após leitura e reutilização | Primeiro cadastro permitido e UID consumido; replay negado | HTTP/HIL |
| MOD-T24 | Overflow de `millis()` | TTL correto ao cruzar limite uint32 | C++ host |
| MOD-T25 | Canal Bluetooth de comandos | Não há inicialização, processamento ou endpoint aceitando comandos no firmware | CI host/inspeção |
| MOD-T26 | Build firmware | `pio run -e esp32dev` passa sem dependências faltando | CI PlatformIO |
 
Os 18 casos anteriores continuam planejados. Os testes C++ isolam **política**, e o teste Python verifica o código-fonte — não são substitutos da execução do ESP32 com RFID, relé e HTTP reais.


### Resultado da automação da revisão

[Workflow #43089433513](https://github.com/DanieldeAlmeida2024/Modulock/actions/runs/43089433513) **aprovado**: política C++11 (`g++`) e ligação de handlers (Python) aprovadas; compilação ESP32 com PlatformIO aprovada. MOD-T19–T23 continuam exigindo validação de HTTP/RFID real e MOD-T26 comprova apenas build, não operação elétrica.

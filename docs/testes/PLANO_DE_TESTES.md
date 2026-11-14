# Modulock — plano de validação de firmware e segurança

> **MOD-DOC v1.0.0** · 11/11/2026 · código auditado `a83b7fd3d97d5a356e869e172ccaf761aceb150b` · documentação por inspeção estática, sem validação do hardware.

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

## Regressões de segurança e gates automatizados — 14/11/2026

| Identificador | Contrato | Automação |
| --- | --- | --- |
| MOD-S01 | Interface HTTP administrativa desligada por padrão | `tests/test_security_defaults.py` |
| MOD-S02 | Remoção de comandos Bluetooth que poderiam acionar o relé | `tests/test_security_defaults.py` |
| MOD-S03 | Laço RFID independente do servidor web | `tests/test_security_defaults.py` |
| MOD-S04 | Não formatar o SPIFFS ao falhar montagem | `tests/test_security_defaults.py` |
| MOD-S05 | Credenciais reais ausentes do arquivo de exemplo | `tests/test_security_defaults.py` |
| MOD-C01 | Compilar para `esp32dev` com PlatformIO | `.github/workflows/compile-esp32.yml` |

**Separação de evidências:** testes estáticos apenas verificam padrões no código. A compilação prova que o alvo especificado é compilável, mas não que o comportamento elétrico, o fail-safe ou os tempos sejam corretos na placa. O acionamento do relé, RFID, falhas de energia e tentativas de ataque continuam bloqueados para uso real até teste HIL controlado.

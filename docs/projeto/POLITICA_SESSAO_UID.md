# Modulock — política de sessão e dados RFID

**Documento:** MOD-SEC-001 · **Versão:** 2.0.0 proposta · **Revisão:** 2026-11-11 · **Branch:** `fix/p0-uid-exposure-and-registration-20261111`.

## Fronteira de confiança
O UID de cartão RFID é um identificador de credencial; divulgá-lo pode facilitar clonagem, associação indevida ou tentativa de registro. Não é fator criptográfico de autenticação. A leitura sensível só deve ocorrer após autenticação administrativa, em rede isolada e com expiração curta.

## Matriz de controles na revisão

| Componente | Proteção codificada | Riscos residuais |
|---|---|---|
| GET `/getLastScannedUid` | Sessão obrigatória; 401 para visitante; UID disponível somente por 30s; sem cache | Cookie em HTTP sem TLS; múltiplos administradores compartilham sessão global |
| POST `/registerUser` | Sessão + UID físico recente e correspondente; leitura consumida após sucesso | Operador autenticado ainda pode cadastrar pessoa errada; exigir auditoria e validação humana |
| Bluetooth | Handler de comandos/loop removidos, sem dispositivo inicializado | Não reativar sem pareamento forte, autenticação de comandos e replay protection |
| `users.json` | Persistência SPIFFS existente | Arquivo pode ser extraído do dispositivo; proteção física/flash encryption pendente |
| `/getUsers` | Já exigia sessão | Lista completa com UID, limitada à sessão administrativa; considerar mascaramento |
| `/openDoor` | Já exige sessão | HTTPS/CSRF/rate limit; relé e RFID sem proteção anti-clone |

## Limites e aceitação
- Testes host: `test/security_policy_test.cpp` compila regras C++11 para TTL e replay; `test/test_security_wiring.py` valida presença de guardas nas rotas. Testes em CI são **evidência parcial**.
- CI firmware `pio run` comprova compilação, não valida relé/instalação.
- Teste HIL em bancada sem fechadura: leitura recente, expiração, sessão inválida, consumo de UID após cadastro, power cycle e operação do relé.
- Não há homologação para controle de acesso físico em produção. Devem existir plano de emergência de saída, conformidade elétrica e avaliação dos riscos de clonagem dos cartões.
- Não promover segurança com base apenas no fato de Bluetooth não inicializar; ausência de comandos é protegida por teste de regressão na nova build.


## Execução da verificação de código e firmware

**CI aprovado em 11/11/2026:** [run #43089433513](https://github.com/DanieldeAlmeida2024/Modulock/actions/runs/43089433513), incluindo teste host de TTL, overflow, UID idêntico e falha de autorização, inspeção de ligação das rotas e compilação PlatformIO do ESP32. **HIL, rede real, cookies e hardware de relé não testados**. O resultado não certifica a fechadura.

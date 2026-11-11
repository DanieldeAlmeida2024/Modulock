# Modulock — especificação do controle de acesso ESP32

> **MOD-DOC v2.0.0 proposta** · 11/11/2026 · código auditado `a83b7fd3d97d5a356e869e172ccaf761aceb150b` · documentação por inspeção estática, sem validação do hardware.

## Finalidade e fronteiras de confiança
Firmware Arduino para ESP32 que aciona relé físico a partir de cartão RFID ou interface administrativa HTTP; armazena lista de usuários/identificadores em SPIFFS. **Não é uma fechadura certificada**, e sua implantação em porta real deve ser precedida por ensaios elétricos, autenticação e segurança física.

## Componentes e pinagem verificada
| Componente | Origem | Pinagem/função |
|---|---|---|
| ESP32 `esp32dev` | `platformio.ini` | Framework Arduino, `SPIFFS`, serial 115200 |
| RFID MFRC522 | `src/main.cpp` | `RST_PIN=22`, `SS_PIN=21` via SPI |
| Relé | `src/main.cpp` | `pinRele=4`, saída ativa em LOW |
| LEDs | `src/main.cpp` | Verde GPIO 13; vermelho GPIO 14 |
| Web admin | `ESPAsyncWebServer` | Servidor porta 80 |
| Bluetooth Classic | `BluetoothSerial` | Interface para comandos; política precisa de revisão |
| Dados persistidos | `SPIFFS` | `/users.json`: objetos `ra`, `name`, `uid` |
| Chaves e Wi-Fi | `include/ModulockSecrets.h` local | Exemplo versionado, segredo real **fora** do Git |

## Cadastro e validação
`loadUsers()` monta SPIFFS, lê JSON e inicializa array vazio quando não há arquivo ou a desserialização falha. `addUser(ra,name,uid)` rejeita RA e UID vazios, proíbe RA/UID duplicados e grava `usersDoc` no arquivo por `saveUsers()`. `removeUser(ra)` exclui por RA e salva. O reconhecimento de cartão compara UID obtido por `getUidHexString()` com `users[*].uid`.

**Risco:** UID de RFID não representa autenticação criptográfica forte; cartões podem ser clonados. A memória JSON, logs seriais e estratégias de restauração devem ser auditados.

## Autenticação e endpoints observados
`isAuthenticated()` verifica `session_id` e compara seu valor com `identificadorSessao` em memória (o teste de presença de cookie foi corrigido anteriormente). Login e painel operam sobre HTTP sem TLS embarcado. O token de sessão não equivale a sistema de identidade robusto e requer política de renovação, expiração e auditoria.

| Método/rota | Ação |
|---|---|
| `GET /` | Página de login |
| `POST /login` | Validar credenciais administrativas |
| `GET /dashboard` | Painel |
| `POST /openDoor` | Abrir por relé após checar sessão |
| `GET /getUsers` | Lista de usuários em JSON; dados sensíveis |
| `GET /register`, `POST /registerUser` | Formulário e cadastro |
| `POST /removeUser` | Remoção de usuário |
| `GET /getLastScannedUid` | UID lido recentemente |
| `GET /style.css`, `GET /script.js` | Arquivos estáticos SPIFFS |

## Máquina física de acionamento
`ativarRelePorta()` ajusta o relé para LOW e define `relayOffTime = millis()+1000`. O loop coloca o relé novamente em HIGH após o prazo e apaga LEDs após 2 segundos. O firmware deve ser testado com relé sem carga de fechadura antes do uso real. O arquivo atual contém comandos Bluetooth incluindo `OPEN DOOR`, que demanda proteção específica para impedir abertura por pareamento ou conexão não autorizada.

## Limitações explícitas
- Bibliotecas de terceiros foram incluídas em `lib/`; não confundir seu código com autoria deste projeto.
- O sistema não possui suíte automatizada de integração ESP32/RFID no repositório.
- Não mostrar credenciais, UID completo ou usuários reais em logs de produção.
- Sem HTTPS e política de proteção da conexão, não expor porta 80 na internet.

## Correção P0 proposta: leitura e cadastro de UID

- `GET /getLastScannedUid` agora exige sessão autenticada (401 quando ausente/inválida), valida TTL de 30s e responde com `Cache-Control: no-store`.
- `POST /registerUser` continua exigindo sessão e também exige que o UID informado seja **igual ao último UID fisicamente lido**, dentro do prazo; UID é consumido após cadastro bem-sucedido.
- Comandos Bluetooth antigos `OPEN DOOR`, `ADD USER`, `REMOVE USER` e `GET USERS` foram **removidos do firmware compilável** por falta de autenticação do canal; Bluetooth já não era inicializado na main.
- A UI de registro mantém polling de 1 segundo, mas o endpoint devolve 401 sem login; JS deve tratar sessão expirada no futuro.
- `include/ModulockAccessPolicy.h` contém a lógica de validade temporal independente do Arduino, testada com `g++` inclusive em overflow do contador; `test/test_security_wiring.py` verifica ligações dos guardas ao firmware.
- Esta revisão **não** torna RFID UID criptograficamente resistente a clones, não adiciona HTTPS, não fecha os riscos de sessão/CSRF, e não foi gravada em hardware.

[Política detalhada](POLITICA_SESSAO_UID.md).
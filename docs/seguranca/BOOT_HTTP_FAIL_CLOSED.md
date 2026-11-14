# MOD-HTTP-GATE — Gate executável de administração HTTP do ESP32

**Revisão crítica:** 14/11/2026.

## Falha confirmada
O firmware previa `MODULOCK_ENABLE_UNENCRYPTED_ADMIN_HTTP=0` por padrão. Entretanto, o `if (!MODULOCK_ENABLE_UNENCRYPTED_ADMIN_HTTP) {...}` estava **dentro do literal C++ `R"rawliteral(...)` que contém HTML/JavaScript** do painel, após `loadUsers()` da página, em vez de `setup()`. A compilação PlatformIO passava porque conteúdo HTML é texto; o teste anterior `assertIn` também passava por verificar apenas substring. Com credenciais presentes, `setup()` ainda alcançava `WiFi.begin()` e `server.begin()`.

## Correção aplicada no código
O bloco inválido foi removido do HTML, restaurando a estrutura da página. A política agora é executada em `setup()` **antes de validar credenciais, iniciar o Wi-Fi ou expor as rotas HTTP**. Sem `MODULOCK_ENABLE_UNENCRYPTED_ADMIN_HTTP=1`, o sistema continua com inicialização RFID, GPIOs e SPIFFS, mas não estabelece Wi-Fi nem inicia o painel. A opção 1 destina-se somente a bancada de teste isolada; não é HTTPS.

## Regressão demonstrável
`tests/test_security_defaults.py` exige a configuração padrão 0, **exatamente uma ocorrência da condição**, ocorrência dentro do corpo de `setup()`, ordem anterior a `WiFi.begin()` e `server.begin()`, presença de `return` e ausência no HTML raw. O CI também compila com PlatformIO ESP32 sem flash; nenhum dispositivo real foi testado.

## Pendências
Sessões e CSRF, autenticação forte de cartões, TLS adequado, controles de tentativas e testes HIL continuam bloqueadores para utilização como fechadura. O gate não substitui testes de placa nem análise de outras superfícies de acesso.

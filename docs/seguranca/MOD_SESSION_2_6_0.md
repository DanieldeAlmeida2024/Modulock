# MOD-SESSION 2.6.0 — Ciclo de vida da sessão HTTP de bancada

14/11/2026 · O painel HTTP permanece DESABILITADO por padrão no firmware. Esta alteração não autoriza uso físico em produção.

## Defeito reproduzível em revisão
O identificador global era emitido em setup() uma única vez e reutilizado em todos os logins válidos, sem prazo de expiração ou rota de logout. Um cookie antigo podia continuar válido até reiniciar o ESP32.

## Controles adicionados
- Gerar um novo identificador de 256 bits com esp_random() somente após comparar credenciais; o login seguinte invalida a sessão anterior.
- Uma única sessão administrativa ativa por vez e validade máxima de 15 minutos com diferença modular de millis() (suporta rollover).
- POST /logout revoga a sessão no servidor e expira o cookie no navegador.
- Cookies HttpOnly, SameSite=Strict, Path=/ e Max-Age=900. Sem token no boot.
- Testes estáticos de posição, rotação, expiração e revogação, além do build PlatformIO existente.

## Risco residual / liberação
HTTP continua sem TLS quando habilitado por macro; a sessão não possui proteção CSRF dedicada, limitação de tentativas, persistência de auditoria nem autenticação criptográfica RFID. O recurso fica restrito a bancada isolada. Testes HIL e revisão do protocolo administrativo continuam obrigatórios antes de qualquer liberação.

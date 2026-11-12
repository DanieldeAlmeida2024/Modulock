# Modulock — Sessões administrativas, expiração e CSRF

**MOD-SESSION v2.2.0 (proposta, 11/11/2026).** Baseada nos PRs #3 e #4. Não é firmware homologado.

## Problema e mudança
O firmware anterior gerava um identificador de sessão no boot, reutilizado por todos os logins, e o logout apagava apenas o cookie local. A versão proposta emite um token aleatório novo de 256 bits a cada login, armazena apenas o SHA-256 dele em RAM e invalida a sessão anterior. **Somente um operador pode estar conectado por vez**, por decisão de escopo desta implementação.

## Contratos observáveis
| Recurso | Comportamento |
|---|---|
| POST /login | Credenciais corretas geram cookie `HttpOnly; SameSite=Strict`, token novo e nonce anti-CSRF. Login posterior revoga o anterior. |
| Sessão | Timeout por 10 min de inatividade ou 30 min absolutos; diferenças `uint32_t` de `millis()` resistem ao rollover. |
| GET /session/csrf | Exige cookie válido e retorna `{csrfToken:...}` com `Cache-Control: no-store`. |
| POST /openDoor, /registerUser, /removeUser | Exigem cookie válido e `X-Modulock-CSRF`; devolvem 401 sem sessão e 403 sem nonce. |
| POST /logout | Cookie e CSRF obrigatórios; limpa estado no servidor e remove cookie com `Max-Age=0`. |
| Concorrência | Mutex FreeRTOS protege estado da sessão entre AsyncTCP e loop; acesso negado se não obtido. |

## Limites de segurança P0 que permanecem
- **A porta 80 continua HTTP sem TLS.** Credenciais, cookies e nonces podem ser capturados ou alterados na rede. Não usar em produção nem ativar `Secure` sem preparar HTTPS real via gateway confiável.
- Não há rate-limit/bloqueio de tentativas de login, CSP/XSS auditado, MFA, gerenciamento de múltiplas contas nem autenticação criptográfica de cartões RFID.
- O cookie é hashado em RAM, mas o nonce CSRF existe em memória; qualquer invasor com acesso ao firmware ou ao tráfego pode comprometer a sessão.
- `GET /getUsers` ainda entrega RA, nome e UID completos a administradores autenticados, e `/users.json` permanece no SPIFFS sem criptografia adicional.
- Autorizar uma requisição e executar GPIO/SPIFFS não forma transação atômica; verificar disputa com logout e comandos simultâneos no HIL.
- O workflow testa política C++ e ligação no código e compila para `esp32dev`, mas **não valida rede física, TLS, sincronização elétrica ou relé**.

## Implantação segura
Não fazer flash em controle físico. Ensaiar em bancada sem fechadura: validar 401/403, novo login derrubando cookie anterior, logout, revogação, CSRF ausente, TTL/overflow, reset e comandos simultâneos. Depois projetar transporte TLS e atributos de cookie Secure com revisão de hardware, rate limiting e anticlone RFID.

[Matriz de testes](../testes/SESSAO_CSRF.md) · [UML](../uml/06_sessao_revogacao_csrf.puml) · [Histórico](../controle/VERSIONAMENTO.md).

**Evidência de software:** [CI #43140037802](https://github.com/DanieldeAlmeida2024/Modulock/actions/runs/43140037802) — testes C++/Python e build ESP32 aprovados no commit `62a1723266000d03b61e7bf5d78326a41a912cb8`. Revalidação de código após esse SHA é obrigatória; HIL/TLS permanecem não executados.

### Atualização de segurança MOD-LOGIN v2.3.0 (proposta)

A frase anterior sobre ausência de rate-limit descreve v2.2.0, não a proposta posterior. Na nova branch, cinco credenciais erradas em 60 s bloqueiam o login por 300 s e há cadência de 2 s. O HTTP administrativo permanece sem TLS, **mas o build padrão agora não o inicia**; apenas `esp32dev-insecure-lab` habilita essa superfície para rede isolada. Bloqueio global ainda admite DoS e reinício limpa a contagem. [Novo contrato](LOGIN_RATE_LIMIT_E_HTTP.md).

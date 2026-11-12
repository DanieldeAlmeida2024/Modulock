# Modulock — Controle de tentativas e bloqueio de HTTP inseguro

**MOD-LOGIN v2.3.0 — proposta, 12/11/2026.** Dependências: PR #3 (UID), #4 (timer) e #6 (sessão/CSRF). Esta versão não é firmware homologado ou apto para portas reais.

## Ameaça original
O painel usa `AsyncWebServer server(80)` com usuário/senha administrativa configurados no firmware e nenhum controle sobre frequência de tentativas. A sessão foi fortalecida na v2.2.0, mas uma senha pode ser submetida repetidamente e **HTTP transporta credenciais, cookie e CSRF em claro**. Não basta adicionar um cookie `Secure` em um firmware que ainda não serve HTTPS.

## Contrato implementado

| ID | Medida | Efeito verificável |
|---|---|---|
| MOD-LOGIN-01 | `ModulockLoginThrottle` sem Arduino | Até **5 falhas por janela de 60 segundos** em todo o dispositivo; a quinta falha ativa bloqueio global por **300 segundos** |
| MOD-LOGIN-02 | Cadência mínima | Aceita no máximo **1 tentativa a cada 2 segundos**; requisições no intervalo são negadas com HTTP 429 |
| MOD-LOGIN-03 | Erro de autenticação | Credenciais inválidas ou parâmetros ausentes contam como falhas; sucesso limpa a contagem e emite nova sessão da v2.2.0 |
| MOD-LOGIN-04 | Bloqueio explícito | Durante cooldown, login retorna HTTP **429**, `Retry-After` em segundos e `Cache-Control: no-store` |
| MOD-LOGIN-05 | Segurança de concorrência | Verificação de tentativa, comparação de credenciais, contagem e criação de sessão são serializadas no mutex `adminSessionMutex` |
| MOD-LOGIN-06 | Arimética temporal | Diferenças `uint32_t(now - referência)` funcionam antes/depois de rollover de `millis()` |
| MOD-LOGIN-07 | Fail-closed HTTP | Sem macro explícita `MODULOCK_ALLOW_INSECURE_HTTP_LAB=1` durante compilação, o `setup()` **não conecta Wi-Fi nem inicia o servidor web** |
| MOD-LOGIN-08 | Estado padrão seguro | Como o `loop()` retorna quando `servidorHabilitado` é falso, **o dispositivo também não processa RFID nem aciona relé** por padrão. As saídas são inicializadas em relé desenergizado e LEDs desligados no boot |

A política é **global**, não por IP, porque `X-Forwarded-For` pode ser falsificado e o ESP32 não tem uma camada de proxies autenticados. Uma série de tentativas maliciosas pode impedir o login do administrador (negação de serviço): proteger rede e gateway continua obrigatório. Bloqueios são mantidos apenas em RAM e reinício do ESP32 limpa o histórico — isso não equivale a proteção persistente contra atacante com acesso físico.

## Uso estritamente experimental
O ambiente PlatformIO adicional `esp32dev-insecure-lab` define a macro de laboratório de modo explícito e claro. **Só compilar/instalar essa variante em bancada isolada, sem fechadura conectada**, e sem conexão à internet/rede compartilhada. O build `esp32dev` continua padrão fail-closed (sem Wi-Fi e sem RFID remoto). O uso do servidor HTTP em laboratório envia credenciais em texto claro na rede local e não deve ser confundido com HTTPS.

Compilação isolada sem flash:

```bash
pio run -e esp32dev
pio run -e esp32dev-insecure-lab
```

Para uma implantação real é obrigatório **redesenhar o canal de administração com TLS fim a fim ou um gateway confiável e segmentação de rede que impeça acesso HTTP direto**, validar cookies `Secure`, autenticação entre gateway e dispositivo, certificados, proteção de login e operações físicas com HIL. Um reverse proxy HTTPS que apenas reencaminha HTTP por uma rede compartilhada **não resolve** o problema.

## Limitações restantes que impedem liberação
- Não há TLS na placa, autenticação de cliente no gateway, MFA, prevenção de DoS distribuído, proteção contra reset intencional nem persistência de contadores entre reinicializações.
- O administrador ainda usa um par de credenciais; não existe identidade individual ou histórico auditável de operador.
- UID de RFID segue potencialmente clonável; arquivo `users.json` em SPIFFS possui nomes, RAs e UIDs em claro. `GET /getUsers` continua retornando identificadores completos para a sessão.
- `ESP32AsyncWebServer`, `AsyncTCP`, `MFRC522` e bibliotecas vendorizadas precisam de revisão de origem/versão, além de hardware e compatibilidade de bibliotecas.
- Não foram executados testes de rede, TLS, relé, brownout, watchdog ou acessibilidade da saída de emergência.

## Rastreabilidade
[Plano MOD-LOGIN](../testes/LOGIN_RATE_LIMIT.md) · [UML de autenticação](../uml/07_login_lockout_lab_gate.puml) · [Versionamento](../controle/VERSIONAMENTO.md).

## Verificação de compilação e limites da evidência

[CI #43215252905](https://github.com/DanieldeAlmeida2024/Modulock/actions/runs/43215252905) e [#43215357873](https://github.com/DanieldeAlmeida2024/Modulock/actions/runs/43215357873) aprovaram C++ host/inspeções Python e builds `esp32dev` e `esp32dev-insecure-lab` no commit `87eb10eb56d204271c35a1b3c812d18c643c7570`. Isso não resolve transporte HTTP inseguro e nem demonstra segurança física; usar apenas como evidência de software. **Sem deploy e sem flash.**

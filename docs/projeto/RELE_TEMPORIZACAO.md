# Modulock — temporização segura do relé e LEDs em ESP32

> **MOD-TIME v2.1.0 proposta, 11/11/2026.** Base: PR #3 (`afa5c6ad`). Código em `src/main.cpp` e `include/ModulockTimingPolicy.h`. Teste HIL **não executado**.

## Defeito identificado
O firmware definia `relayOffTime = millis() + 1000` e posteriormente `millis() >= relayOffTime` para desligar relé. Os dois LEDs usavam a mesma abordagem com 2000 ms. `millis()` no ESP32 usa contador `uint32_t` que transborda aproximadamente a cada 49,7 dias. Ao calcular deadline próximo ao overflow, o deadline pode virar um número pequeno, fazendo o loop terminar a ativação antecipadamente; o sistema não mantinha estado de atividade separado de deadline zero. É comportamento inseguro para uma saída física e requer teste de bancada.

## Nova máquina de estados
- `relayActive` e `relayActivatedAtMs`: marcação explícita de que o relé está energizado. `ativarRelePorta()` grava o instante de acionamento e define GPIO 4 em **LOW**; novo acionamento renova a janela (1.000 ms).
- `greenLedActive/redLedActive` e respectivos timestamps: indicadores em GPIO 13/14 permanecem HIGH por 2.000 ms e voltam a LOW.
- `modulockTimedActionExpired(active,now,activatedAt,duration)`: calcula `uint32_t(now - activatedAt)` e compara com duração, usando aritmética modular. O timestamp zero é válido; estado desativado não gera evento. Duração zero ou acima do intervalo seguro de `2^31` ms resulta em encerramento defensivo da ação.
- O loop desenergiza o relé escrevendo **HIGH** antes de definir `relayActive=false`. GPIO é inicializado em HIGH no boot. Nunca alterar polaridade do hardware sem verificar a placa e o circuito do relé.
- A comparação anterior `millis() >= deadline` não é mais usada para relé e LEDs. O estado é usado somente para temporização e **não altera a autenticação RFID/HTTP**.

## Proteção de dados em logs
`addUser()` imprimia `ra`, `name` e `uid` completos na serial durante a tentativa de cadastro. Isso foi substituído por mensagem genérica de operação. **Não garante que todas as outras rotas, bibliotecas de terceiros ou arquivos SPIFFS estejam livres de PII**; auditar a serial completa e acesso físico ao chip separadamente. UID MIFARE simples permanece clonável e não é fator criptográfico.

## Invariantes e testes
- Testes C++ host (sem placa): dentro da duração, no limite exato, pós-overflow, estado inativo e duração inválida. `test/security_policy_test.cpp` cobre as bordas com `-std=c++11 -Werror`.
- Teste estático `test/test_timing_privacy_wiring.py` verifica que **as três** saídas usam o helper e que os logs de cadastro não imprimem campos sensíveis. Não é teste elétrico.
- CI PlatformIO `pio run -e esp32dev` comprova compilação; apenas HIL pode confirmar polaridade real, acionamento, bounce, condições de carga, watchdog e comportamento em reinicialização.

## Riscos e critérios antes de instalar em fechadura
- **HIL obrigatório sem fechadura energizada**: osciloscópio ou analisador lógico na saída do relé, ativação antes e depois do transbordamento simulado, ativação repetida, reinicialização e perda de energia.
- Acesso à memória compartilhada entre callbacks HTTP assíncronos e loop Arduino ainda requer revisão formal de sincronização. O uso de `bool`/timestamps não implica atomicidade composta.
- O circuito deve oferecer meio de saída mecânica/emergência, proteção elétrica da bobina, isolamento e avaliação de risco de falha segura da porta.
- HTTP sem TLS, sessão global, CSRF/rate-limit, clonagem de RFID e persistência em SPIFFS continuam bloqueadores de uso real. Nenhum flash, deploy ou operação física é autorizado por este PR.

[Plano de testes](../testes/PLANO_DE_TESTES.md) · [UML](../uml/05_temporizadores_rele_wrap.puml) · [Controle de versões](../controle/VERSIONAMENTO.md).

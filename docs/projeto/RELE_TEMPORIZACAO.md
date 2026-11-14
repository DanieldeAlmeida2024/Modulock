# MOD-TIMER — Temporização segura de relé/LEDs

## Problema comprovado
O firmware anterior agendava o desligamento como `millis() + duração` e verificava `millis() >= deadline`. Próximo ao overflow de 32 bits, o prazo pode parecer já alcançado; também `0` era usado como marcador de inatividade, apesar de ser valor legítimo do contador.

## Contrato corrigido
- Gravar `activatedAtMs` como `uint32_t` e manter um booleano `active` separado para cada GPIO.
- Expirar quando `uint32_t(now - start) >= duration`, apenas para duração positiva inferior à metade do ciclo.
- Reativações renovam o instante inicial. Relé energizado em LOW e desenergizado em HIGH; LED LOW desliga.
- Loop deve ser executado regularmente; atrasos de CPU não são eliminados pelo algoritmo.

## Evidência
`test/test_timers.cpp` exercita instantes normais, fronteira exata, wrap em `0xFFFFFFFE`, duração inválida e canal inativo. `test/test_timing_wiring.py` garante ligação de relé e dois LEDs à política temporal. O CI compila ESP32 sem gravar placa e executa testes host.

## Não homologado
Exige ensaio HIL em bancada desacoplada de fechadura, GPIO e relé medidos, operação repetida, reinicialização, brownout, watchdog e resposta após 49,7 dias simulados. Nenhum dispositivo físico foi atualizado.

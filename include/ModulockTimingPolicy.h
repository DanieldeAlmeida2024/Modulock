#pragma once
#include <stdint.h>

// Tempo modular de 32 bits; usa um marcador de atividade separado do timestamp.
// Duração inválida encerra a ação por segurança. Verificar o loop regularmente.
inline bool modulockTimedActionExpired(
    bool active, uint32_t nowMs, uint32_t activatedAtMs, uint32_t durationMs) {
  if (!active) return false;
  if (durationMs == 0u || durationMs >= 0x80000000u) return true;
  return static_cast<uint32_t>(nowMs - activatedAtMs) >= durationMs;
}

#pragma once
#include <stdint.h>

// Delta modular em contador unsigned de 32 bits. Requer verificação de
// durações menores que meia janela para distinguir expiração de wrap.
inline bool modulockTimedActionExpired(
    bool active, uint32_t nowMs, uint32_t activatedAtMs, uint32_t durationMs) {
  if (!active) return false;
  if (durationMs == 0u || durationMs >= 0x80000000u) return true;
  return static_cast<uint32_t>(nowMs - activatedAtMs) >= durationMs;
}

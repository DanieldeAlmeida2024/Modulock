#pragma once

// Regras de leitura/cadastro isoladas do Arduino para testes host-side.
// uint32_t garante que a subtração modular funciona com overflow de millis()
// no ESP32 (exige timeout bem menor que 2^31 ms).
#include <stdint.h>
#include <string.h>

inline bool modulockUidRecentlyScanned(
    bool uidAvailable, uint32_t now, uint32_t lastScanAt, uint32_t ttlMs) {
  return uidAvailable && ttlMs > 0 && ttlMs < 0x80000000u &&
         static_cast<uint32_t>(now - lastScanAt) < ttlMs;
}

inline bool modulockUidRegistrationAllowed(
    bool authenticated, const char* requestedUid, const char* lastScannedUid,
    uint32_t now, uint32_t lastScanAt, uint32_t ttlMs) {
  return authenticated && requestedUid != nullptr && lastScannedUid != nullptr &&
         lastScannedUid[0] != '\0' && requestedUid[0] != '\0' &&
         modulockUidRecentlyScanned(true, now, lastScanAt, ttlMs) &&
         strcmp(requestedUid, lastScannedUid) == 0;
}

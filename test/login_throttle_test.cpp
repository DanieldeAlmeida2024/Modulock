#include <cassert>
#include <cstdint>
#include <iostream>
#include "ModulockLoginThrottle.h"

int main() {
  ModulockLoginThrottle p;
  // Primeiro login permitido até quando millis() == 0.
  assert(p.allowAttempt(0));
  p.failed(0);
  assert(!p.allowAttempt(0));
  assert(p.retryAfterSeconds(0) == 2);
  assert(!p.allowAttempt(1999));
  assert(p.retryAfterSeconds(1999) == 1);
  assert(p.allowAttempt(2000)); p.failed(2000);
  assert(p.allowAttempt(4000)); p.failed(4000);
  assert(p.allowAttempt(6000)); p.failed(6000);
  assert(p.allowAttempt(8000)); p.failed(8000);
  assert(p.isLocked(8000));
  assert(!p.allowAttempt(10000));
  assert(p.retryAfterSeconds(8000) == 300);
  assert(!p.allowAttempt(8000 + MODULOCK_LOGIN_LOCK_MS - 1));
  assert(!p.isLocked(8000 + MODULOCK_LOGIN_LOCK_MS)); // lock expira exatamente
  assert(p.allowAttempt(8000 + MODULOCK_LOGIN_LOCK_MS));
  p.failed(8000 + MODULOCK_LOGIN_LOCK_MS);
  p.succeeded();
  assert(p.allowAttempt(8000 + MODULOCK_LOGIN_LOCK_MS)); // sucesso zera histórico
  p.failed(8000 + MODULOCK_LOGIN_LOCK_MS);
  // Janela de 60 s reseta contagem, mesmo com falhas antigas.
  assert(p.allowAttempt(8000 + MODULOCK_LOGIN_LOCK_MS + MODULOCK_LOGIN_WINDOW_MS + 1));
  p.failed(8000 + MODULOCK_LOGIN_LOCK_MS + MODULOCK_LOGIN_WINDOW_MS + 1);
  assert(!p.isLocked(8000 + MODULOCK_LOGIN_LOCK_MS + MODULOCK_LOGIN_WINDOW_MS + 1));

  ModulockLoginThrottle wrap;
  uint32_t started = 0xFFFFFFF0u;
  assert(wrap.allowAttempt(started)); wrap.failed(started);
  for (uint32_t i=1; i<=4; ++i) {
    const uint32_t now = static_cast<uint32_t>(started + i*MODULOCK_LOGIN_INTERVAL_MS);
    assert(wrap.allowAttempt(now));
    wrap.failed(now);
  }
  const uint32_t lockStart = static_cast<uint32_t>(started + 4u*MODULOCK_LOGIN_INTERVAL_MS);
  assert(wrap.isLocked(lockStart));
  assert(!wrap.allowAttempt(static_cast<uint32_t>(lockStart + MODULOCK_LOGIN_LOCK_MS - 1u)));
  assert(wrap.allowAttempt(static_cast<uint32_t>(lockStart + MODULOCK_LOGIN_LOCK_MS)));
  assert(wrap.retryAfterSeconds(static_cast<uint32_t>(lockStart + MODULOCK_LOGIN_LOCK_MS)) == 2);
  std::cout << "PASS: 5 failures/60s, 2s minimum, 300s lockout, recovery, millis wrap\n";
}

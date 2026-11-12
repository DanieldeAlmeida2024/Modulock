#pragma once
#include <stdint.h>

// MOD-AUTH v2.3.0 — limite global de tentativas por dispositivo, independente de IP.
// Estado protegido pelo mesmo mutex FreeRTOS usado para sessão administrativa.
// Uma rajada distribuída pode negar acesso ao painel: mitigação exige gateway TLS
// confiável com controle adicional de origem / rate limit e testes em bancada.
static const uint32_t MODULOCK_LOGIN_INTERVAL_MS = 2000u;
static const uint32_t MODULOCK_LOGIN_WINDOW_MS = 60000u;
static const uint32_t MODULOCK_LOGIN_LOCK_MS = 300000u;
static const uint8_t MODULOCK_LOGIN_MAX_FAILURES = 5u;

class ModulockLoginThrottle {
 public:
  ModulockLoginThrottle()
      : attemptsWindow_(false), hasLastAttempt_(false), locked_(false),
        failedCount_(0), windowAt_(0u), lastAttemptAt_(0u), lockedAt_(0u) {}

  bool allowAttempt(uint32_t now) {
    if (locked_) {
      if (static_cast<uint32_t>(now - lockedAt_) < MODULOCK_LOGIN_LOCK_MS)
        return false;
      // O lockout não depende de reset nem do valor absoluto do contador.
      locked_ = false;
      failedCount_ = 0;
      attemptsWindow_ = false;
      hasLastAttempt_ = false;
    }
    if (!attemptsWindow_ ||
        static_cast<uint32_t>(now - windowAt_) >= MODULOCK_LOGIN_WINDOW_MS) {
      attemptsWindow_ = true;
      windowAt_ = now;
      failedCount_ = 0;
    }
    if (hasLastAttempt_ &&
        static_cast<uint32_t>(now - lastAttemptAt_) < MODULOCK_LOGIN_INTERVAL_MS)
      return false;
    hasLastAttempt_ = true;
    lastAttemptAt_ = now;
    return true;
  }

  void failed(uint32_t now) {
    if (failedCount_ < MODULOCK_LOGIN_MAX_FAILURES) ++failedCount_;
    if (failedCount_ >= MODULOCK_LOGIN_MAX_FAILURES) {
      locked_ = true;
      lockedAt_ = now;
    }
  }

  void succeeded() {
    failedCount_ = 0;
    attemptsWindow_ = false;
    hasLastAttempt_ = false;
  }

  uint32_t retryAfterSeconds(uint32_t now) const {
    uint32_t remaining = 0u;
    if (locked_) {
      const uint32_t elapsed = static_cast<uint32_t>(now - lockedAt_);
      if (elapsed < MODULOCK_LOGIN_LOCK_MS)
        remaining = MODULOCK_LOGIN_LOCK_MS - elapsed;
    }
    if (!remaining && hasLastAttempt_) {
      const uint32_t elapsed = static_cast<uint32_t>(now - lastAttemptAt_);
      if (elapsed < MODULOCK_LOGIN_INTERVAL_MS)
        remaining = MODULOCK_LOGIN_INTERVAL_MS - elapsed;
    }
    return remaining ? (remaining + 999u) / 1000u : 1u;
  }

  bool isLocked(uint32_t now) const {
    return locked_ && static_cast<uint32_t>(now - lockedAt_) < MODULOCK_LOGIN_LOCK_MS;
  }

 private:
  bool attemptsWindow_, hasLastAttempt_, locked_;
  uint8_t failedCount_;
  uint32_t windowAt_, lastAttemptAt_, lockedAt_;
};

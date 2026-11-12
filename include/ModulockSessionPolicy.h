#pragma once
// Sessão única por operador: novo login revoga o anterior. Política testável
// sem Arduino; firmware persiste apenas hash SHA-256 do cookie e protege
// todas as operações de estado com mutex FreeRTOS.
#include <stdint.h>
#include <stddef.h>
#include <string.h>
static const uint32_t MODULOCK_SESSION_IDLE_MS = 600000u;
static const uint32_t MODULOCK_SESSION_ABSOLUTE_MS = 1800000u;
inline bool modulockDigestEqual(const uint8_t *a, const uint8_t *b) {
  if (!a || !b) return false;
  uint8_t diff = 0;
  for (size_t i=0; i<32; ++i) diff |= static_cast<uint8_t>(a[i] ^ b[i]);
  return diff == 0;
}
inline bool modulockCsrfEqual(const char *a, const char *b) {
  if (!a || !b || strlen(a) != 64 || strlen(b) != 64) return false;
  uint8_t diff = 0;
  for (size_t i=0; i<64; ++i) diff |= static_cast<uint8_t>(a[i] ^ b[i]);
  return diff == 0;
}
class ModulockAdminSession {
 public:
  ModulockAdminSession() { clear(); }
  void clear() {
    memset(digest_,0,sizeof(digest_)); memset(csrf_,0,sizeof(csrf_));
    issuedAt_=0; lastSeenAt_=0; active_=false;
  }
  bool issue(const uint8_t *digest, const char *csrf, uint32_t now) {
    if (!digest || !csrf || strlen(csrf)!=64) return false;
    clear(); // não reutilizar cookie de outro login
    memcpy(digest_,digest,32); memcpy(csrf_,csrf,65);
    issuedAt_=now; lastSeenAt_=now; active_=true; return true;
  }
  void expire(uint32_t now) {
    if (active_ &&
       (static_cast<uint32_t>(now-issuedAt_) >= MODULOCK_SESSION_ABSOLUTE_MS ||
        static_cast<uint32_t>(now-lastSeenAt_) >= MODULOCK_SESSION_IDLE_MS)) clear();
  }
  bool valid(const uint8_t *digest,uint32_t now) {
    expire(now);
    if (!active_ || !modulockDigestEqual(digest_,digest)) return false;
    lastSeenAt_=now; return true;
  }
  bool csrfFor(const uint8_t *digest,uint32_t now,char out[65]) {
    if (!out || !valid(digest,now)) return false;
    memcpy(out,csrf_,65); return true;
  }
  bool mutationAllowed(const uint8_t *digest,const char *csrf,uint32_t now) {
    expire(now);
    if (!active_ || !modulockDigestEqual(digest_,digest) ||
        !modulockCsrfEqual(csrf_,csrf)) return false;
    lastSeenAt_=now; return true;
  }
  bool revoke(const uint8_t *digest,const char *csrf,uint32_t now) {
    if (!mutationAllowed(digest,csrf,now)) return false;
    clear(); return true;
  }
 private:
  uint8_t digest_[32];
  char csrf_[65];
  uint32_t issuedAt_,lastSeenAt_;
  bool active_;
};

#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include "ModulockSessionPolicy.h"
int main() {
  uint8_t a[32],b[32]; memset(a,0x11,32); memset(b,0x22,32);
  const char* csrf="abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789";
  const char* other="0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
  ModulockAdminSession store;
  assert(!store.valid(a,0));
  assert(!store.issue(nullptr,csrf,0));
  assert(store.issue(a,csrf,0));
  assert(store.valid(a,1) && !store.valid(b,2));
  assert(!store.mutationAllowed(a,nullptr,2));
  assert(!store.mutationAllowed(a,other,2));
  assert(store.mutationAllowed(a,csrf,3));
  char token[65]={0};
  assert(store.csrfFor(a,4,token) && strcmp(token,csrf)==0);
  assert(!store.revoke(b,csrf,5) && !store.revoke(a,other,5));
  assert(store.revoke(a,csrf,5) && !store.valid(a,6));
  assert(store.issue(a,csrf,100));
  assert(store.issue(b,other,101) && !store.valid(a,102));
  assert(store.valid(b,102));
  store.expire(102 + MODULOCK_SESSION_IDLE_MS);
  assert(!store.valid(b,103+MODULOCK_SESSION_IDLE_MS));
  assert(store.issue(a,csrf,1000));
  for(uint32_t t=1000;t<1000+MODULOCK_SESSION_ABSOLUTE_MS;t+=500000u)assert(store.valid(a,t));
  store.expire(1000+MODULOCK_SESSION_ABSOLUTE_MS);
  assert(!store.valid(a,1001+MODULOCK_SESSION_ABSOLUTE_MS));
  assert(store.issue(a,csrf,0xFFFFFFF0u));
  assert(store.valid(a,4));
  // A chamada valid(a,4) atualizou lastSeenAt_ após o wrap.
  store.expire(static_cast<uint32_t>(4u+MODULOCK_SESSION_IDLE_MS));
  assert(!store.valid(a,static_cast<uint32_t>(4u+MODULOCK_SESSION_IDLE_MS)));
  assert(!modulockCsrfEqual(csrf,"short"));
  assert(!modulockDigestEqual(a,b));
  std::cout<<"PASS: rotating session, logout, CSRF, TTL and uint32 wrap\n";
}

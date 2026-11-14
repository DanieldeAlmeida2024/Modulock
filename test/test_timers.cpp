#include <cassert>
#include <cstdint>
#include "ModulockTimingPolicy.h"
int main() {
  assert(!modulockTimedActionExpired(false, 3000u, 1000u, 1000u));
  assert(!modulockTimedActionExpired(true, 1999u, 1000u, 1000u));
  assert(modulockTimedActionExpired(true, 2000u, 1000u, 1000u));
  assert(!modulockTimedActionExpired(true, 0u, 0xFFFFFFFEu, 1000u));
  assert(!modulockTimedActionExpired(true, 997u, 0xFFFFFFFEu, 1000u));
  assert(modulockTimedActionExpired(true, 998u, 0xFFFFFFFEu, 1000u));
  assert(modulockTimedActionExpired(true, 3u, 0xFFFFFFFEu, 5u));
  assert(modulockTimedActionExpired(true, 4u, 0u, 0u));
  assert(modulockTimedActionExpired(true, 4u, 0u, 0x80000000u));
  assert(!modulockTimedActionExpired(false, 4u, 0u, 0u));
  return 0;
}

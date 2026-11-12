#include <cassert>
#include <cstdint>
#include <iostream>
#include "ModulockAccessPolicy.h"
#include "ModulockTimingPolicy.h"

int main() {
    assert(modulockUidRecentlyScanned(true, 1050u, 1000u, 30000u));
    assert(!modulockUidRecentlyScanned(false, 1050u, 1000u, 30000u));
    assert(!modulockUidRecentlyScanned(true, 31000u, 1000u, 30000u));
    assert(modulockUidRecentlyScanned(true, 2u, 0xFFFFFFFEu, 30000u));
    assert(!modulockUidRecentlyScanned(true, 30005u, 0xFFFFFFFEu, 30000u));
    assert(!modulockUidRecentlyScanned(true, 1050u, 1000u, 0u));

    assert(modulockUidRegistrationAllowed(
        true, "A1B2C3D4", "A1B2C3D4", 1050u, 1000u, 30000u));
    assert(!modulockUidRegistrationAllowed(
        false, "A1B2C3D4", "A1B2C3D4", 1050u, 1000u, 30000u));
    assert(!modulockUidRegistrationAllowed(
        true, "FFFFFFFF", "A1B2C3D4", 1050u, 1000u, 30000u));
    assert(!modulockUidRegistrationAllowed(
        true, "A1B2C3D4", "A1B2C3D4", 31000u, 1000u, 30000u));
    assert(!modulockUidRegistrationAllowed(
        true, "", "", 1050u, 1000u, 30000u));
    assert(!modulockUidRegistrationAllowed(
        true, nullptr, "A1B2C3D4", 1050u, 1000u, 30000u));
    // 1000ms do relé e 2000ms do LED, inclusive após overflow do contador millis.
    assert(!modulockTimedActionExpired(false, 3000u, 1000u, 1000u));
    assert(!modulockTimedActionExpired(true, 1999u, 1000u, 1000u));
    assert(modulockTimedActionExpired(true, 2000u, 1000u, 1000u));
    assert(!modulockTimedActionExpired(true, 0u, 0xFFFFFFFEu, 1000u));
    assert(!modulockTimedActionExpired(true, 997u, 0xFFFFFFFEu, 1000u));
    assert(modulockTimedActionExpired(true, 998u, 0xFFFFFFFEu, 1000u));
    assert(!modulockTimedActionExpired(true, 1u, 0xFFFFFFFEu, 5u));
    assert(modulockTimedActionExpired(true, 3u, 0xFFFFFFFEu, 5u));
    assert(modulockTimedActionExpired(true, 4u, 0u, 0u)); // duração inválida encerra
    assert(modulockTimedActionExpired(true, 4u, 0u, 0x80000000u));
    assert(!modulockTimedActionExpired(false, 4u, 0u, 0u));
    std::cout << "PASS: UID TTL, relay/LED elapsed timer, millis rollover\n";
}

#include <cassert>
#include <cstdint>
#include <iostream>
#include "ModulockAccessPolicy.h"

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
    std::cout << "PASS: TTL/overflow/UID match/authorization guards\n";
}

from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
FIRMWARE = (ROOT / "src/main.cpp").read_text(encoding="utf-8")
HEADER = (ROOT / "include/ModulockTimingPolicy.h").read_text(encoding="utf-8")

class TimingAndPrivacyWiring(unittest.TestCase):
    def test_modular_timing_used_for_all_three_outputs(self):
        self.assertEqual(FIRMWARE.count("modulockTimedActionExpired("), 3)
        for marker in ("relayActive", "greenLedActive", "redLedActive",
                       "relayActivatedAtMs", "greenLedActivatedAtMs", "redLedActivatedAtMs"):
            self.assertIn(marker, FIRMWARE)
        self.assertIn("digitalWrite(pinRele, HIGH); // HIGH = relé desenergizado", FIRMWARE)
        self.assertIn("static_cast<uint32_t>(nowMs - activatedAtMs)", HEADER)
    def test_no_absolute_deadline_comparison(self):
        for marker in ("relayOffTime", "greenLedOffTime", "redLedOffTime", "millis() >="):
            self.assertNotIn(marker, FIRMWARE)
    def test_no_rfid_identity_in_registration_serial_trace(self):
        for marker in ('Serial.print(ra)', 'Serial.print(name)', 'Serial.println(uid)',
                       'Serial.print("Tentando adicionar usuário: RA=")'):
            self.assertNotIn(marker, FIRMWARE)

if __name__ == "__main__":
    unittest.main()

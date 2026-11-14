"""Garante que as três saídas usem a política temporal modular real."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
CODE = (ROOT / "src/main.cpp").read_text(encoding="utf-8")
HEADER = (ROOT / "include/ModulockTimingPolicy.h").read_text(encoding="utf-8")

class TimerWiring(unittest.TestCase):
    def test_tres_canais_usam_delta_modular(self):
        self.assertEqual(3, CODE.count("modulockTimedActionExpired("))
        for marker in ("relayActive", "greenLedActive", "redLedActive",
                       "relayActivatedAtMs", "greenLedActivatedAtMs", "redLedActivatedAtMs"):
            self.assertIn(marker, CODE)
        self.assertIn("static_cast<uint32_t>(nowMs - activatedAtMs)", HEADER)

    def test_prazos_absolutos_inseguros_removidos(self):
        for marker in ("relayOffTime", "greenLedOffTime", "redLedOffTime", "millis() >="):
            self.assertNotIn(marker, CODE)

    def test_desenergiza_rele_apos_expiracao(self):
        self.assertIn("digitalWrite(pinRele, HIGH); // HIGH = relé desenergizado", CODE)

if __name__ == "__main__":
    unittest.main()

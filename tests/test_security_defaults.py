"""Regressões estáticas de segurança; não substituem compilação nem ensaio no ESP32."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = (ROOT / "src/main.cpp").read_text(encoding="utf-8")
EXAMPLE = (ROOT / "include/ModulockSecrets.example.h").read_text(encoding="utf-8")


class TestesSegurancaPadrao(unittest.TestCase):
    def test_http_administrativo_desativado_por_padrao(self):
        self.assertIn("#define MODULOCK_ENABLE_UNENCRYPTED_ADMIN_HTTP 0", SOURCE)
        gate = "if (!MODULOCK_ENABLE_UNENCRYPTED_ADMIN_HTTP)"
        self.assertEqual(1, SOURCE.count(gate), "Gate nao pode existir no HTML embutido")
        setup = SOURCE.split("void setup() {", 1)[1].split("void loop() {", 1)[0]
        self.assertIn(gate, setup, "Gate precisa executar no setup C++")
        self.assertLess(setup.index(gate), setup.index("WiFi.begin("))
        self.assertLess(setup.index(gate), setup.index('server.begin()'))
        self.assertIn('return;', setup[setup.index(gate):setup.index("WiFi.begin(")])
        # A proteção incorreta ficava numa string RAW de HTML/JavaScript.
        html = SOURCE.split('R"rawliteral(', 1)[1].split(')rawliteral"', 1)[0]
        self.assertNotIn(gate, html)


    def test_admin_session_is_issued_only_after_valid_login(self):
        login = SOURCE.split("void handleLogin(AsyncWebServerRequest *request) {", 1)[1].split("void handleLogout(", 1)[0]
        self.assertIn("username == WEB_USERNAME && password == WEB_PASSWORD", login)
        self.assertIn("iniciarSessaoAdministrativa();", login)
        self.assertGreater(login.index("iniciarSessaoAdministrativa();"),
                           login.index("username == WEB_USERNAME && password == WEB_PASSWORD"))
        setup = SOURCE.split("void setup() {", 1)[1].split("void loop() {", 1)[0]
        self.assertNotIn("esp_random()", setup)
        self.assertIn("Max-Age=900", login)

    def test_expiration_and_logout_revoke_server_token(self):
        session = SOURCE.split("bool isAuthenticated(AsyncWebServerRequest *request) {", 1)[1].split("void handleRoot(", 1)[0]
        self.assertIn("sessaoAtiva", session)
        self.assertIn("millis() - sessaoEmitidaEmMs", session)
        self.assertIn("SESSAO_TTL_MS", session)
        logout = SOURCE.split("void handleLogout(AsyncWebServerRequest *request) {", 1)[1].split("void handleDashboard(", 1)[0]
        self.assertIn("sessaoAtiva = false;", logout)
        self.assertIn('identificadorSessao = "";', logout)
        self.assertIn("Max-Age=0", logout)
        self.assertIn('server.on("/logout", HTTP_POST, handleLogout);', SOURCE)

    def test_bluetooth_sem_comandos_de_abertura(self):
        self.assertNotIn("BluetoothSerial", SOURCE)
        self.assertNotIn("processarComandoBluetooth", SOURCE)
        self.assertNotIn("OPEN DOOR", SOURCE)

    def test_rfid_funciona_sem_rede(self):
        self.assertIn("rfid.PICC_IsNewCardPresent()", SOURCE)
        self.assertNotIn("if (!servidorHabilitado) { delay(1000); return; }", SOURCE)

    def test_nao_formatar_dados_em_erro_de_montagem(self):
        self.assertNotIn("SPIFFS.begin(true)", SOURCE)

    def test_exemplo_nao_tem_wifi_configurado(self):
        self.assertIn('MODULOCK_WIFI_SSID ""', EXAMPLE)
        self.assertIn('MODULOCK_WEB_PASSWORD ""', EXAMPLE)


if __name__ == "__main__":
    unittest.main()

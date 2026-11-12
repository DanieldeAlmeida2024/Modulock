from pathlib import Path
import unittest

fw = Path("src/main.cpp").read_text(encoding="utf-8")
js = Path("data/script.js").read_text(encoding="utf-8")

def body(name):
    needle = "void " + name + "(AsyncWebServerRequest* request) {"
    if needle not in fw:
        needle = "void " + name + "(AsyncWebServerRequest *request) {"
    pos = fw.index(needle)
    end = fw.find("\nvoid ", pos + len(needle))
    return fw[pos:end if end >= 0 else len(fw)]

class SessionWiring(unittest.TestCase):
    def test_no_boot_shared_cookie(self):
        self.assertNotIn("identificadorSessao", fw)
        self.assertIn("adminSession.issue(", body("handleLogin"))
        self.assertIn("mbedtls_sha256_ret(", fw)
        self.assertIn("xSemaphoreCreateMutex()", fw)
        self.assertIn("adminSession.expire(now)", fw)
    def test_logout_revokes_server_session(self):
        self.assertIn("modulockSessionCheck(request, true, true)", body("handleLogout"))
        self.assertIn("Max-Age=0", body("handleLogout"))
        self.assertIn('server.on("/logout", HTTP_POST, handleLogout)', fw)
        self.assertNotIn('document.cookie = "session_id=', fw)
    def test_protect_mutations_before_effects(self):
        for name, effect in [("handleAbrirPorta", "ativarRelePorta()"),
                             ("handleRegisterUser", "addUser("),
                             ("handleRemoveUser", "removeUser(")]:
            chunk = body(name)
            self.assertIn("hasValidCsrf(request)", chunk)
            self.assertLess(chunk.index("hasValidCsrf(request)"), chunk.index(effect))
        self.assertIn('server.on("/session/csrf", HTTP_GET, handleSessionCsrf)', fw)
        self.assertIn("csrfHeaders()", fw)
        self.assertIn("modulockCsrfHeaders()", js)
    def test_registration_log_no_identity(self):
        self.assertNotIn('Serial.println("Usuário registrado: " + name', fw)
        self.assertNotIn('Serial.println(uid)', fw)

if __name__ == "__main__":
    unittest.main()

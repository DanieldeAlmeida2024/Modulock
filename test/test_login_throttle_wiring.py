from pathlib import Path
import unittest

src = Path("src/main.cpp").read_text(encoding="utf-8")
policy = Path("include/ModulockLoginThrottle.h").read_text(encoding="utf-8")

def region(start, end):
    a = src.index(start)
    b = src.index(end, a + len(start))
    return src[a:b]

class LoginThrottleWiring(unittest.TestCase):
    def test_fail_closed_http_lab_gate(self):
        setup = region("void setup() {", "void loop() {")
        self.assertIn("#if !defined(MODULOCK_ALLOW_INSECURE_HTTP_LAB)", setup)
        self.assertIn("MODULOCK_ALLOW_INSECURE_HTTP_LAB != 1", setup)
        self.assertLess(setup.index("MODULOCK_ALLOW_INSECURE_HTTP_LAB"), setup.index("WiFi.begin("))
        self.assertLess(setup.index("return; // loop() mantém saídas desligadas"), setup.index("server.begin();"))
        loop = src[src.index("void loop() {"):]
        self.assertIn("if (!servidorHabilitado) { delay(1000); return; }", loop)
    def test_global_throttle_before_credentials_and_session(self):
        login = region("void handleLogin(AsyncWebServerRequest *request) {", "void handleDashboard(")
        self.assertLess(login.index("adminLoginThrottle.allowAttempt(now)"),login.index('request->hasParam("password"'))
        self.assertLess(login.index("adminLoginThrottle.allowAttempt(now)"),login.index("adminSession.issue("))
        self.assertIn("adminLoginThrottle.failed(now);",login)
        self.assertIn("adminLoginThrottle.succeeded();",login)
        self.assertIn("beginResponse(\n        429",login)
        self.assertIn('response->addHeader("Retry-After"',login)
        self.assertIn("xSemaphoreTake(adminSessionMutex",login)
    def test_wrap_safe_policy_limits(self):
        for term in ["MODULOCK_LOGIN_MAX_FAILURES = 5u",
                     "MODULOCK_LOGIN_WINDOW_MS = 60000u",
                     "MODULOCK_LOGIN_LOCK_MS = 300000u",
                     "MODULOCK_LOGIN_INTERVAL_MS = 2000u",
                     "static_cast<uint32_t>(now - lockedAt_)"]:
            self.assertIn(term,policy)
        self.assertNotIn("request->client()->remoteIP()",src)

if __name__ == "__main__":
    unittest.main()

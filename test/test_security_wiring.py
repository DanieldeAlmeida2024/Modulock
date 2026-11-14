from pathlib import Path

src = Path("src/main.cpp").read_text(encoding="utf-8")

def function_body(name):
    needle = f"void {name}(AsyncWebServerRequest *request) {{"
    start = src.index(needle)
    # Essas duas rotas não têm literais raw HTML; contar chaves é suficiente.
    open_at = src.index("{", start)
    level = 0
    for i in range(open_at, len(src)):
        if src[i] == "{":
            level += 1
        elif src[i] == "}":
            level -= 1
            if level == 0:
                return src[open_at:i + 1]
    raise AssertionError("função incompleta: " + name)

uid = function_body("handleGetLastScannedUid")
register = function_body("handleRegisterUser")
assert "if (!isAuthenticated(request))" in uid, "GET do UID sem verificação de sessão"
assert "request->send(401" in uid, "GET do UID precisa recusar visitantes"
assert uid.index("if (!isAuthenticated(request))") < uid.index("lastScannedUidForRegistration"), "UID consultado antes do guarda"
assert "no-store" in uid, "UID não pode ser cacheado"
assert "modulockUidRecentlyScanned(" in uid, "validar TTL do UID"
assert "if (!isAuthenticated(request))" in register
assert "modulockUidRegistrationAllowed(" in register, "POST não deve aceitar UID arbitrário"
assert register.index("modulockUidRegistrationAllowed(") < register.index("addUser(")
assert 'lastScannedUidForRegistration = "";' in register, "UID deve ser consumido após cadastro"
assert "processarComandoBluetooth(" not in src
assert "SerialBT.available()" not in src
print("PASS: HTTP security guards and inactive Bluetooth command surface")

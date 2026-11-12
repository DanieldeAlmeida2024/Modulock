#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "ModulockAccessPolicy.h"
#include "ModulockTimingPolicy.h"
#include "ModulockSessionPolicy.h"
#include <mbedtls/sha256.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ArduinoJson.h>
#include <FS.h>      
#include <SPIFFS.h>
#include <esp_system.h>
#include <cstring>
#if __has_include("ModulockSecrets.h")
#include "ModulockSecrets.h"
#else
#include "ModulockSecrets.example.h"
#endif

#define RST_PIN 22   
#define SS_PIN 21   
#define pinRele 4    
#define ledVerde 13   
#define ledVermelho 14   

const char* WEB_USERNAME = MODULOCK_WEB_USERNAME;
const char* WEB_PASSWORD = MODULOCK_WEB_PASSWORD;
const char* USERS_FILE_PATH = "/users.json";
const char* WIFI_SSID = MODULOCK_WIFI_SSID;
const char* WIFI_PASSWORD = MODULOCK_WIFI_PASSWORD;

ModulockAdminSession adminSession;
SemaphoreHandle_t adminSessionMutex = nullptr;
bool servidorHabilitado = false;

const unsigned long DOOR_OPEN_DURATION_MS = 1000; 
const unsigned long LED_FLASH_DURATION_MS = 2000; 
const unsigned long UID_SCAN_TIMEOUT_MS = 30000; 
MFRC522 rfid(SS_PIN, RST_PIN);
AsyncWebServer server(80);

DynamicJsonDocument usersDoc(8192);

String lastScannedUidForRegistration = "";
unsigned long lastUidScanTime = 0;
uint32_t relayActivatedAtMs = 0;
uint32_t greenLedActivatedAtMs = 0;
uint32_t redLedActivatedAtMs = 0;
bool relayActive = false;
bool greenLedActive = false;
bool redLedActive = false;

void loadUsers();
void saveUsers();
bool addUser(const String& ra, const String& name, const String& uid);
bool removeUser(const String& ra);
bool isAuthorized(byte *uid);
String findUserByUid(byte *uid);
String findUserByRa(const String& ra);

bool isAuthenticated(AsyncWebServerRequest *request);
bool hasValidCsrf(AsyncWebServerRequest *request);
void handleSessionCsrf(AsyncWebServerRequest *request);
void handleLogout(AsyncWebServerRequest *request);
void handleRoot(AsyncWebServerRequest *request);
void handleLogin(AsyncWebServerRequest *request);
void handleDashboard(AsyncWebServerRequest *request); 
void handleAbrirPorta(AsyncWebServerRequest *request);
void handleNotFound(AsyncWebServerRequest *request);
void handleGetUsuarios(AsyncWebServerRequest *request);

void handlePaginaRegistro(AsyncWebServerRequest *request);
void handleRegisterUser(AsyncWebServerRequest *request);
void handleRemoveUser(AsyncWebServerRequest *request);

void handleGetLastScannedUid(AsyncWebServerRequest *request);

void ativarRelePorta();
void flashLED(int pin);
String getUidHexString(byte *uidBytes, byte uidSize);


void loadUsers() {
  if (!SPIFFS.begin(false)) { 
    Serial.println("Erro ao montar o SPIFFS para carregar usuários!");
    return;
  }

  File file = SPIFFS.open(USERS_FILE_PATH, "r");
  if (!file) {
    Serial.println("Arquivo users.json não encontrado. Criando JSON vazio.");
    usersDoc.clear();
    usersDoc["users"].to<JsonArray>();
    return;
  }

  DeserializationError error = deserializeJson(usersDoc, file);
  if (error) {
    Serial.print(F("deserializeJson() falhou: "));
    Serial.println(error.f_str());
    usersDoc.clear(); 
    usersDoc["users"].to<JsonArray>(); 
  } 
  file.close();
}

void saveUsers() {
  if (!SPIFFS.begin(false)) { 
    return;
  }

  File file = SPIFFS.open(USERS_FILE_PATH, "w"); 
  if (!file) {
    Serial.println("Falha ao abrir users.json para escrita!");
    return;
  }

  if (serializeJson(usersDoc, file) == 0) { 
    Serial.println("Falha ao escrever no users.json!");
  } else {
    Serial.println("Usuários salvos com sucesso no users.json.");
  }
  file.close();
}

String getUidHexString(byte *uidBytes, byte uidSize) {
  String uidHex = "";
  for (byte i = 0; i < uidSize; i++) {
    if (uidBytes[i] < 0x10) uidHex += "0";
    uidHex += String(uidBytes[i], HEX);
  }
  uidHex.toUpperCase();
  return uidHex;
}

bool addUser(const String& ra, const String& name, const String& uid) {
  if (ra.isEmpty() || uid.isEmpty()) {
    Serial.println("Erro: RA ou UID não podem ser vazios.");
    return false;
  }

  // Não imprimir RA, nome ou UID na serial: são dados identificáveis e credenciais clonáveis.
  Serial.println("Solicitação de cadastro RFID recebida.");

  JsonArray usersArray = usersDoc["users"].as<JsonArray>();
  if (!usersArray) {
    Serial.println("addUser: JsonArray 'users' inválido ou não inicializado.");
    return false;
  }

  for (JsonObject user : usersArray) {
    if (user["ra"] == ra) {
      Serial.println("Usuário com este RA já existe.");
      return false;
    }
    if (user["uid"] == uid) {
      Serial.println("Esta UID já está atribuída a outro usuário.");
      return false;
    }
  }

  JsonObject newUser = usersArray.createNestedObject();
  newUser["ra"] = ra;
  newUser["name"] = name;
  newUser["uid"] = uid;

  saveUsers(); 
  return true;
}

bool removeUser(const String& ra) {
  if (ra.isEmpty()) {
    Serial.println("Erro: RA para remoção não pode ser vazio.");
    return false;
  }

  JsonArray usersArray = usersDoc["users"].as<JsonArray>();
  if (!usersArray) {
    Serial.println("removeUser: JsonArray 'users' inválido.");
    return false;
  }

  bool removed = false;
  for (int i = 0; i < usersArray.size(); i++) {
    JsonObject user = usersArray[i];
    if (user["ra"] == ra) {
      usersArray.remove(i);
      removed = true;
      break; 
    }
  }

  if (removed) {
    saveUsers(); 
  }
  return removed;
}

bool isAuthorized(byte *uid) {
  String incomingUidHex = getUidHexString(uid, rfid.uid.size);
  JsonArray usersArray = usersDoc["users"].as<JsonArray>();
  if (!usersArray) {
    return false; 
  }
  for (JsonObject user : usersArray) {
    if (user["uid"] == incomingUidHex) {
      return true;
    }
  }
  return false;
}

String findUserByUid(byte *uid) {
  String incomingUidHex = getUidHexString(uid, rfid.uid.size);
  JsonArray usersArray = usersDoc["users"].as<JsonArray>();
  if (!usersArray) {
    return "Erro de Array";
  }
  for (JsonObject user : usersArray) {
    if (user["uid"] == incomingUidHex) {
      return user["name"].as<String>();
    }
  }
  return "Usuário Desconhecido";
}

String findUserByRa(const String& ra) {
  JsonArray usersArray = usersDoc["users"].as<JsonArray>();
  if (!usersArray) {
    return ""; 
  }
  for (JsonObject user : usersArray) {
    if (user["ra"] == ra) {
      return user["name"].as<String>();
    }
  }
  return "";
}


void ativarRelePorta() {
  // Reativação renova o intervalo; o timestamp zero é válido após wrap.
  relayActivatedAtMs = static_cast<uint32_t>(millis());
  relayActive = true;
  digitalWrite(pinRele, LOW);
  Serial.println("Relé acionado.");
}

void flashLED(int pin) {
  const uint32_t now = static_cast<uint32_t>(millis());
  if (pin == ledVerde) {
    greenLedActivatedAtMs = now;
    greenLedActive = true;
  } else if (pin == ledVermelho) {
    redLedActivatedAtMs = now;
    redLedActive = true;
  } else {
    return;
  }
  digitalWrite(pin, HIGH);
}


// Token de cookie aleatório: valor bruto entregue somente no Set-Cookie.
// O servidor guarda SHA-256 do cookie, e um nonce anti-CSRF por login.
String modulockRandomHex() {
  uint8_t bytes[32];
  esp_fill_random(bytes, sizeof(bytes));
  static const char hex[] = "0123456789abcdef";
  char token[65];
  for (size_t i=0; i<32; ++i) {
    token[i*2] = hex[(bytes[i] >> 4) & 15];
    token[i*2+1] = hex[bytes[i] & 15];
  }
  token[64] = '\0';
  memset(bytes, 0, sizeof(bytes));
  return String(token);
}

String modulockCookieToken(AsyncWebServerRequest* request) {
  if (!request->hasHeader("Cookie")) return "";
  const String cookies = request->getHeader("Cookie")->value();
  if (cookies.length() > 2048) return "";
  String token = "";
  bool seen = false;
  int index = 0;
  while (index < cookies.length()) {
    int end = cookies.indexOf(';', index);
    if (end < 0) end = cookies.length();
    String cookie = cookies.substring(index, end);
    cookie.trim();
    if (cookie.startsWith("session_id=")) {
      if (seen) return ""; // cookies duplicados são ambíguos
      seen = true;
      token = cookie.substring(strlen("session_id="));
    }
    index = end + 1;
  }
  if (!seen || token.length() != 64) return "";
  for (size_t i=0; i<64; ++i) {
    char ch = token[i];
    if (!((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f'))) return "";
  }
  return token;
}

bool modulockCookieDigest(AsyncWebServerRequest* request, uint8_t digest[32]) {
  const String cookie = modulockCookieToken(request);
  if (cookie.length() != 64) return false;
  return mbedtls_sha256_ret(
      reinterpret_cast<const unsigned char*>(cookie.c_str()),
      cookie.length(), digest, 0) == 0;
}

// Acesso a estado compartilhado entre AsyncTCP e loop() requer mutex.
// Todos os métodos da política são chamados apenas dentro do lock.
bool modulockSessionCheck(AsyncWebServerRequest* request, bool mutation,
                         bool revoke = false) {
  uint8_t digest[32] = {0};
  if (!adminSessionMutex || !modulockCookieDigest(request, digest)) return false;
  String csrf;
  if (mutation) {
    if (!request->hasHeader("X-Modulock-CSRF")) return false;
    csrf = request->getHeader("X-Modulock-CSRF")->value();
    if (csrf.length() != 64) return false;
  }
  if (xSemaphoreTake(adminSessionMutex, pdMS_TO_TICKS(50)) != pdTRUE) return false;
  const uint32_t now = static_cast<uint32_t>(millis());
  const bool ok = revoke
      ? adminSession.revoke(digest, csrf.c_str(), now)
      : (mutation ? adminSession.mutationAllowed(digest, csrf.c_str(), now)
                  : adminSession.valid(digest, now));
  xSemaphoreGive(adminSessionMutex);
  memset(digest, 0, sizeof(digest));
  return ok;
}

bool isAuthenticated(AsyncWebServerRequest* request) {
  return modulockSessionCheck(request, false);
}

bool hasValidCsrf(AsyncWebServerRequest* request) {
  return modulockSessionCheck(request, true);
}

void handleSessionCsrf(AsyncWebServerRequest* request) {
  uint8_t digest[32] = {0};
  if (!adminSessionMutex || !modulockCookieDigest(request, digest)) {
    request->send(401, "text/plain", "Não autorizado.");
    return;
  }
  char csrf[65] = {0};
  if (xSemaphoreTake(adminSessionMutex, pdMS_TO_TICKS(50)) != pdTRUE) {
    request->send(503, "text/plain", "Sessão indisponível.");
    return;
  }
  const bool ok = adminSession.csrfFor(
      digest, static_cast<uint32_t>(millis()), csrf);
  xSemaphoreGive(adminSessionMutex);
  memset(digest, 0, sizeof(digest));
  if (!ok) {
    request->send(401, "text/plain", "Sessão expirada.");
    return;
  }
  const String responseJson =
      String(R"json({"csrfToken":")json") + String(csrf) + R"json("})json";
  AsyncWebServerResponse* response =
      request->beginResponse(200, "application/json", responseJson);
  response->addHeader("Cache-Control", "no-store");
  response->addHeader("Pragma", "no-cache");
  request->send(response);
  memset(csrf, 0, sizeof(csrf));
}

void handleLogout(AsyncWebServerRequest* request) {
  if (!isAuthenticated(request)) {
    request->send(401, "text/plain", "Sessão inválida.");
    return;
  }
  if (!modulockSessionCheck(request, true, true)) {
    request->send(403, "text/plain", "Token de requisição inválido.");
    return;
  }
  AsyncWebServerResponse* response = request->beginResponse(204, "text/plain", "");
  response->addHeader("Set-Cookie",
      "session_id=; Path=/; Max-Age=0; HttpOnly; SameSite=Strict");
  response->addHeader("Cache-Control", "no-store");
  request->send(response);
}

void handleRoot(AsyncWebServerRequest *request) {
  Serial.println("Requisição recebida para /");

  if (isAuthenticated(request)) {
      request->redirect("/dashboard");
      return;
  }

  String htmlContent = R"rawliteral(<!DOCTYPE html>
    <html>
    <head>
      <title>Login</title>
      <meta name="viewport" content="width=device-width, initial-scale=1">
      <style>
        body { font-family: Arial, sans-serif; background-color: #f4f4f4; display: flex; justify-content: center; align-items: center; min-height: 100vh; margin: 0; }
        .container { background-color: #fff; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); width: 100%; max-width: 400px; text-align: center; }
        h2 { color: #333; }
        input[type="text"], input[type="password"] { width: calc(100% - 20px); padding: 10px; margin: 8px 0; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box; }
        button[type="submit"], .button-link { background-color: #007bff; color: white; padding: 10px 15px; border: none; border-radius: 4px; cursor: pointer; font-size: 16px; margin-top: 10px; display: inline-block; text-decoration: none; }
        button[type="submit"]:hover, .button-link:hover { background-color: #0056b3; }
        .error { color: red; font-size: 0.9em; margin-top: 5px; }
      </style>
    </head>
    <body>
      <div class="container">
        <h2>RFID Access System</h2>
        <form action="/login" method="post">
          <p>RA: <input type="text" name="username" required></p>
          <p>Password: <input type="password" name="password" required></p>
          <button type="submit">Login</button>
          <p class="error" id="loginError"></p>
        </form>
      </div>
      <script>
        const urlParams = new URLSearchParams(window.location.search);
        const error = urlParams.get('error');
        if (error === '1') {
          document.getElementById('loginError').innerText = 'Usuário ou senha incorretos.';
        } else if (error === '2') {
          document.getElementById('loginError').innerText = 'Acesso não autorizado.';
        }
      </script>
    </body>
    </html>)rawliteral";
  
  request->send(200, "text/html", htmlContent);
  Serial.println("Página de login principal enviada.");
}

void handleLogin(AsyncWebServerRequest *request) {

  if (request->hasParam("username", true) && request->hasParam("password", true)) {
    String username = request->getParam("username", true)->value();
    String password = request->getParam("password", true)->value();

    if (username == WEB_USERNAME && password == WEB_PASSWORD) {
      String token = modulockRandomHex();
      String csrf = modulockRandomHex();
      uint8_t digest[32] = {0};
      if (!adminSessionMutex ||
          mbedtls_sha256_ret(reinterpret_cast<const unsigned char*>(token.c_str()),
                             token.length(), digest, 0) != 0 ||
          xSemaphoreTake(adminSessionMutex, pdMS_TO_TICKS(50)) != pdTRUE) {
        request->send(503, "text/plain", "Não foi possível criar sessão.");
        return;
      }
      const bool issued = adminSession.issue(
          digest, csrf.c_str(), static_cast<uint32_t>(millis()));
      xSemaphoreGive(adminSessionMutex);
      memset(digest, 0, sizeof(digest));
      csrf = "";
      if (!issued) {
        request->send(503, "text/plain", "Não foi possível criar sessão.");
        return;
      }
      AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "");
      response->addHeader("Set-Cookie",
          "session_id=" + token + "; Path=/; HttpOnly; SameSite=Strict");
      response->addHeader("Cache-Control", "no-store");
      response->addHeader("Location", "/dashboard");
      request->send(response);

    } else {
      request->redirect("/?error=1");
    }
  } else {
    request->redirect("/?error=1");
  }
}

void handleDashboard(AsyncWebServerRequest *request) {
  if (!isAuthenticated(request)) {
      request->redirect("/?error=2");
      return;
  }

  String htmlContent = R"rawliteral(<!DOCTYPE html>
    <html>
    <head>
      <title>ModuLock</title>
      <meta name="viewport" content="width=device-width, initial-scale=1">
      <style>
        body { font-family: Arial, sans-serif; background-color: #f4f4f4; display: flex; justify-content: center; align-items: center; min-height: 100vh; margin: 0; }
        .container { background-color: #fff; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); width: 100%; max-width: 600px; text-align: center; }
        h2, h3 { color: #333; }
        button, .button-link { background-color: #007bff; color: white; padding: 10px 15px; border: none; border-radius: 4px; cursor: pointer; font-size: 16px; margin: 5px; display: inline-block; text-decoration: none; }
        button:hover, .button-link:hover { background-color: #0056b3; }
        .section { margin-top: 20px; border-top: 1px solid #eee; padding-top: 20px; }
        table { width: 100%; border-collapse: collapse; margin-top: 10px; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
        th { background-color: #f2f2f2; }
        .logout-button { background-color: #dc3545; }
        .logout-button:hover { background-color: #c82333; }
      </style>
    </head>
    <body>
      <div class="container">
        <h2>Bem vindo, %USERNAME%!</h2>
        <button onclick="openDoor()">Abrir porta</button>
        <button class="logout-button" onclick="logout()">Logout</button>

        <div class="section">
          <h3>Usuários cadastrados</h3>
          <table id="userList">
            <thead><tr><th>RA</th><th>Nome</th><th>Pin</th><th>Ação</th></tr></thead>
            <tbody></tbody>
          </table>
          <a href="/register" class="button-link">Registrar novo usuário</a>
        </div>
      </div>
      <script>
        async function csrfHeaders() {
          const response = await fetch('/session/csrf', { cache: 'no-store' });
          if (!response.ok) throw new Error('Sessão encerrada');
          const { csrfToken } = await response.json();
          return { 'X-Modulock-CSRF': csrfToken };
        }
        function openDoor() {
          csrfHeaders().then(headers => fetch('/openDoor', { method: 'POST', headers }))
            .then(response => response.text())
            .then(data => alert(data))
            .catch(error => console.error('Error:', error));
        }

        function logout() {
          csrfHeaders()
            .then(headers => fetch('/logout', { method: 'POST', headers }))
            .finally(() => { window.location.href = '/'; });
        }

        function loadUsers() {
          fetch('/getUsers')
            .then(response => response.json())
            .then(data => {
              const userList = document.getElementById('userList').getElementsByTagName('tbody')[0];
              userList.innerHTML = ''; 
              data.users.forEach(user => {
                const row = userList.insertRow();
                row.insertCell().innerText = user.ra;
                row.insertCell().innerText = user.name;
                row.insertCell().innerText = user.uid;
                const actionCell = row.insertCell();
                const removeButton = document.createElement('button');
                removeButton.innerText = 'Remove';
                removeButton.style.backgroundColor = '#dc3545';
                removeButton.style.padding = '5px 10px';
                removeButton.style.fontSize = '0.8em';
                removeButton.onclick = () => removeUser(user.ra);
                actionCell.appendChild(removeButton);
              });
            })
            .catch(error => console.error('Error loading users:', error));
        }

        function removeUser(ra) {
          if (confirm('Are you sure you want to remove user with RA: ' + ra + '?')) {
            const formData = new URLSearchParams();
            formData.append('ra', ra);
            csrfHeaders().then(headers => fetch('/removeUser', {
              method: 'POST',
              headers: { ...headers, 'Content-Type': 'application/x-www-form-urlencoded' },
              body: formData
            }))
            .then(response => response.text())
            .then(data => {
              alert(data);
              loadUsers(); 
            })
            .catch(error => console.error('Error removing user:', error));
          }
        }
        
        loadUsers();
      </script>
    </body>
    </html>)rawliteral";

  htmlContent.replace("%USERNAME%", WEB_USERNAME);

  request->send(200, "text/html", htmlContent);
}


void handleAbrirPorta(AsyncWebServerRequest *request) {
  if (!isAuthenticated(request)) {
      request->send(401, "text/plain", "Não autorizado. Faça login.");
      return;
  }
  if (!hasValidCsrf(request)) {
      request->send(403, "text/plain", "Token de requisição inválido.");
      return;
  }
  ativarRelePorta();
  flashLED(ledVerde);
  request->send(200, "text/plain", "Porta aberta com sucesso!");
}

void handleGetUsuarios(AsyncWebServerRequest *request) {
  Serial.println("Requisição GET para /getUsers.");
  if (!isAuthenticated(request)) {
      request->send(401, "text/plain", "Não autorizado. Faça login.");
      return;
  }

  String jsonResponse;
  serializeJson(usersDoc, jsonResponse);
  request->send(200, "application/json", jsonResponse);
}

void handlePaginaRegistro(AsyncWebServerRequest *request) {
  if (!isAuthenticated(request)) {
      request->redirect("/?error=2");
      return;
  }

  String htmlContent = R"rawliteral(<!DOCTYPE html>
    <html>
    <head>
      <title>Registro</title>
      <meta name="viewport" content="width=device-width, initial-scale=1">
      <style>
        body { font-family: Arial, sans-serif; background-color: #f4f4f4; display: flex; justify-content: center; align-items: center; min-height: 100vh; margin: 0; }
        .container { background-color: #fff; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); width: 100%; max-width: 400px; text-align: center; }
        h2 { color: #333; }
        p { margin: 8px 0; }
        input[type="text"], input[type="password"] { width: calc(100% - 20px); padding: 10px; margin: 8px 0; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box; }
        button, .back-button { background-color: #28a745; color: white; padding: 10px 15px; border: none; border-radius: 4px; cursor: pointer; font-size: 16px; margin-top: 10px; display: inline-block; text-decoration: none; }
        button:hover, .back-button:hover { background-color: #218838; }
        .uid-display { font-weight: bold; color: #007bff; }
        #message { color: green; font-weight: bold; margin-top: 10px; }
        #message.error { color: red; }
      </style>
    </head>
    <body>
      <div class="container">
        <h2>Registrar novo PIN</h2>
        <p class="uid-display" id="uidStatus">Escaneie o pin no RFID</p>
        <p>PIN: <input type="text" id="uidInput" name="uid" readonly></p>
        <p>RA: <input type="text" id="raInput" name="ra" required></p>
        <p>Nome: <input type="text" id="nameInput" name="name" required></p>
        <button onclick="registerUser()">Registrar Usuário</button>
        <p id="message"></p>
        <a href="/dashboard" class="back-button">Voltar</a>
      </div>
      <script>
        function fetchLastScannedUid() {
          fetch('/getLastScannedUid')
            .then(response => response.text())
            .then(data => {
              const uidInput = document.getElementById('uidInput');
              const uidStatus = document.getElementById('uidStatus');
              if (data !== "No UID") {
                uidInput.value = data;
                uidStatus.innerText = "Pin escaneado " + data;
              } else {
              }
            })
            .catch(error => console.error('Error fetching UID:', error));
        }

        setInterval(fetchLastScannedUid, 1000);

        async function csrfHeaders() {
          const response = await fetch('/session/csrf', { cache: 'no-store' });
          if (!response.ok) throw new Error('Sessão encerrada');
          const { csrfToken } = await response.json();
          return { 'X-Modulock-CSRF': csrfToken };
        }
        function registerUser() {
          const ra = document.getElementById('raInput').value;
          const name = document.getElementById('nameInput').value;
          const uid = document.getElementById('uidInput').value;
          const messageElement = document.getElementById('message');

          if (!ra || !name || !uid) {
              messageElement.className = 'error';
              messageElement.innerText = 'Por favor, preencha todos os campos.';
              return;
          }

          const formData = new URLSearchParams();
          formData.append('ra', ra);
          formData.append('name', name);
          formData.append('uid', uid);

          csrfHeaders().then(headers => fetch('/registerUser', {
            method: 'POST',
            headers: { ...headers, 'Content-Type': 'application/x-www-form-urlencoded' },
            body: formData
          }))
          .then(response => response.text())
          .then(data => {
            messageElement.innerText = data;
            if (data.includes("sucesso")) {
                messageElement.className = '';
                document.getElementById('raInput').value = '';
                document.getElementById('nameInput').value = '';
                document.getElementById('uidInput').value = '';
                document.getElementById('uidStatus').innerText = 'Awaiting RFID scan...';
            } else {
                messageElement.className = 'error';
            }
          })
          .catch(error => {
            console.error('Error registering user:', error);
            messageElement.className = 'error';
            messageElement.innerText = 'Ocorreu um erro na requisição.';
          });
        }
      </script>
    </body>
    </html>)rawliteral";

  request->send(200, "text/html", htmlContent);
}

void handleRegisterUser(AsyncWebServerRequest *request) {
  if (!isAuthenticated(request)) {
      request->send(401, "text/plain", "Não autorizado. Faça login como admin.");
      return;
  }
  if (!hasValidCsrf(request)) {
      request->send(403, "text/plain", "Token de requisição inválido.");
      return;
  }

  if (request->hasParam("ra", true) && request->hasParam("name", true) && request->hasParam("uid", true)) {
    String ra = request->getParam("ra", true)->value();
    String name = request->getParam("name", true)->value();
    String uid = request->getParam("uid", true)->value();

    // Um administrador autenticado precisa confirmar um cartão fisicamente
    // escaneado nesta sessão de cadastro, sem aceitar UID arbitrário/expirado.
    if (!modulockUidRegistrationAllowed(
            true, uid.c_str(), lastScannedUidForRegistration.c_str(),
            static_cast<uint32_t>(millis()), static_cast<uint32_t>(lastUidScanTime),
            static_cast<uint32_t>(UID_SCAN_TIMEOUT_MS))) {
      request->send(409, "text/plain", "Aguarde uma nova leitura RFID válida para cadastrar.");
      return;
    }

    if (addUser(ra, name, uid)) {
      lastScannedUidForRegistration = "";
      lastUidScanTime = 0;
      request->send(200, "text/plain", "Usuário " + name + " (RA: " + ra + ") registrado com sucesso!");
      Serial.println("Registro RFID concluído.");
    } else {
      request->send(200, "text/plain", "Erro: Falha ao registrar usuário. RA ou UID podem já existir ou campos vazios.");
    }
  } else {
    request->send(400, "text/plain", "Erro: Parâmetros ausentes para o registro do usuário.");
    Serial.println("Falha ao registrar usuário: Parâmetros ausentes.");
  }
}

void handleRemoveUser(AsyncWebServerRequest *request) {
  Serial.println("Requisição POST recebida para /removeUser.");
  if (!isAuthenticated(request)) {
      request->send(401, "text/plain", "Não autorizado. Faça login como admin.");
      return;
  }
  if (!hasValidCsrf(request)) {
      request->send(403, "text/plain", "Token de requisição inválido.");
      return;
  }

  if (request->hasParam("ra", true)) {
    String raToRemove = request->getParam("ra", true)->value();
    if (removeUser(raToRemove)) {
      request->send(200, "text/plain", "Usuário com RA '" + raToRemove + "' removido com sucesso.");
    } else {
      request->send(404, "text/plain", "Erro: Usuário com RA '" + raToRemove + "' não encontrado ou RA vazio.");
    }
  } else {
    request->send(400, "text/plain", "Erro: Parâmetro RA ausente para remoção.");
  }
}

void handleGetLastScannedUid(AsyncWebServerRequest *request) {
  if (!isAuthenticated(request)) {
    request->send(401, "text/plain", "Não autorizado.");
    return;
  }

  const bool recente = modulockUidRecentlyScanned(
      !lastScannedUidForRegistration.isEmpty(),
      static_cast<uint32_t>(millis()),
      static_cast<uint32_t>(lastUidScanTime),
      static_cast<uint32_t>(UID_SCAN_TIMEOUT_MS));
  AsyncWebServerResponse *response = request->beginResponse(
      200, "text/plain", recente ? lastScannedUidForRegistration : "No UID");
  response->addHeader("Cache-Control", "no-store");
  response->addHeader("Pragma", "no-cache");
  request->send(response);
}

void handleNotFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Página Não Encontrada");
}

// Comandos Bluetooth não autenticados removidos intencionalmente.
// Nenhum comando remoto de abertura ou cadastro existe nesta build.

// --- Função Setup ---


void setup() {
  Serial.begin(115200);
  SPI.begin();
  rfid.PCD_Init();

  pinMode(pinRele, OUTPUT);
  pinMode(ledVerde, OUTPUT);
  pinMode(ledVermelho, OUTPUT);
  digitalWrite(pinRele, HIGH);
  digitalWrite(ledVerde, LOW);
  digitalWrite(ledVermelho, LOW);

  if (!SPIFFS.begin(true)) {
    Serial.println("Erro ao montar o SPIFFS! Verifique se a partição está correta.");
    while(true); 
  }

  loadUsers(); 
  
  // Nao registrar a lista de pessoas/RFID na porta serial.

  // Entrada de comandos Bluetooth permanece desativada, pois nao autenticava operadores.
  // Um protocolo autenticado deve ser implementado e testado antes de reativar.
  Serial.println("Bluetooth de comandos desativado por seguranca.");

  // SSID e senha nunca sao registrados em logs.
  if (strlen(WIFI_SSID) == 0 || strlen(WIFI_PASSWORD) == 0 ||
      strlen(WEB_USERNAME) == 0 || strlen(WEB_PASSWORD) < 12) {
    Serial.println("Configure include/ModulockSecrets.h com credenciais locais seguras.");
    return;
  }
  adminSessionMutex = xSemaphoreCreateMutex();
  if (!adminSessionMutex) {
    Serial.println("Falha ao iniciar controle de sessão; servidor não iniciado.");
    return;
  }
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long inicioConexao = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - inicioConexao < 30000) {
    delay(500);
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Falha de Wi-Fi: servidor nao iniciado.");
    return;
  }

  Serial.println("");
  Serial.println("Conectado ao Wi-Fi!");
  Serial.print("Endereço IP do ESP32: ");
  Serial.println(WiFi.localIP());


  server.on("/", HTTP_GET, handleRoot);
  server.on("/login", HTTP_POST, handleLogin);
  server.on("/session/csrf", HTTP_GET, handleSessionCsrf);
  server.on("/logout", HTTP_POST, handleLogout);
  server.on("/dashboard", HTTP_GET, handleDashboard); 

  server.on("/openDoor", HTTP_POST, handleAbrirPorta);
  server.on("/getUsers", HTTP_GET, handleGetUsuarios);
  server.on("/register", HTTP_GET, handlePaginaRegistro);
  server.on("/registerUser", HTTP_POST, handleRegisterUser);
  server.on("/removeUser", HTTP_POST, handleRemoveUser);
  server.on("/getLastScannedUid", HTTP_GET, handleGetLastScannedUid);

  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });

  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/script.js", "application/javascript");
  });

  server.onNotFound(handleNotFound);

  server.begin();
  servidorHabilitado = true;
  Serial.println("Servidor Web iniciado!");
}


void loop() {
  if (!servidorHabilitado) { delay(1000); return; }
  const uint32_t now = static_cast<uint32_t>(millis());
  static uint32_t lastSessionSweepAt = 0;
  if (static_cast<uint32_t>(now - lastSessionSweepAt) >= 30000u &&
      xSemaphoreTake(adminSessionMutex, 0) == pdTRUE) {
    adminSession.expire(now);
    xSemaphoreGive(adminSessionMutex);
    lastSessionSweepAt = now;
  }
  if (modulockTimedActionExpired(relayActive, now, relayActivatedAtMs,
                                static_cast<uint32_t>(DOOR_OPEN_DURATION_MS))) {
    digitalWrite(pinRele, HIGH); // HIGH = relé desenergizado
    relayActive = false;
  }
  if (modulockTimedActionExpired(greenLedActive, now, greenLedActivatedAtMs,
                                static_cast<uint32_t>(LED_FLASH_DURATION_MS))) {
    digitalWrite(ledVerde, LOW);
    greenLedActive = false;
  }
  if (modulockTimedActionExpired(redLedActive, now, redLedActivatedAtMs,
                                static_cast<uint32_t>(LED_FLASH_DURATION_MS))) {
    digitalWrite(ledVermelho, LOW);
    redLedActive = false;
  }

  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    String currentUid = getUidHexString(rfid.uid.uidByte, rfid.uid.size);

    lastScannedUidForRegistration = currentUid;
    lastUidScanTime = millis();

    if (isAuthorized(rfid.uid.uidByte)) {
      ativarRelePorta();
      flashLED(ledVerde);
    } else {
      flashLED(ledVermelho);
    }

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }

}
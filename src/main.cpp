#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <BluetoothSerial.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ArduinoJson.h>
#include <FS.h>      
#include <SPIFFS.h>  

#define RST_PIN 22   
#define SS_PIN 21   
#define pinRele 4    
#define ledVerde 13   
#define ledVermelho 14   

const char* WEB_USERNAME = "admin"; 
const char* WEB_PASSWORD = "123";   
const char* USERS_FILE_PATH = "/users.json"; 

const char* WIFI_SSID = "VIVOFIBRA-E150"; 
const char* WIFI_PASSWORD = "D07F134963"; 

const unsigned long DOOR_OPEN_DURATION_MS = 1000; 
const unsigned long LED_FLASH_DURATION_MS = 2000; 
const unsigned long UID_SCAN_TIMEOUT_MS = 30000; 
MFRC522 rfid(SS_PIN, RST_PIN);
AsyncWebServer server(80);
BluetoothSerial SerialBT;

DynamicJsonDocument usersDoc(8192);

String lastScannedUidForRegistration = "";
unsigned long lastUidScanTime = 0;
unsigned long relayOffTime = 0; 
unsigned long greenLedOffTime = 0; 
unsigned long redLedOffTime = 0;  

void loadUsers();
void saveUsers();
bool addUser(const String& ra, const String& name, const String& uid);
bool removeUser(const String& ra);
bool isAuthorized(byte *uid);
String findUserByUid(byte *uid);
String findUserByRa(const String& ra);

bool isAuthenticated(AsyncWebServerRequest *request); 
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
void processarComandoBluetooth(const String& command);
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

  Serial.print("Tentando adicionar usuário: RA=");
  Serial.print(ra);
  Serial.print(", Nome=");
  Serial.print(name);
  Serial.print(", UID=");
  Serial.println(uid);

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
  digitalWrite(pinRele, LOW);
  relayOffTime = millis() + DOOR_OPEN_DURATION_MS;
  Serial.println("Relé acionado.");
}

void flashLED(int pin) {
  digitalWrite(pin, HIGH);
  if (pin == ledVerde) {
    greenLedOffTime = millis() + LED_FLASH_DURATION_MS;
  } else if (pin == ledVermelho) {
    redLedOffTime = millis() + LED_FLASH_DURATION_MS;
  }
}


bool isAuthenticated(AsyncWebServerRequest *request) {
    if (request->hasHeader("Cookie")) {
        String cookieHeader = request->getHeader("Cookie")->value();
        int idx = cookieHeader.indexOf("session_id=");
        if (idx != -1) {
            return true;
        }
    }
    return false;
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
      AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "");
      response->addHeader("Set-Cookie", "session_id=admin_session; Path=/"); 
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
        function openDoor() {
          fetch('/openDoor', { method: 'POST' })
            .then(response => response.text())
            .then(data => alert(data))
            .catch(error => console.error('Error:', error));
        }

        function logout() {
            document.cookie = "session_id=; expires=Thu, 01 Jan 1970 00:00:00 UTC; path=/;"; // Expira o cookie
            window.location.href = "/"; 
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
            fetch('/removeUser', {
              method: 'POST',
              headers: {
                'Content-Type': 'application/x-www-form-urlencoded'
              },
              body: formData
            })
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
  SerialBT.println("Web: Porta Aberta");
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

          fetch('/registerUser', {
            method: 'POST',
            headers: {
              'Content-Type': 'application/x-www-form-urlencoded'
            },
            body: formData
          })
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

  if (request->hasParam("ra", true) && request->hasParam("name", true) && request->hasParam("uid", true)) {
    String ra = request->getParam("ra", true)->value();
    String name = request->getParam("name", true)->value();
    String uid = request->getParam("uid", true)->value();

    if (addUser(ra, name, uid)) {
      request->send(200, "text/plain", "Usuário " + name + " (RA: " + ra + ") registrado com sucesso!");
      Serial.println("Usuário registrado: " + name + " (RA: " + ra + ")");
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
  if (lastScannedUidForRegistration.length() > 0 && (millis() - lastUidScanTime < UID_SCAN_TIMEOUT_MS)) {
    request->send(200, "text/plain", lastScannedUidForRegistration);
  } else {
    request->send(200, "text/plain", "No UID");
  }
}

void handleNotFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Página Não Encontrada");
}

void processarComandoBluetooth(const String& command) {
  String cmd = command;
  cmd.trim();
  cmd.toUpperCase();

  Serial.print("Comando Bluetooth recebido: ");
  Serial.println(cmd);

  if (cmd == "OPEN DOOR") {
    SerialBT.println("BT: Porta Aberta");
    ativarRelePorta();
    flashLED(ledVerde);
  } else if (cmd.startsWith("ADD USER ")) {
    int firstComma = cmd.indexOf(',');
    int secondComma = cmd.indexOf(',', firstComma + 1);
    if (firstComma != -1 && secondComma != -1) {
      String ra = cmd.substring(9, firstComma);
      String name = cmd.substring(firstComma + 1, secondComma);
      String uid = cmd.substring(secondComma + 1);

      if (addUser(ra, name, uid)) {
        SerialBT.println("BT: Usuário " + name + " (RA: " + ra + ") adicionado.");
      } else {
        SerialBT.println("BT: Falha ao adicionar usuário. RA ou UID podem existir.");
      }
    } else {
      SerialBT.println("BT: Formato de comando inválido. Use 'ADD USER RA,NOME,UID'");
    }
  } else if (cmd.startsWith("REMOVE USER ")) {
    String raToRemove = cmd.substring(12);
    if (removeUser(raToRemove)) {
      SerialBT.println("BT: Usuário " + raToRemove + " removido.");
    } else {
      SerialBT.println("BT: Usuário " + raToRemove + " não encontrado.");
    }
  } else if (cmd == "GET USERS") {
    String jsonResponse;
    serializeJson(usersDoc, jsonResponse);
    SerialBT.print("BT: Usuários Registrados: ");
    SerialBT.println(jsonResponse);
  }
  else {
    SerialBT.println("BT: Comando desconhecido. Comandos: OPEN DOOR, ADD USER RA,NOME,UID, REMOVE USER RA, GET USERS");
  }
}

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
  
  serializeJson(usersDoc, Serial); 

  SerialBT.begin("ESP32_RFID_Access");

  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  Serial.println("");
  Serial.println("Conectado ao Wi-Fi!");
  Serial.print("Endereço IP do ESP32: ");
  Serial.println(WiFi.localIP());


  server.on("/", HTTP_GET, handleRoot);
  server.on("/login", HTTP_POST, handleLogin);
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
  Serial.println("Servidor Web iniciado!");
}


void loop() {
  if (relayOffTime != 0 && millis() >= relayOffTime) {
    digitalWrite(pinRele, HIGH);
    relayOffTime = 0;
  }
  if (greenLedOffTime != 0 && millis() >= greenLedOffTime) {
    digitalWrite(ledVerde, LOW);
    greenLedOffTime = 0;
  }
  if (redLedOffTime != 0 && millis() >= redLedOffTime) {
    digitalWrite(ledVermelho, LOW);
    redLedOffTime = 0;
  }

  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    String currentUid = getUidHexString(rfid.uid.uidByte, rfid.uid.size);

    lastScannedUidForRegistration = currentUid;
    lastUidScanTime = millis();

    if (isAuthorized(rfid.uid.uidByte)) {
      String userName = findUserByUid(rfid.uid.uidByte);
      SerialBT.println("RFID: Autorizado - Porta Aberta para " + userName);
      ativarRelePorta();
      flashLED(ledVerde);
    } else {
      SerialBT.println("RFID: Não Autorizado");
      flashLED(ledVermelho);
    }

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }

  if (SerialBT.available()) {
    String command = SerialBT.readStringUntil('\n');
    processarComandoBluetooth(command);
  }
}
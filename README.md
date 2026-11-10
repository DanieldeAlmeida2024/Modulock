# Modulock — controle experimental de acesso RFID/NFC (ESP32)

Firmware de laboratório para ESP32-WROOM-32, leitor MFRC522, relé e página web administrativa. **Não utilizar como fechadura de segurança real sem auditoria, criptografia de transporte e testes no dispositivo.**

## Hardware
ESP32, MFRC522, módulo relé compatível, LEDs, fonte e conversor adequados ao projeto elétrico. Consulte o diagrama físico existente antes de energizar a montagem.

## Configurar e compilar
1. Instale PlatformIO e as dependências declaradas em `platformio.ini`.
2. Copie `include/ModulockSecrets.example.h` para `include/ModulockSecrets.h`.
3. Defina Wi-Fi e administrador localmente; não versionar esse arquivo. A senha administrativa deve ter **12 ou mais caracteres**.
4. Compile com `pio run`; para gravar no hardware use `pio run -t upload` apenas após conferir circuito e pinos.
5. Inicialize SPIFSS conforme particionamento do firmware e revise riscos de perda de dados.

## Segurança
- Antes havia senha/SSID fixos no código e um cookie web aceito sem validação do identificador. A correção proposta remove os valores fixos e usa sessão aleatória por inicialização.
- O canal de comandos **Bluetooth foi desativado** porque permitia desbloqueio e alteração de usuários sem autenticação.
- O painel usa HTTP sem TLS: restringir a rede de teste; não expor portas ao público.
- **Rotacione imediatamente** quaisquer credenciais previamente publicadas; os valores continuam visíveis nos commits antigos até um saneamento do histórico.
- UID de cartões RFID pode ser clonado dependendo do cartão/tecnologia: não utilizar como autenticação forte sem mecanismo adicional.

## Documentação
[Arquitetura](docs/ARQUITETURA.md) · [Requisitos](docs/REQUISITOS.md) · [UML](docs/UML.puml) · [Testes](docs/PLANO_TESTES.md) · [Instruções para IA](AGENTS.md).

## Status
Alteração de segurança em **revisão**. Compilação PlatformIO e testes de ponta a ponta no ESP32 ainda obrigatórios. Não há garantia de uso seguro em ambientes reais.

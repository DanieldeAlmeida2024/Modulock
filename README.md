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

## Documentação técnica Modulock v1.0.0

- [Especificação do hardware e firmware](docs/projeto/ESPECIFICACAO.md)
- [Três diagramas UML](docs/uml/README.md)
- [Matriz de testes e segurança](docs/testes/PLANO_DE_TESTES.md)
- [Controle de versão](docs/controle/VERSIONAMENTO.md)

> Hardware físico: não considerar a implementação pronta para uma porta real sem testes de autenticação, Bluetooth e falha segura.

## Segurança da temporização física — proposta MOD-TIME v2.1.0

As saídas do relé e dos dois LEDs usam tempo modular uint32, evitando comparações de deadline absoluto após overflow de `millis()`. Foi removido também o log serial de RA/nome/UID durante cadastro. Há testes C++ host, verificações estáticas e build PlatformIO no CI, mas **nenhum hardware foi conectado/testado**. Não instalar em fechadura real sem ensaio de bancada, TLS/CSRF, avaliação de clonagem RFID e revisão das condições de corrida. [Especificação](docs/projeto/RELE_TEMPORIZACAO.md), [testes](docs/testes/PLANO_DE_TESTES.md), [UML](docs/uml/05_temporizadores_rele_wrap.puml), [versões](docs/controle/VERSIONAMENTO.md).

## Sessão administrativa — MOD-SESSION v2.2.0 (proposta)

Login cria um cookie novo de 256 bits, com SHA-256 do token na RAM do ESP32 e apenas uma sessão de operador por vez. Idle 10 min, expiração absoluta 30 min, logout via POST com revogação do lado do servidor, CSRF obrigatório para abrir porta e mudar cadastros. **HTTP porta 80 permanece sem TLS e não pode ser usado em produção.** [Especificação](docs/projeto/SESSOES_OPERADOR.md) · [Testes](docs/testes/SESSAO_CSRF.md) · [UML](docs/uml/06_sessao_revogacao_csrf.puml).

# Modulock — controle experimental de acesso RFID/NFC (ESP32)

Firmware de laboratório para ESP32-WROOM-32, leitor MFRC522, relé e página web administrativa. **Não utilizar como fechadura real.** HTTP administrativo desligado por padrão, Bluetooth de comandos removido e UID RFID clonável: ainda exigem autenticação robusta, TLS, controle físico, testes de bancada e homologação.

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
- O parser e a recepção de comandos **Bluetooth foram removidos do firmware**, pois permitiam desbloqueio e alteração de usuários sem autenticação.
- O painel HTTP sem TLS **não inicia por padrão** (`MODULOCK_ENABLE_UNENCRYPTED_ADMIN_HTTP=0`). Para testes isolados em bancada, alterar para `1` apenas no arquivo local `include/ModulockSecrets.h`. Não habilitar em rede de usuários ou Internet. Mesmo em bancada, a sessão/cookie e ausência de HTTPS/CSRF continuam inadequadas para porta real.
- A leitura RFID local continua sem exigir Wi-Fi; isso não torna UIDs clonáveis adequados para controle de acesso real.
- Falha na montagem de SPIFFS não provoca mais formatação automática (protege dados; sistema permanece em falha segura).
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

## Evidências da correção de segurança (13/11/2026)

A branch `fix/auditoria-entrevista-20261113` inclui o teste estático `python -m unittest discover -s tests -v` e execução no GitHub Actions. Esse contrato comprova apenas os padrões de código presentes. **Ainda não foi executada compilação PlatformIO nem teste físico/HIL**, e a issue de segurança não deve ser encerrada até eliminar o risco de clonagem de RFID e implementar um canal administrativo seguro com sessão revogável e proteção CSRF.

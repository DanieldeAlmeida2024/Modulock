# Modulock — controle experimental de acesso RFID (ESP32)

Firmware de bancada para estudo de controle físico de acesso com ESP32-WROOM-32, leitor MFRC522, relé, indicadores luminosos e interface de administração local.

## Hardware

ESP32, leitor RFID MFRC522, módulo relé, LEDs, fonte e conversor elétrico apropriados. Conferir a ligação dos pinos, a alimentação e as condições do circuito antes de energizar o dispositivo.

## Configuração

1. Instale PlatformIO e consulte as dependências de `platformio.ini`.
2. Copie `include/ModulockSecrets.example.h` para `include/ModulockSecrets.h`.
3. Configure Wi-Fi e credenciais locais; não versione o arquivo de segredos.
4. Compile com `pio run`.
5. Grave com `pio run -t upload` somente após conferir a montagem.
6. Valide o armazenamento SPIFFS e os estados do relé em condições de erro.

## Proteções padrão de rede e autenticação

- A interface administrativa HTTP sem TLS está **desligada por padrão**. O firmware não inicia o painel sem `MODULOCK_ENABLE_UNENCRYPTED_ADMIN_HTTP=1`, configurado exclusivamente no header local, para bancada isolada.
- O parser Bluetooth de comandos foi removido, incluindo comandos de abertura e modificação de usuários sem autenticação.
- A rotina de leitura RFID e o desligamento do relé continuam sendo executados sem depender de Wi-Fi ou do painel web.
- `SPIFFS.begin(false)` evita formatação automática do armazenamento se a montagem falhar.
- As verificações automáticas compilam o firmware e procuram regressões estáticas de segurança. Não substituem ensaios com a placa e o relé.

**Atenção:** a autenticação baseada apenas no UID RFID pode ser clonada. Mesmo com essas proteções, **não instalar como fechadura de segurança real** sem hardware apropriado, protocolos criptográficos e ensaios físicos.

## Segurança e condições de uso

- O sistema envolve interfaces Wi-Fi, HTTP e código de comunicação Bluetooth; todas devem ser avaliadas quanto a autenticação, autorização e possibilidade de acionar o relé indevidamente.
- HTTP sem TLS não fornece confidencialidade ou proteção adequada das credenciais. Mantenha o hardware em rede de teste isolada.
- O UID de cartões RFID pode ser clonado conforme a tecnologia do cartão; não equivale a autenticação forte.
- Credenciais de laboratório não devem ser reutilizadas em ambientes reais. Caso uma credencial tenha sido exposta em commits públicos, ela precisa ser substituída na origem.
- Para uma fechadura real seriam necessários proteção do canal, gestão de sessões, testes físicos de falha segura e avaliação específica do hardware.

**Finalidade:** prototipagem e pesquisa em bancada. O firmware não deve ser tratado como fechadura homologada ou solução comercial de controle de acesso.

## Documentação

[Requisitos](docs/REQUISITOS.md) · [Arquitetura](docs/ARQUITETURA.md) · [Especificação do firmware](docs/projeto/ESPECIFICACAO.md) · [Diagramas](docs/uml/README.md) · [Testes e segurança](docs/testes/PLANO_DE_TESTES.md) · [Versionamento](docs/controle/VERSIONAMENTO.md).

## Temporização do relé e LEDs — tolerante a overflow

O relé (GPIO4) e os LEDs (GPIO13/14) usam diferenças modulares de `millis()` com flags explícitas de atividade. A correção evita que a comparação absoluta de prazo desligue cedo após o wrap do contador de 32 bits (~49,7 dias). Testes C++11 em host e build PlatformIO são automatizados; **não equivalem a ensaio HIL**, portanto não conectar a fechaduras reais antes de validar temporização, brownout, reset, sensores e falha segura no hardware.

#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <IRremoteESP8266.h>
#include <IRac.h>
#include <IRtext.h>
#include <IRutils.h>

#include <addons/RTDBHelper.h>

/* 1. Define as credenciais do WiFi */
#define WIFI_SSID "LAPTOP-C1OJGPF7 5514"
#define WIFI_PASSWORD ";h45085D"

/* 2. Define a URL do RTDB e o segredo do banco de dados */
#define DATABASE_URL "https://ventilai-default-rtdb.firebaseio.com/"
#define DATABASE_SECRET "h6Y4SoLoaAKtrPwGYuPosiqq0r2qbWlOsdlVIB1X"

/* 3. Define o objeto Firebase Data */
FirebaseData fbdo;
FirebaseData fbdo_temp_menu; // Objeto separado para leituras do menu

/* 4. Define os dados FirebaseAuth para autenticação */
FirebaseAuth auth;

/* Define os dados FirebaseConfig para configuração */
FirebaseConfig config;

// --- Configurações IR Send (Envio) ---
const uint16_t kIrLed = 2; // ESP8266/ESP32 GPIO pin to use. Recommended: 4 (D2).
IRsend irsend(kIrLed);     // Define o GPIO a ser usado para enviar a mensagem.

// --- Configurações IR Receive (Receptor) ---
const uint16_t kRecvPin = 4; // GPIO 4 é um bom pino para o ESP32
const uint32_t kBaudRate = 115200;
const uint16_t kCaptureBufferSize = 2048; // Buffer maior para AC
const uint8_t kTimeout = 50; // Um pouco maior para garantir a captura de AC
const uint8_t kTolerancePercentage = kTolerance; // kTolerance é normalmente 25%

IRrecv irrecv(kRecvPin, kCaptureBufferSize, kTimeout, true);
decode_results results; // Onde armazenar os resultados

// Variáveis para armazenar o código IR lido/capturado
const int MAX_IR_DATA_SIZE = 2000;
uint16_t currentLoadedRawData[MAX_IR_DATA_SIZE];
int currentLoadedSize = 0;
String currentDescription = "";
int currentStatusVazioOcupado = 1; // 1 = Vazio, 2 = Ocupado

// Estados do programa
enum AppState {
    STATE_CONNECTING,
    STATE_MAIN_MENU,
    STATE_COMMAND_MENU,
    STATE_RECEIVE_MODE,
    STATE_SEND_MODE
};

AppState currentState = STATE_CONNECTING;
int selectedCommand = 0; // Armazena o número do comando selecionado (1 a 5)

// Removidas as variáveis de inatividade
// unsigned long lastActivityMillis = 0;
// const unsigned long INACTIVITY_TIMEOUT = 30000; // 30 segundos para voltar ao menu principal se inativo

// --- Funções Auxiliares ---

// Função para converter o array uint16_t em string separada por vírgulas
String convertRawDataToString(uint16_t* arr, uint16_t size) {
    String result = "";
    for (uint16_t i = 0; i < size; i++) {
        result += String(arr[i]);
        if (i < size - 1) {
            result += ","; // Adiciona vírgula como delimitador
        }
    }
    return result;
}

// Função para converter string de volta para array de uint16_t
void convertStringToArray(String str, uint16_t* arr, int& size) {
    size = 0;
    char tempCharArray[str.length() + 1]; 
    str.toCharArray(tempCharArray, sizeof(tempCharArray));

    char* ptr = strtok(tempCharArray, ",");
    while (ptr != NULL && size < MAX_IR_DATA_SIZE) {
        arr[size++] = atoi(ptr);
        ptr = strtok(NULL, ",");
    }
}

// Função para exibir o menu principal (Otimizado para velocidade)
void displayMainMenu() {
    Serial.println("\n--- MENU PRINCIPAL ---");
    Serial.println("Escolha um COMANDO (1-5) para acessar:");
    for (int i = 1; i <= 5; i++) {
        String pathDesc = "comando_" + String(i) + "/descricao";
        String description = "Vazio"; // Default

        if (Firebase.ready()) {
            if (Firebase.getString(fbdo_temp_menu, pathDesc)) { // Usando fbdo_temp_menu
                if (fbdo_temp_menu.dataType() == "string") {
                    description = fbdo_temp_menu.stringData();
                    if (description.isEmpty()) description = "Gravado (sem descricao)";
                } else {
                    description = "Gravado (descricao invalida)";
                }
            } else {
                description = "Vazio";
            }
        } else {
            description = "Firebase Offline";
        }
        Serial.printf(" %d - Comando %d: %s\n", i, i, description.c_str());
        yield(); // Alimenta o WDT
    }
    Serial.println("Digite o número do comando e pressione Enter:");
}


// Função para ler dados de um comando específico do Firebase
void loadCommandFromFirebase(int cmdNum) {
    String basePath = "comando_" + String(cmdNum);
    
    currentLoadedSize = 0;
    currentDescription = "N/A";
    currentStatusVazioOcupado = 1; // Assume vazio por padrão (1)

    Serial.printf("\nCarregando dados para %s do Firebase...\n", basePath.c_str());

    if (!Firebase.ready()) {
        Serial.println("Firebase não está pronto. Verifique a conexão.");
        return;
    }

    // Acessando os subnós diretamente com fbdo
    // Ler 'cont'
    if (Firebase.getInt(fbdo, basePath + "/cont")) {
        if (fbdo.dataType() == "int") {
            currentLoadedSize = fbdo.intData();
        } else {
            Serial.printf("AVISO: '%s/cont' nao e um inteiro (tipo: %s).\n", basePath.c_str(), fbdo.dataType().c_str());
        }
    } else {
        Serial.printf("AVISO: Falha ao ler '%s/cont': %s\n", basePath.c_str(), fbdo.errorReason().c_str());
    }
    yield();

    // Ler 'ir_code'
    if (Firebase.getString(fbdo, basePath + "/ir_code")) {
        if (fbdo.dataType() == "string") {
            String irCodeString = fbdo.stringData();
            if (!irCodeString.isEmpty() && currentLoadedSize > 0 && currentLoadedSize <= MAX_IR_DATA_SIZE) {
                convertStringToArray(irCodeString, currentLoadedRawData, currentLoadedSize);
            } else {
                currentLoadedSize = 0; // Se a string estiver vazia ou o tamanho for inválido
                Serial.printf("AVISO: '%s/ir_code' vazio ou tamanho invalido para conversao.\n", basePath.c_str());
            }
        } else {
            Serial.printf("AVISO: '%s/ir_code' nao e uma string (tipo: %s).\n", basePath.c_str(), fbdo.dataType().c_str());
        }
    } else {
        Serial.printf("AVISO: Falha ao ler '%s/ir_code': %s\n", basePath.c_str(), fbdo.errorReason().c_str());
    }
    yield();

    // Ler 'descricao'
    if (Firebase.getString(fbdo, basePath + "/descricao")) {
        if (fbdo.dataType() == "string") {
            currentDescription = fbdo.stringData();
        } else {
            Serial.printf("AVISO: '%s/descricao' nao e uma string (tipo: %s).\n", basePath.c_str(), fbdo.dataType().c_str());
        }
    } else {
        Serial.printf("AVISO: Falha ao ler '%s/descricao': %s\n", basePath.c_str(), fbdo.errorReason().c_str());
    }
    yield();

    // Ler 'vazio' (agora como int)
    if (Firebase.getInt(fbdo, basePath + "/vazio")) {
        if (fbdo.dataType() == "int") {
            currentStatusVazioOcupado = fbdo.intData();
        } else {
            Serial.printf("AVISO: '%s/vazio' nao e um inteiro (tipo: %s).\n", basePath.c_str(), fbdo.dataType().c_str());
        }
    } else {
        Serial.printf("AVISO: Falha ao ler '%s/vazio': %s\n", basePath.c_str(), fbdo.errorReason().c_str());
    }
    yield();

    Serial.println("Dados carregados:");
    Serial.printf("  Tamanho (cont): %d\n", currentLoadedSize);
    Serial.printf("  Descricao: %s\n", currentDescription.c_str());
    Serial.printf("  Vazio/Ocupado Status: %d (%s)\n", currentStatusVazioOcupado, (currentStatusVazioOcupado == 1 ? "Vazio" : (currentStatusVazioOcupado == 2 ? "Ocupado" : "Desconhecido")));
    Serial.println("-------------------------");
}

// Função para exibir o menu de um comando específico
void displayCommandMenu() {
    Serial.printf("\n--- COMANDO %d SELECIONADO ---\n", selectedCommand);
    Serial.printf("Descricao: %s\n", currentDescription.c_str());
    Serial.printf("Status: %s (Tamanho: %d)\n", (currentStatusVazioOcupado == 1 ? "VAZIO" : (currentStatusVazioOcupado == 2 ? "OCUPADO" : "DESCONHECIDO")), currentLoadedSize);

    if (currentStatusVazioOcupado == 1) { // 1 = Vazio
        Serial.println("Opcoes: (C)opiar sinal IR, (P)arar e voltar ao menu principal");
    } else { // 2 = Ocupado
        Serial.println("Opcoes: (E)nviar sinal IR, (C)opiar novo sinal IR, (P)arar e voltar ao menu principal");
    }
    Serial.println("Digite sua opcao e pressione Enter:");
}

// Função para iniciar o modo de cópia
void enterReceiveMode() {
    currentState = STATE_RECEIVE_MODE;
    irrecv.enableIRIn(); // Garante que o receptor está ativo
    Serial.println("\n--- MODO COPIAR SINAL IR ---");
    Serial.println("Aponte o controle remoto para o sensor IR e pressione o botao desejado.");
    Serial.println("Aguardando sinal...");
    Serial.println("(P) para Parar e voltar ao menu do comando.");
}

// Função para salvar o sinal IR capturado no Firebase
void saveIrToFirebase(int cmdNum, uint16_t* rawData, uint16_t rawSize) {
    String basePath = "comando_" + String(cmdNum);
    String irCodeString = convertRawDataToString(rawData, rawSize);

    Serial.printf("Salvando sinal IR para %s no Firebase...\n", basePath.c_str());

    if (!Firebase.ready()) {
        Serial.println("Firebase não está pronto. Verifique a conexão.");
        return;
    }

    // Salvar 'cont'
    if (Firebase.setInt(fbdo, basePath + "/cont", rawSize)) {
        Serial.println("Firebase: 'cont' salvo com sucesso.");
    } else {
        Serial.printf("Firebase: Falha ao salvar 'cont': %s\n", fbdo.errorReason().c_str());
    }
    yield();

    // Salvar 'ir_code'
    if (Firebase.setString(fbdo, basePath + "/ir_code", irCodeString)) {
        Serial.println("Firebase: 'ir_code' salvo com sucesso.");
    } else {
        Serial.printf("Firebase: Falha ao salvar 'ir_code': %s\n", fbdo.errorReason().c_str());
    }
    yield();

    // Salvar 'vazio' como 2 (Ocupado)
    if (Firebase.setInt(fbdo, basePath + "/vazio", 2)) { // 2 = Ocupado
        Serial.println("Firebase: 'vazio' atualizado para Ocupado (2).");
        currentStatusVazioOcupado = 2; // Atualiza o status local
    } else {
        Serial.printf("Firebase: Falha ao atualizar 'vazio': %s\n", fbdo.errorReason().c_str());
    }
    yield();

    // Tenta salvar/atualizar 'descricao' se estiver vazio ou N/A
    if (currentDescription.isEmpty() || currentDescription == "Vazio" || currentDescription == "N/A" || currentDescription == "Sinal Gravado") {
        String newDescription = "Comando " + String(cmdNum) + " Gravado"; // Descrição padrão mais útil
        if (Firebase.setString(fbdo, basePath + "/descricao", newDescription)) {
            Serial.printf("Firebase: 'descricao' atualizado para '%s'.\n", newDescription.c_str());
            currentDescription = newDescription;
        } else {
            Serial.printf("Firebase: Falha ao atualizar 'descricao': %s\n", fbdo.errorReason().c_str());
        }
    }
    yield();

    Serial.println("Salvamento no Firebase finalizado.");
}

// Função para enviar o sinal IR
void sendIrCommand() {
    Serial.println("\n--- MODO ENVIAR SINAL IR ---");

    if (currentLoadedSize > 0) {
        Serial.printf("Enviando sinal IR (tamanho: %d) para Comando %d...\n", currentLoadedSize, selectedCommand);
        // A frequência de 38kHz é a mais comum. Se sua TV usa outra, ajuste aqui.
        irsend.sendRaw(currentLoadedRawData, currentLoadedSize, 36);
        irsend.sendRaw(currentLoadedRawData, currentLoadedSize, 38);
        irsend.sendRaw(currentLoadedRawData, currentLoadedSize, 40);
        irsend.sendRaw(currentLoadedRawData, currentLoadedSize, 56);
        Serial.println("Sinal IR enviado.");
    } else {
        Serial.println("Nenhum sinal IR para enviar. O comando esta vazio ou o sinal e invalido.");
    }

    Serial.println("Pressione (P) para Parar e voltar ao menu do comando.");
}

// Loop para lidar com entrada serial do usuário
void handleSerialInput() {
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        input.toUpperCase();

        // Removida a linha de reset de lastActivityMillis

        switch (currentState) {
            case STATE_MAIN_MENU: {
                int cmd = input.toInt();
                if (cmd >= 1 && cmd <= 5) {
                    selectedCommand = cmd;
                    loadCommandFromFirebase(selectedCommand);
                    currentState = STATE_COMMAND_MENU;
                    displayCommandMenu();
                } else {
                    Serial.println("Opcao invalida. Digite um numero de 1 a 5.");
                }
                break;
            }
            case STATE_COMMAND_MENU: {
                if (input == "C") {
                    enterReceiveMode();
                } else if (input == "E") {
                    if (currentLoadedSize > 0) {
                        sendIrCommand();
                    } else {
                        Serial.println("Nenhum sinal gravado para enviar neste comando. Escolha 'C' para copiar.");
                    }
                } else if (input == "P") {
                    selectedCommand = 0;
                    currentState = STATE_MAIN_MENU;
                    displayMainMenu();
                } else {
                    Serial.println("Opcao invalida. Digite 'E', 'C' ou 'P'.");
                }
                break;
            }
            case STATE_RECEIVE_MODE: {
                if (input == "P") {
                    irrecv.disableIRIn();
                    Serial.println("Modo Copiar desativado.");
                    currentState = STATE_COMMAND_MENU;
                    displayCommandMenu();
                } else {
                    Serial.println("Modo Copiar ativo. Pressione 'P' para parar ou aguarde um sinal IR.");
                }
                break;
            }
            case STATE_SEND_MODE: {
                 if (input == "P") {
                    currentState = STATE_COMMAND_MENU;
                    displayCommandMenu();
                } else if (input == "E") {
                    if (currentLoadedSize > 0) {
                        sendIrCommand();
                    } else {
                        Serial.println("Nenhum sinal IR para enviar. O comando esta vazio ou o sinal e invalido.");
                    }
                } else {
                    Serial.println("Sinal IR ja enviado. Pressione 'P' para parar e voltar, ou 'E' para reenviar.");
                }
                break;
            }
            default:
                break;
        }
    }
}

void setup() {
    Serial.begin(kBaudRate);
    while (!Serial) delay(50);

    Serial.print("Connecting to Wi-Fi");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(300);
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();

    Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

    config.database_url = DATABASE_URL;
    config.signer.tokens.legacy_token = DATABASE_SECRET;
    Firebase.reconnectNetwork(true);
    fbdo.setBSSLBufferSize(4096, 1024); // buffer principal
    fbdo_temp_menu.setBSSLBufferSize(1024, 512); // buffer menor para leituras do menu

    Firebase.begin(&config, &auth);
    Serial.println("Firebase initialized.");

    irsend.begin();
    Serial.println("IRsend initialized.");

    irrecv.setTolerance(kTolerancePercentage);
    Serial.println("IR Receiver ready.");

    currentState = STATE_MAIN_MENU;
    displayMainMenu();
    // Removida a inicialização de lastActivityMillis
}

void loop() {
    handleSerialInput();

    switch (currentState) {
        case STATE_CONNECTING:
            break;
        case STATE_MAIN_MENU:
            break;
        case STATE_COMMAND_MENU:
            break;
        case STATE_RECEIVE_MODE:
            if (irrecv.decode(&results)) {
                Serial.println("\n--- SINAL IR CAPTURADO! ---");
                uint32_t now = millis();
                Serial.printf(D_STR_TIMESTAMP " : %06u.%03u\n", now / 1000, now % 1000);
                if (results.overflow) Serial.printf(D_WARN_BUFFERFULL "\n", kCaptureBufferSize);
                Serial.println(D_STR_LIBRARY "   : v" _IRREMOTEESP8266_VERSION_STR "\n");
                Serial.print(resultToHumanReadableBasic(&results));
                String description = IRAcUtils::resultAcToString(&results);
                if (description.length()) Serial.println(D_STR_MESGDESC ": " + description);
                yield();

                uint16_t* rawData = resultToRawArray(&results);
                uint16_t rawSize = results.rawlen;

                Serial.printf("Tamanho RAW capturado: %d\n", rawSize);
                Serial.print("Array RAW capturado: {");
                for (uint16_t i = 0; i < rawSize; i++) {
                    Serial.print(rawData[i]);
                    if (i < rawSize - 1) Serial.print(", ");
                }
                Serial.println("}\n");
                if (Firebase.ready()) {
                    saveIrToFirebase(selectedCommand, rawData, rawSize);
                } else {
                    Serial.println("Firebase nao esta pronto, nao foi possivel salvar o sinal.");
                }

                delete[] rawData;
                
                irrecv.disableIRIn();
                Serial.println("Receptor IR desativado apos captura.");
                currentState = STATE_COMMAND_MENU;
                displayCommandMenu();
            }
            break;
        case STATE_SEND_MODE:
            break;
    }

    delay(10);
}
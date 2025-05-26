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

/* Define as credenciais do WiFi */
#define WIFI_SSID "CLARO_2GEEC2D3"
#define WIFI_PASSWORD "06EEC2D3"

/* Define a URL do RTDB e o segredo do banco de dados */
#define DATABASE_URL "https://ventilai-default-rtdb.firebaseio.com/"
#define DATABASE_SECRET "h6Y4SoLoaAKtrPwGYuPosiqq0r2qbWlOsdlVIB1X"

/* Define o objeto Firebase Data */
FirebaseData fbdo;

/* Define os dados FirebaseAuth para autenticação */
FirebaseAuth auth;

/* Define os dados FirebaseConfig para configuração */
FirebaseConfig config;

// --- Configurações IR Send (Envio) ---
const uint16_t kIrLed = 2;
IRsend irsend(kIrLed);

// --- Configurações IR Receive (Receptor) ---
const uint16_t kRecvPin = 4;
const uint32_t kBaudRate = 115200;
const uint16_t kCaptureBufferSize = 2048;
const uint8_t kTimeout = 50;
const uint8_t kTolerancePercentage = kTolerance;

IRrecv irrecv(kRecvPin, kCaptureBufferSize, kTimeout, true);
decode_results results;

const int MAX_IR_DATA_SIZE = 2000;
uint16_t currentLoadedRawData[MAX_IR_DATA_SIZE];
int currentLoadedSize = 0;
String currentDescription = "";
int currentStatusVazioOcupado = 1; // 1 = Vazio, 2 = Ocupado

uint16_t transmitRawDataBuffer[MAX_IR_DATA_SIZE];
int transmitRawSize = 0;

enum AppState
{
    STATE_CONNECTING,
    STATE_MAIN_MENU,
    STATE_RECEIVE_MODE,
    STATE_DEVICE_MENU
};

AppState currentState = STATE_CONNECTING;

const int MAX_MENU_DEVICES = 10;
String menuDeviceUUIDs[MAX_MENU_DEVICES];
String menuDeviceNames[MAX_MENU_DEVICES];
int menuDeviceCount = 0;
String currentDeviceUUID = "";
String currentDeviceName = "";

String streamCaptureUUID = "";

const int MAX_LOGS = 100; 
unsigned long lastLogCleanup = 0;
const unsigned long LOG_CLEANUP_INTERVAL = 3600000; 

String convertRawDataToString(uint16_t* arr, uint16_t size) {
    String result = "";
    for (uint16_t i = 0; i < size; i++) {
        result += String(arr[i]);
        if (i < size - 1) {
            result += ",";
        }
    }
    return result;
}

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

void displayMainMenu()
{
    Serial.println("\n--- DISPOSITIVOS ---");
    menuDeviceCount = 0;

    if (!Firebase.ready())
    {
        Serial.println("Firebase não está pronto. Não foi possível carregar dispositivos.");
        currentState = STATE_MAIN_MENU;
        return;
    }

    Serial.println("DEBUG: Fetching /devices for menu listing...");
    FirebaseData devices_fbdo;
    devices_fbdo.setBSSLBufferSize(16384, 4096);

    if (Firebase.getJSON(devices_fbdo, "/devices"))
    {
        Serial.println("DEBUG: Fetched /devices.");
        if (devices_fbdo.dataType() == "json")
        {
            Serial.println("DEBUG: Data type is json.");
            FirebaseJson json;
            json.setJsonData(devices_fbdo.jsonString());
            size_t len = json.iteratorBegin();
            Serial.printf("DEBUG: Found %d items under /devices.\n", len);

            if (len == 0)
            {
                Serial.println("Nenhum dispositivo encontrado no Firebase.");
            }
            else
            {
                for (size_t i = 0; i < len; i++)
                {
                    FirebaseJson::IteratorValue value = json.valueAt(i);
                    // Only process if it's an object (a device) and not a known field
                    // Store ONLY the UUID (value.key) and display that initially
                    if (value.type == FirebaseJson::JSON_OBJECT &&
                        menuDeviceCount < MAX_MENU_DEVICES &&
                        value.key != "code_size" &&
                        value.key != "description" &&
                        value.key != "empty" &&
                        value.key != "ir_code" &&
                        value.key != "name")
                    {

                        String uuid = value.key;
                        // Do NOT fetch the name here to save memory

                        menuDeviceUUIDs[menuDeviceCount] = uuid;
                        // Store UUID as name placeholder for initial display
                        menuDeviceNames[menuDeviceCount] = uuid;
                        Serial.printf("%d. %s\n", menuDeviceCount + 1, uuid.c_str()); // Display UUID
                        menuDeviceCount++;
                    }
                }
            }
            json.iteratorEnd();
        }
        else
        {
            Serial.println("AVISO: /devices nao e um JSON valido.");
        }
    }
    else
    {
        Serial.printf("ERRO ao carregar dispositivos: %s\n", devices_fbdo.errorReason().c_str());
    }

    Serial.println("------------------------");
    Serial.println("Digite o numero do dispositivo:");
}

void displayDeviceMenu()
{
    // Fetch the name here when entering the device menu
    String nameToDisplay = currentDeviceUUID;
    FirebaseData name_fbdo; // Use a smaller temporary object
    name_fbdo.setBSSLBufferSize(1024, 512);
    if (Firebase.getString(name_fbdo, "/devices/" + currentDeviceUUID + "/name"))
    {
        if (name_fbdo.dataType() == "string")
        {
            String fetchedName = name_fbdo.stringData();
            if (fetchedName.length() > 0)
            {
                nameToDisplay = fetchedName;
                currentDeviceName = fetchedName; // Update global name
            }
        }
    }
    Serial.printf("\n--- %s MENU ---\n", nameToDisplay.c_str());
    Serial.println("(R)egistrar Sinal");
    Serial.println("(T)ransmitir Sinal");
    Serial.println("(V)oltar para dispositivos");
    Serial.println("Digite sua opção:");
}

void enterReceiveMode()
{
    currentState = STATE_RECEIVE_MODE;
    irrecv.enableIRIn();
    Serial.println("\n--- MODO COPIAR SINAL IR (SERIAL) ---");
    Serial.println("Aponte o controle remoto para o sensor IR e pressione o botao desejado.");
    Serial.println("Aguardando sinal...");
    Serial.println("(P) para Parar e voltar ao menu do comando.");
}

void saveIrToFirebase(String uuid, uint16_t *rawData, uint16_t rawSize)
{
    String devicePath = "/devices/" + uuid;

    String irCodeString = "";
    for (uint16_t i = 0; i < rawSize; i++)
    {
        irCodeString += String(rawData[i]);
        if (i < rawSize - 1)
            irCodeString += ",";
    }

    Serial.printf("Salvando sinal IR para dispositivo %s no Firebase...\n", uuid.c_str());

    const int MAX_RETRIES = 3;
    int retryCount = 0;
    bool success = false;

    while (retryCount < MAX_RETRIES && !success)
    {
        if (!Firebase.ready())
        {
            Serial.println("Firebase não está pronto. Tentando reconectar...");
            Firebase.reconnectNetwork(true);
            delay(2000);
            retryCount++;
            continue;
        }

        Firebase.reconnectNetwork(true);
        delay(500);

        bool saveSuccess = true;

        if (!Firebase.setString(fbdo, devicePath + "/ir_code", irCodeString))
        {
            Serial.printf("Firebase: Falha ao salvar 'ir_code': %s\n", fbdo.errorReason().c_str());
            saveSuccess = false;
        }
        yield();
        delay(100);

        if (saveSuccess)
        {
            if (!Firebase.setInt(fbdo, devicePath + "/code_size", rawSize))
            {
                Serial.printf("Firebase: Falha ao salvar 'code_size': %s\n", fbdo.errorReason().c_str());
                saveSuccess = false;
            }
            yield();
            delay(100);
        }

        if (saveSuccess)
        {
            if (!Firebase.setBool(fbdo, devicePath + "/empty", false))
            {
                Serial.printf("Firebase: Falha ao atualizar 'empty': %s\n", fbdo.errorReason().c_str());
                saveSuccess = false;
            }
            yield();
            delay(100);
        }

        if (saveSuccess)
        {
            success = true;
            Serial.println("Salvamento no Firebase finalizado com sucesso.");
        }
        else
        {
            retryCount++;
            Serial.printf("Tentativa %d de %d falhou. Tentando novamente...\n", retryCount, MAX_RETRIES);
            delay(2000);
        }
    }

    if (!success)
    {
        Serial.println("ERRO: Falha ao salvar no Firebase após várias tentativas.");
    }
}

void cleanupOldLogs();
void handleSerialInput();
void requestTransmit(String uuid, String signal);

FirebaseData captureStream;
FirebaseData transmitStream;
bool captureStreamStarted = false;
bool transmitStreamStarted = false;

void addLog(String message, String type = "info")
{
    if (!Firebase.ready())
    {
        Serial.println("Firebase não está pronto para adicionar log");
        return;
    }

    String timestamp = String(millis());

    FirebaseJson json;
    json.add("message", message);
    json.add("type", type);
    json.add("timestamp", timestamp);

    const int MAX_RETRIES = 3;
    int retryCount = 0;
    bool success = false;

    while (retryCount < MAX_RETRIES && !success)
    {
        String logPath = "/logs/" + timestamp;
        if (Firebase.setJSON(fbdo, logPath, json))
        {
            success = true;
        }
        else
        {
            retryCount++;
            delay(1000);
        }
    }

    if (millis() - lastLogCleanup > LOG_CLEANUP_INTERVAL)
    {
        cleanupOldLogs();
        lastLogCleanup = millis();
    }
}

void cleanupOldLogs()
{
    if (!Firebase.ready())
        return;

    if (Firebase.getJSON(fbdo, "/logs"))
    {
        FirebaseJson json;
        json.setJsonData(fbdo.jsonString());

        size_t count = json.iteratorBegin();
        if (count > MAX_LOGS)
        {
            size_t toRemove = count - MAX_LOGS;
            size_t removed = 0;

            for (size_t i = 0; i < count && removed < toRemove; i++)
            {
                FirebaseJson::IteratorValue value = json.valueAt(i);
                if (value.type == FirebaseJson::JSON_OBJECT)
                {
                    String path = "/logs/" + value.key;
                    Firebase.deleteNode(fbdo, path);
                    removed++;
                }
            }
        }
        json.iteratorEnd();
    }
}

// Modifique a função streamCallback para usar o novo sistema de logs
void streamCallback(StreamData data)
{
    Serial.println("\n=== NOVA REQUISIÇÃO RECEBIDA ===");
    String path = data.dataPath();
    String type = data.dataType();

    // Debug prints for data type and value
    Serial.println("DEBUG - Data Type: [" + type + "]");
    Serial.println("DEBUG - Raw Value: [" + data.stringData() + "]");
    Serial.println("DEBUG - Boolean Value: [" + String(data.boolData() ? "true" : "false") + "]");
    Serial.println("DEBUG - Full Path: [" + path + "]");

    String logMessage = String("[STREAM] Recebido: ") + path + " (tipo: " + type + ")";
    if (type != "binary" && type != "blob")
    {
        logMessage += " = " + data.stringData();
    }
    addLog(logMessage, "stream");

    // Process request if path exists and data is boolean true
    if (path.length() > 0 && (type == "boolean" || type == "bool") && data.boolData())
    {
        Serial.println("DEBUG - Valid boolean true request.");
        // Remove leading slash if present
        String uuid = path;
        if (uuid.startsWith("/"))
        {
            uuid = uuid.substring(1);
        }
        Serial.println("DEBUG - UUID (cleaned): [" + uuid + "]");

        bool isCapture = (data.streamPath() == "/captureRequests");
        Serial.println("DEBUG - Stream Path: [" + data.streamPath() + "]");
        Serial.println("DEBUG - Is Capture: " + String(isCapture ? "true" : "false"));

        if (isCapture)
        {
            Serial.printf("DEBUG - Capture request. Setting streamCaptureUUID.\n");
            streamCaptureUUID = uuid; // Store the UUID requesting capture
            addLog("Stream: Habilitado modo de captura para " + uuid, "info");
            enterReceiveMode(); // Use the centralized function instead of direct state change
        }
        else
        { // isTransmit
            Serial.printf("DEBUG - Transmit request. Getting ir_code from Firebase.\n");
            // Get signal (ir_code) from device (assuming ir_code field exists)
            String devicePath = "/devices/" + uuid;
            FirebaseData signal_fbdo;

            Serial.printf("DEBUG - Attempting Firebase.getString(%s/ir_code)...\n", devicePath.c_str());
            if (Firebase.getString(signal_fbdo, devicePath + "/ir_code"))
            {
                Serial.println("DEBUG - Firebase.getString successful.");
                String signal = signal_fbdo.stringData(); // This is the ir_code string
                if (signal.length() > 0)
                {
                    Serial.println("DEBUG - Signal found, converting to buffer and calling requestTransmit.");
                    Serial.println("Sinal encontrado, iniciando transmissão...");
                    
                    // Convert signal string to global transmit buffer
                    convertStringToArray(signal, transmitRawDataBuffer, transmitRawSize);

                    if (transmitRawSize > 0) {
                       requestTransmit(uuid, signal); // Call requestTransmit with UUID and original signal string (UUID used for logging/Firebase update)
                    } else {
                       Serial.println("ERRO: Conversao do sinal falhou ou resultou em tamanho zero.");
                       addLog("ERRO: Falha na conversao do sinal para transmissao do dispositivo " + uuid, "error");
                       // Set transmit request back to false if conversion fails
                       Firebase.setBool(fbdo, "/transmitRequests/" + uuid, false);
                    }
                }
                else
                {
                    Serial.println("ERRO: Sinal vazio para transmissão no Firebase.");
                    addLog("ERRO: Sinal vazio para transmissão do dispositivo " + uuid, "error");
                    // Set transmit request back to false if signal is empty
                    Firebase.setBool(fbdo, "/transmitRequests/" + uuid, false);
                }
            }
            else
            {
                // Log error if ir_code field doesn't exist or read error
                Serial.printf("ERRO: Falha ao obter sinal para transmissão. %s\n", signal_fbdo.errorReason().c_str());
                addLog("ERRO: Falha ao obter sinal para transmissão do dispositivo " + uuid + ". Campo 'ir_code' não encontrado ou erro de leitura: " + signal_fbdo.errorReason(), "error");
                // Set transmit request back to false on error
                Firebase.setBool(fbdo, "/transmitRequests/" + uuid, false);
            }
            Serial.println("DEBUG - Finished transmit request handling in streamCallback.");
        }
    }
    else
    {
        Serial.println("DEBUG - Not a valid boolean true request.");
    }
    Serial.println("=== FIM DO PROCESSAMENTO DA REQUISIÇÃO ===\n");
}

void streamTimeoutCallback(bool timeout)
{
    if (timeout)
    {
        String logMessage = "[STREAM] Timeout na conexão do stream";
        addLog(logMessage, "error");
        Serial.println(logMessage); // Manter este no serial para debug de conexão
    }
}

void startFirebaseStreams()
{
    if (!Firebase.ready())
    {
        String logMessage = "[STREAM] Firebase não está pronto para iniciar streams";
        addLog(logMessage, "error");
        return;
    }

    if (!captureStreamStarted)
    {
        if (Firebase.beginStream(captureStream, "/captureRequests"))
        {
            captureStreamStarted = true;
            Firebase.setStreamCallback(captureStream, streamCallback, streamTimeoutCallback);
            String logMessage = "[STREAM] Stream de captura iniciado";
            addLog(logMessage, "info");
        }
        else
        {
            String logMessage = String("[STREAM] Erro ao iniciar stream de captura: ") + captureStream.errorReason();
            addLog(logMessage, "error");
        }
    }

    if (!transmitStreamStarted)
    {
        if (Firebase.beginStream(transmitStream, "/transmitRequests"))
        {
            transmitStreamStarted = true;
            Firebase.setStreamCallback(transmitStream, streamCallback, streamTimeoutCallback);
            String logMessage = "[STREAM] Stream de transmissão iniciado";
            addLog(logMessage, "info");
        }
        else
        {
            String logMessage = String("[STREAM] Erro ao iniciar stream de transmissão: ") + transmitStream.errorReason();
            addLog(logMessage, "error");
        }
    }
}

void setup()
{
    Serial.begin(kBaudRate);
    while (!Serial)
        delay(50);

    Serial.print("Conectando ao WiFi");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(300);
    }
    Serial.println();
    Serial.print("Conectado com IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();

    addLog("Firebase Client v" + String(FIREBASE_CLIENT_VERSION), "info");

    config.database_url = DATABASE_URL;
    config.signer.tokens.legacy_token = DATABASE_SECRET;

    fbdo.setBSSLBufferSize(32768, 8192);

    config.timeout.serverResponse = 30 * 1000;
    config.timeout.socketConnection = 30 * 1000;
    config.timeout.sslHandshake = 30 * 1000;

    Firebase.reconnectNetwork(true);
    Firebase.begin(&config, &auth);

    int retryCount = 0;
    while (!Firebase.ready() && retryCount < 5)
    {
        addLog("Aguardando Firebase estar pronto...", "info");
        delay(1000);
        retryCount++;
    }

    if (Firebase.ready())
    {
        addLog("Firebase inicializado com sucesso", "info");
        if (!Firebase.getJSON(fbdo, "/logs"))
        {
            FirebaseJson json;
            json.setJsonData("{}");
            Firebase.setJSON(fbdo, "/logs", json);
        }
        startFirebaseStreams();
        Serial.println("Sistema inicializado. Aguardando comandos...");
    }
    else
    {
        addLog("Aviso: Inicialização do Firebase pode estar incompleta", "error");
    }

    irsend.begin();
    Serial.println("IRsend initialized.");

    irrecv.setTolerance(kTolerancePercentage);
    Serial.println("IR Receiver ready.");

    currentState = STATE_MAIN_MENU;
    displayMainMenu();
}

void loop()
{
    handleSerialInput();

    if (Firebase.ready())
    {
        if (!captureStreamStarted || !transmitStreamStarted)
        {
            startFirebaseStreams();
        }
    }

    switch (currentState) {
        case STATE_CONNECTING:
            break;
        case STATE_MAIN_MENU:
            break;
        case STATE_DEVICE_MENU:
            break;
        case STATE_RECEIVE_MODE:
            if (irrecv.decode(&results)) {
                Serial.println("\nDEBUG: IR signal detected!");
                Serial.println("\n=== SINAL IR CAPTURADO! ===");
                uint32_t now = millis();
                Serial.printf(D_STR_TIMESTAMP " : %06u.%03u\n", now / 1000, now % 1000);
                if (results.overflow)
                    Serial.printf(D_WARN_BUFFERFULL "\n", kCaptureBufferSize);
                Serial.println(D_STR_LIBRARY "   : v" _IRREMOTEESP8266_VERSION_STR "\n");
                Serial.print(resultToHumanReadableBasic(&results));
                String description = IRAcUtils::resultAcToString(&results);
                if (description.length())
                    Serial.println(D_STR_MESGDESC ": " + description);
                yield();

                uint16_t *rawData = resultToRawArray(&results);
                uint16_t rawSize = results.rawlen;

                Serial.printf("DEBUG: Raw size: %d\n", rawSize);
                Serial.print("DEBUG: Raw data: {");
                for (uint16_t i = 0; i < rawSize; i++) {
                    Serial.print(rawData[i]);
                    if (i < rawSize - 1)
                        Serial.print(", ");
                }
                Serial.println("}");

                if (Firebase.ready()) {
                    Serial.println("\nDEBUG: Firebase ready, saving signal...");

                    if (!streamCaptureUUID.isEmpty()) {
                        Serial.printf("DEBUG: Saving signal for stream capture UUID: %s\n", streamCaptureUUID.c_str());
                        saveIrToFirebase(streamCaptureUUID, rawData, rawSize);
                        streamCaptureUUID = "";
                        Serial.println("DEBUG: Signal saved, returning to main menu");
                        currentState = STATE_MAIN_MENU;
                        displayMainMenu();
                    }
                    else if (!currentDeviceUUID.isEmpty()) {
                        Serial.printf("DEBUG: Saving signal for serial capture UUID: %s\n", currentDeviceUUID.c_str());
                        saveIrToFirebase(currentDeviceUUID, rawData, rawSize);
                        currentState = STATE_DEVICE_MENU;
                        displayDeviceMenu();
                        Serial.println("DEBUG: Signal saved, returning to device menu");
                    }
                    else {
                        Serial.println("DEBUG: No active capture request, ignoring signal");
                        addLog("AVISO: Sinal IR capturado sem requisição ativa.", "warning");
                        currentState = STATE_MAIN_MENU;
                        displayMainMenu();
                    }
                }
                else
                {
                    Serial.println("\nERRO: Firebase não está pronto, não foi possível salvar o sinal.");
                    currentState = STATE_MAIN_MENU;
                    displayMainMenu();
                }

                delete[] rawData;
                irrecv.disableIRIn();
                Serial.println("Receptor IR desativado após captura.");
            }
            break;
    }

    delay(10);
}

void handleSerialInput()
{
    if (Serial.available())
    {
        String input = Serial.readStringUntil('\n');
        input.trim();
        input.toUpperCase();

        switch (currentState)
        {
        case STATE_CONNECTING:
            break;
        case STATE_MAIN_MENU:
        {
            int deviceNum = input.toInt();
            if (deviceNum > 0 && deviceNum <= menuDeviceCount)
            {
                currentDeviceUUID = menuDeviceUUIDs[deviceNum - 1];
                currentState = STATE_DEVICE_MENU;
                displayDeviceMenu();
            }
            else
            {
                Serial.println("Opção inválida. Digite o numero do dispositivo.");
            }
        }
        break;
        case STATE_DEVICE_MENU:
            if (input == "R")
            {
                Serial.println("Iniciando modo de captura...");
                enterReceiveMode();
            }
            else if (input == "T")
            {
                Serial.println("Iniciando transmissão...");
                String devicePath = "/devices/" + currentDeviceUUID;
                Serial.printf("DEBUG: Reading from path: %s\n", devicePath.c_str());
                
                FirebaseData signal_fbdo;
                signal_fbdo.setBSSLBufferSize(4096, 1024);
                
                Serial.println("DEBUG: Attempting to read ir_code from Firebase...");
                if (Firebase.getString(signal_fbdo, devicePath + "/ir_code"))
                {
                    Serial.println("DEBUG: Firebase.getString successful");
                    String signal = signal_fbdo.stringData();
                    Serial.printf("DEBUG: Signal length: %d\n", signal.length());
                    Serial.printf("DEBUG: Signal content: %s\n", signal.c_str());
                    
                    if (signal.length() > 0)
                    {
                        Serial.println("DEBUG: Signal found, calling requestTransmit");
                        requestTransmit(currentDeviceUUID, signal);
                    }
                    else
                    {
                        Serial.println("ERRO: Sinal vazio para transmissão");
                        addLog("Erro: Sinal vazio para transmissão do dispositivo " + currentDeviceUUID, "error");
                    }
                }
                else
                {
                    Serial.printf("ERRO: Falha ao obter sinal para transmissão. %s\n", signal_fbdo.errorReason().c_str());
                    addLog("Erro: Falha ao obter sinal para transmissão do dispositivo " + currentDeviceUUID + ". Erro: " + signal_fbdo.errorReason(), "error");
                }
                displayDeviceMenu();
            }
            else if (input == "V")
            {
                Serial.println("Voltando para lista de dispositivos...");
                currentState = STATE_MAIN_MENU;
                displayMainMenu();
            }
            else
            {
                Serial.println("Opção inválida. Tente novamente.");
                displayDeviceMenu();
            }
            break;
        case STATE_RECEIVE_MODE:
            if (input == "P")
            {
                Serial.println("Modo de captura cancelado.");
                irrecv.disableIRIn();
                currentState = STATE_DEVICE_MENU;
                displayDeviceMenu();
            }
            break;
        default:
            break;
        }
    }
}

void requestTransmit(String uuid, String signal)
{
    if (!Firebase.ready())
    {
        Serial.println("ERRO: Firebase não está pronto para transmitir sinal");
        return;
    }

    Serial.printf("\n=== INICIANDO TRANSMISSÃO PARA %s ===\n", uuid.c_str());
    // Signal string parameter is no longer used for conversion
    
    // Use the global buffer populated by streamCallback
    uint16_t* rawData = transmitRawDataBuffer;
    int rawSize = transmitRawSize;

    if (rawSize > 0)
    {
        Serial.println("Enviando sinal IR...");
        irsend.sendRaw(rawData, rawSize, 36);
        delay(100);
        irsend.sendRaw(rawData, rawSize, 38);
        delay(100);
        irsend.sendRaw(rawData, rawSize, 40);
        delay(100);
        irsend.sendRaw(rawData, rawSize, 56);

        Serial.println("Sinal IR enviado.");
        addLog("Sinal IR transmitido com sucesso para dispositivo " + uuid, "info");
        // Set the transmit request back to false only AFTER successful transmission
        Firebase.setBool(fbdo, "/transmitRequests/" + uuid, false);

        // Clear the buffer after successful transmission
        transmitRawSize = 0;
    }
    else
    {
        Serial.println("ERRO: Sinal inválido ou buffer vazio para transmissão");
        addLog("Erro: Sinal inválido ou buffer vazio para transmissão do dispositivo " + uuid, "error");
        // Note: The transmit request should ideally already be set to false in streamCallback if conversion failed
    }
    Serial.println("=== FIM DA TRANSMISSÃO ===\n");
}

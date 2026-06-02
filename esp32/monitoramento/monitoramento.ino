#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// =====================================================
// ESP32-S3 - MONITORAMENTO
// Recebe dados via ESP-NOW e exibe na matriz MAX7219
// Envia dados para API Python via Wi-Fi
// =====================================================

#define ESPNOW_CHANNEL 1

// =====================
// WI-FI
// =====================
#define WIFI_SSID "CIMATEC-VISITANTE"
#define WIFI_PASSWORD ""
#define API_URL "http://172.28.34.71:5000/leitura"

// =====================
// LEDS DE STATUS
// =====================
#define LED_VERDE 26
#define LED_VERMELHO 25

// =====================
// MATRIZ MAX7219
// =====================
#define HARDWARE_TYPE MD_MAX72XX::GENERIC_HW
#define MAX_DEVICES 1

#define DATA_PIN 4   // DIN
#define CS_PIN   5   // CS
#define CLK_PIN  6   // CLK

MD_Parola matriz = MD_Parola(
  HARDWARE_TYPE,
  DATA_PIN,
  CLK_PIN,
  CS_PIN,
  MAX_DEVICES
);

// =====================
// ESTRUTURA DO PACOTE
// =====================
typedef struct struct_message {
  int pacote_id;
  int nivel_tinta;
  float temperatura;
  float umidade;
  int luminosidade;
  int presenca;
  unsigned long timestamp_ms;
} struct_message;

struct_message dadosRecebidos;

unsigned long ultimaRecepcao = 0;
unsigned long ultimoEnvioHttp = 0;

const unsigned long tempoTimeout = 5000;
const unsigned long intervaloHttp = 5000;

int telaAtual = 0;
bool temDados = false;
bool wifiConectado = false;

char textoMatriz[20];

// =====================
// CONECTAR WI-FI
// =====================
void conectarWifi() {
  Serial.println("====================================");
  Serial.println("CONECTANDO AO WI-FI");
  Serial.print("SSID: ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID);

  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiConectado = true;
    Serial.println();
    Serial.println("Wi-Fi conectado com sucesso");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    wifiConectado = false;
    Serial.println();
    Serial.println("AVISO: Wi-Fi nao conectado — continuando sem envio HTTP");
  }
}

// =====================
// ENVIAR DADOS HTTP
// =====================
void enviarHttp() {
  if (!wifiConectado || WiFi.status() != WL_CONNECTED) {
    Serial.println("[HTTP] Wi-Fi indisponivel — envio ignorado");
    return;
  }

  HTTPClient http;
  http.begin(API_URL);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<256> doc;
  doc["nivel_tinta"]  = dadosRecebidos.nivel_tinta;
  doc["temperatura"]  = dadosRecebidos.temperatura;
  doc["umidade"]      = dadosRecebidos.umidade;
  doc["luminosidade"] = dadosRecebidos.luminosidade;
  doc["presenca"]     = dadosRecebidos.presenca;

  String payload;
  serializeJson(doc, payload);

  int httpCode = http.POST(payload);

  if (httpCode == 200) {
    Serial.println("[HTTP] Dados enviados com sucesso");
  } else {
    Serial.print("[HTTP] Erro no envio — codigo: ");
    Serial.println(httpCode);
  }

  http.end();
}

// =====================
// CALLBACK DE RECEPCAO
// =====================
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  memcpy(&dadosRecebidos, incomingData, sizeof(dadosRecebidos));

  ultimaRecepcao = millis();
  temDados = true;

  digitalWrite(LED_VERDE, HIGH);
  digitalWrite(LED_VERMELHO, LOW);

  Serial.println("====================================");
  Serial.println("PACOTE RECEBIDO VIA ESP-NOW");

  Serial.print("Pacote ID: ");
  Serial.println(dadosRecebidos.pacote_id);

  Serial.print("Nivel da tinta: ");
  Serial.print(dadosRecebidos.nivel_tinta);
  Serial.println("%");

  Serial.print("Temperatura: ");
  Serial.print(dadosRecebidos.temperatura);
  Serial.println(" C");

  Serial.print("Umidade: ");
  Serial.print(dadosRecebidos.umidade);
  Serial.println(" %");

  Serial.print("Luminosidade: ");
  Serial.println(dadosRecebidos.luminosidade);

  Serial.print("Presenca: ");
  Serial.println(dadosRecebidos.presenca == 1 ? "Detectada" : "Sem presenca");

  Serial.print("Timestamp ms: ");
  Serial.println(dadosRecebidos.timestamp_ms);

  Serial.println("LED VERDE ON - dados recebidos");
}

// =====================
// ATUALIZAR TEXTO DA MATRIZ
// =====================
void atualizarTextoMatriz() {
  if (!temDados) {
    snprintf(textoMatriz, sizeof(textoMatriz), "SEM DADOS");
  } else {
    switch (telaAtual) {
      case 0:
        snprintf(textoMatriz, sizeof(textoMatriz), "NVL %d%%", dadosRecebidos.nivel_tinta);
        Serial.println("Tela -> NVL");
        break;

      case 1:
        snprintf(textoMatriz, sizeof(textoMatriz), "TMP %.0fC", dadosRecebidos.temperatura);
        Serial.println("Tela -> TMP");
        break;

      case 2:
        snprintf(textoMatriz, sizeof(textoMatriz), "UMD %.0f%%", dadosRecebidos.umidade);
        Serial.println("Tela -> UMD");
        break;

      case 3:
        snprintf(textoMatriz, sizeof(textoMatriz), "LUX %d", dadosRecebidos.luminosidade);
        Serial.println("Tela -> LUX");
        break;

      case 4:
        snprintf(textoMatriz, sizeof(textoMatriz), "PRS %s", dadosRecebidos.presenca == 1 ? "ON" : "OFF");
        Serial.println("Tela -> PRS");
        break;
    }
  }

  matriz.displayClear();
  matriz.displayText(
    textoMatriz,
    PA_CENTER,
    80,
    0,
    PA_SCROLL_LEFT,
    PA_SCROLL_LEFT
  );
}

// =====================
// CONFIGURAR ESP-NOW
// =====================
void configurarEspNow() {
  esp_wifi_set_ps(WIFI_PS_NONE);
  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);

  Serial.println("====================================");
  Serial.println("CONFIGURANDO ESP-NOW MONITORAMENTO");
  Serial.print("MAC deste ESP32-S3: ");
  Serial.println(WiFi.macAddress());
  Serial.print("Canal ESPNOW: ");
  Serial.println(ESPNOW_CHANNEL);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ERRO: falha ao iniciar ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("ESP-NOW pronto. Aguardando dados...");
}

// =====================
// CONFIGURAR MATRIZ
// =====================
void configurarMatriz() {
  matriz.begin();
  matriz.setIntensity(8);
  matriz.displayClear();

  Serial.println("Matriz MAX7219 inicializada");
}

// =====================
// SETUP
// =====================
void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);

  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_VERMELHO, HIGH);

  Serial.println("====================================");
  Serial.println("ESP32-S3 MONITORAMENTO INICIADO");
  Serial.println("Sistema IoT - Pintura de Blocos");
  Serial.println("====================================");

  configurarMatriz();

  // Exibe "WILD" 3 vezes ao iniciar
  for (int i = 0; i < 3; i++) {
    snprintf(textoMatriz, sizeof(textoMatriz), "WILD");
    matriz.displayClear();
    matriz.displayText(textoMatriz, PA_CENTER, 80, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
    while (!matriz.displayAnimate()) {}
  }

  conectarWifi();
  configurarEspNow();

  atualizarTextoMatriz();
}

// =====================
// LOOP
// =====================
void loop() {
  if (temDados && millis() - ultimaRecepcao > tempoTimeout) {
    temDados = false;

    digitalWrite(LED_VERDE, LOW);
    digitalWrite(LED_VERMELHO, HIGH);

    Serial.println("SEM DADOS - timeout de comunicacao");

    snprintf(textoMatriz, sizeof(textoMatriz), "SEM DADOS");
    matriz.displayClear();
    matriz.displayText(
      textoMatriz,
      PA_CENTER,
      80,
      0,
      PA_SCROLL_LEFT,
      PA_SCROLL_LEFT
    );
  }

  if (temDados && millis() - ultimoEnvioHttp >= intervaloHttp) {
    ultimoEnvioHttp = millis();
    enviarHttp();
  }

  if (matriz.displayAnimate()) {
    if (temDados) {
      telaAtual++;
      if (telaAtual > 4) telaAtual = 0;
      atualizarTextoMatriz();
    }
  }
}
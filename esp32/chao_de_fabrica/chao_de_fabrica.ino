#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <DHT.h>

// =====================================================
// ESP32 DEV MODULE - CHAO DE FABRICA
// Le sensores e envia dados via ESP-NOW para o ESP32-S3
// =====================================================

// MAC do ESP32-S3 de monitoramento
uint8_t macMonitoramento[] = {0x30, 0xED, 0xA0, 0xB7, 0x24, 0x08};

#define ESPNOW_CHANNEL 1

// =====================
// PINOS
// =====================
#define PIN_DHT 15
#define TIPO_DHT DHT22   // Se for DHT11, troque para DHT11

#define PIN_LDR 34

#define PIN_TRIG 5
#define PIN_ECHO 27

#define LED_VERDE 26
#define LED_VERMELHO 25

// =====================
// CONFIGURACAO DO TANQUE
// =====================
#define DISTANCIA_CHEIO 5
#define DISTANCIA_VAZIO 60

DHT dht(PIN_DHT, TIPO_DHT);

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

struct_message dadosEnviar;

int contadorPacote = 0;

volatile bool callbackRecebido = false;
volatile bool callbackSucesso = false;

// =====================
// CALLBACK DE ENVIO
// ESP32 Core 3.3.8
// =====================
void OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
  callbackSucesso = (status == ESP_NOW_SEND_SUCCESS);
  callbackRecebido = true;
}

// =====================
// MEDIR DISTANCIA
// =====================
float medirDistancia() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);

  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  long duracao = pulseIn(PIN_ECHO, HIGH, 30000);

  if (duracao == 0) {
    return -1;
  }

  return duracao * 0.034 / 2;
}

// =====================
// CALCULAR NIVEL DO TANQUE
// =====================
int calcularNivelTanque(float distancia) {
  if (distancia < 0) {
    return 0;
  }

  int nivel = map(distancia, DISTANCIA_VAZIO, DISTANCIA_CHEIO, 0, 100);

  if (nivel < 0) nivel = 0;
  if (nivel > 100) nivel = 100;

  return nivel;
}

// =====================
// CONFIGURAR ESP-NOW
// =====================
void configurarEspNow() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  esp_wifi_set_ps(WIFI_PS_NONE);
  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);

  Serial.println("====================================");
  Serial.println("CONFIGURANDO ESP-NOW");
  Serial.print("MAC deste ESP32: ");
  Serial.println(WiFi.macAddress());
  Serial.print("Canal ESPNOW: ");
  Serial.println(ESPNOW_CHANNEL);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ERRO: nao foi possivel iniciar ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, macMonitoramento, 6);
  peerInfo.channel = ESPNOW_CHANNEL;
  peerInfo.encrypt = false;
  peerInfo.ifidx = WIFI_IF_STA;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("ERRO: falha ao adicionar ESP32-S3 como peer");
    return;
  }

  Serial.println("ESP32-S3 adicionado como peer com sucesso");
}

// =====================
// SETUP
// =====================
void setup() {
  Serial.begin(115200);
  delay(1000);

  dht.begin();

  pinMode(PIN_LDR, INPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);

  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_VERMELHO, LOW);

  Serial.println("====================================");
  Serial.println("ESP32 CHAO DE FABRICA INICIADO");
  Serial.println("Sistema IoT - Pintura de Blocos");
  Serial.println("====================================");

  configurarEspNow();
}

// =====================
// LOOP PRINCIPAL
// =====================
void loop() {
  float temperatura = dht.readTemperature();
  float umidade = dht.readHumidity();
  int luminosidade = analogRead(PIN_LDR);
  float distancia = medirDistancia();
  int nivelTanque = calcularNivelTanque(distancia);

  int presenca = 0;

  if (distancia > 0 && distancia <= 50) {
    presenca = 1;
  }

  Serial.println();
  Serial.println("====================================");
  Serial.println("LEITURA DOS SENSORES");

  if (isnan(temperatura) || isnan(umidade)) {
    Serial.println("ERRO: falha ao ler DHT");
    delay(2000);
    return;
  }

  Serial.print("Temperatura: ");
  Serial.print(temperatura);
  Serial.println(" C");

  Serial.print("Umidade: ");
  Serial.print(umidade);
  Serial.println(" %");

  Serial.print("Luminosidade: ");
  Serial.println(luminosidade);

  Serial.print("Distancia ate a tinta: ");

  if (distancia == -1) {
    Serial.println("Sem leitura");
  } else {
    Serial.print(distancia);
    Serial.println(" cm");
  }

  Serial.print("Nivel do tanque: ");
  Serial.print(nivelTanque);
  Serial.println("%");

  Serial.print("Presenca: ");
  Serial.println(presenca == 1 ? "Detectada" : "Sem presenca");

  if (distancia == -1 || nivelTanque < 20) {
    digitalWrite(LED_VERDE, LOW);
    digitalWrite(LED_VERMELHO, HIGH);

    Serial.println("Estado do sistema: ALERTA");
    Serial.println("Motivo: nivel baixo ou falha na leitura do tanque");
  } else {
    digitalWrite(LED_VERDE, HIGH);
    digitalWrite(LED_VERMELHO, LOW);

    Serial.println("Estado do sistema: OPERACAO NORMAL");
  }

  contadorPacote++;

  dadosEnviar.pacote_id = contadorPacote;
  dadosEnviar.nivel_tinta = nivelTanque;
  dadosEnviar.temperatura = temperatura;
  dadosEnviar.umidade = umidade;
  dadosEnviar.luminosidade = luminosidade;
  dadosEnviar.presenca = presenca;
  dadosEnviar.timestamp_ms = millis();

  Serial.println("------------------------------------");
  Serial.println("PACOTE ENVIADO VIA ESP-NOW");

  Serial.print("Pacote ID: ");
  Serial.println(dadosEnviar.pacote_id);

  Serial.print("Nivel tinta: ");
  Serial.print(dadosEnviar.nivel_tinta);
  Serial.println("%");

  Serial.print("Temperatura enviada: ");
  Serial.print(dadosEnviar.temperatura);
  Serial.println(" C");

  Serial.print("Umidade enviada: ");
  Serial.print(dadosEnviar.umidade);
  Serial.println(" %");

  Serial.print("Luminosidade enviada: ");
  Serial.println(dadosEnviar.luminosidade);

  Serial.print("Presenca enviada: ");
  Serial.println(dadosEnviar.presenca);

  callbackRecebido = false;
  callbackSucesso = false;

  esp_err_t resultado = esp_now_send(
    macMonitoramento,
    (uint8_t *) &dadosEnviar,
    sizeof(dadosEnviar)
  );

  if (resultado == ESP_OK) {
    Serial.println("Comando esp_now_send executado");
  } else {
    Serial.println("ERRO: esp_now_send nao executou");
  }

  delay(300);

  if (callbackRecebido) {
    if (callbackSucesso) {
      Serial.println("Confirmacao ESP-NOW: sucesso");
    } else {
      Serial.println("Confirmacao ESP-NOW: nao confirmada");
      Serial.println("Obs: se o S3 recebeu o mesmo Pacote ID, a comunicacao esta funcionando.");
    }
  } else {
    Serial.println("Confirmacao ESP-NOW: sem retorno");
  }

  delay(1700);
}
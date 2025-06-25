#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <DHTesp.h>

// Wi-Fi
const char* ssid = "Kafei";
const char* password = "kau15092003@CASA621";

// IP fixo
IPAddress local_IP(192, 168, 7, 123);
IPAddress gateway(192, 168, 7, 1);
IPAddress subnet(255, 255, 255, 0);

// Sensores
#define DHT_PIN 4
#define SOIL_PIN 34

DHTesp dht;
WebServer server(80);

// Histórico
const int TAM_HISTORICO = 7;
String horarios[TAM_HISTORICO];
float historicoUmi[TAM_HISTORICO];
float historicoTemp[TAM_HISTORICO];
int indice = 0;

unsigned long ultimaLeitura = 0;
const unsigned long intervaloLeitura = 5000;

String getHoraAtual() {
  time_t now = time(nullptr);
  struct tm* t = localtime(&now);
  char buffer[6];
  strftime(buffer, sizeof(buffer), "%H:%M", t);
  return String(buffer);
}

void lerSensores() {
  TempAndHumidity th = dht.getTempAndHumidity();

  if (!isnan(th.temperature) && !isnan(th.humidity)) {
    int leituraBruta = analogRead(SOIL_PIN);
    float umidadeSolo = map(leituraBruta, 4095, 0, 0, 100);

    if (umidadeSolo >= 0 && umidadeSolo <= 100) {
      if (indice >= TAM_HISTORICO) indice = 0;

      historicoTemp[indice] = th.temperature;
      historicoUmi[indice] = umidadeSolo;
      horarios[indice] = getHoraAtual();
      indice++;
    }
  }
}

void handleUmidade() {
  DynamicJsonDocument doc(1024);
  JsonArray root = doc.to<JsonArray>();
  JsonArray header = root.createNestedArray();
  header.add("Horário");
  header.add("Umidade");

  for (int i = 0; i < TAM_HISTORICO; i++) {
    if (horarios[i] == "") continue;
    JsonArray linha = root.createNestedArray();
    linha.add(horarios[i]);
    linha.add(historicoUmi[i]);
  }

  String json;
  serializeJson(doc, json);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

void handleTemperatura() {
  DynamicJsonDocument doc(1024);
  JsonArray root = doc.to<JsonArray>();
  JsonArray header = root.createNestedArray();
  header.add("Horário");
  header.add("Temperatura");

  for (int i = 0; i < TAM_HISTORICO; i++) {
    if (horarios[i] == "") continue;
    JsonArray linha = root.createNestedArray();
    linha.add(horarios[i]);
    linha.add(historicoTemp[i]);
  }

  String json;
  serializeJson(doc, json);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(9600);
  dht.setup(DHT_PIN, DHTesp::DHT22);
  analogSetPinAttenuation(SOIL_PIN, ADC_11db);

  configTime(-3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("Falha IP fixo");
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  server.on("/umidade", handleUmidade);
  server.on("/temperatura", handleTemperatura);
  server.begin();

  Serial.println("Servidor HTTP iniciado");
}

void loop() {
  server.handleClient();

  if (millis() - ultimaLeitura > intervaloLeitura) {
    ultimaLeitura = millis();
    lerSensores();
  }
}

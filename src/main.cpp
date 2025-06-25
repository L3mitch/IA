#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

// Credenciais do Wi-Fi
const char* ssid = "Kafei";
const char* password = "kau15092003@CASA621";

// Configuração de IP fixo
IPAddress local_IP(192, 168, 7, 123);
IPAddress gateway(192, 168, 7, 1);
IPAddress subnet(255, 255, 255, 0);

// Servidor na porta 80
WebServer server(80);

// Dados simulados
const char* horarios[] = {"12:00", "13:00", "14:00", "15:00", "16:00", "17:00", "18:00"};
int umidades[] =    {80, 60, 32, 10, 100, 90, 70};
float temperaturas[] = {25.0, 26.1, 27.0, 27.0, 26.2, 24.7, 22.3};

// Rota de umidade
void handleUmidade() {
  DynamicJsonDocument doc(1024);
  JsonArray root = doc.to<JsonArray>();

  JsonArray header = root.createNestedArray();
  header.add("Horário");
  header.add("Umidade");

  for (int i = 0; i < 7; i++) {
    JsonArray linha = root.createNestedArray();
    linha.add(horarios[i]);
    linha.add(umidades[i]);
  }

  String json;
  serializeJson(doc, json);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

// Rota de temperatura
void handleTemperatura() {
  DynamicJsonDocument doc(1024);
  JsonArray root = doc.to<JsonArray>();

  JsonArray header = root.createNestedArray();
  header.add("Horário");
  header.add("Temperatura");

  for (int i = 0; i < 7; i++) {
    JsonArray linha = root.createNestedArray();
    linha.add(horarios[i]);
    linha.add(temperaturas[i]);
  }

  String json;
  serializeJson(doc, json);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(9600);

  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("Falha ao configurar IP fixo!");
  }

  WiFi.begin(ssid, password);
  Serial.print("Conectando ao Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // Inicia rotas
  server.on("/umidade", handleUmidade);
  server.on("/temperatura", handleTemperatura);
  server.begin();
  Serial.println("Servidor HTTP iniciado");
}

void loop() {
  server.handleClient();
}

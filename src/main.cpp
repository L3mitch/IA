#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <DHTesp.h>

// Wi-Fi
const char *ssid = "Teste HTTP";
const char *password = "123teste";

extern float computeIrrigationTime(float temp, float humAir, float humSoil, float hours);

// IP fixo
IPAddress local_IP(192, 168, 79, 123);
IPAddress gateway(192, 168, 79, 1);
IPAddress subnet(255, 255, 255, 0);

// Sensores
#define DHT_PIN 21
#define SOIL_PIN 34
// #define SOIL_PIN 34

DHTesp dht;
WebServer server(80);

// Histórico
const int TAM_HISTORICO = 120;
String horarios[TAM_HISTORICO];
float historicoUmi[TAM_HISTORICO];
float historicoUmiAr[TAM_HISTORICO];
float historicoTemp[TAM_HISTORICO];

float umidadeSolo = 0.0;
float temperatura = 0.0;
float umidadeAr = 0.0;
int indice = 0;

bool irrigando = false;
unsigned long inicioIrrigacao = 0;
float tempoIrrigacao = 0.0;

unsigned long ultimaLeitura = 0;
unsigned long ultimaIrrigacao = 0;
const unsigned long intervaloLeitura = 5000;

String getHoraAtual()
{
  time_t now = time(nullptr);
  struct tm *t = localtime(&now);
  char buffer[6];
  strftime(buffer, sizeof(buffer), "%H:%M", t);
  return String(buffer);
}

void lerSensores()
{
  TempAndHumidity th = dht.getTempAndHumidity();

  if (!isnan(th.temperature) && !isnan(th.humidity))
  {
    int leituraBruta = analogRead(SOIL_PIN);
    umidadeSolo = map(leituraBruta, 4095, 0, 0, 100);
    umidadeAr = th.humidity;
    temperatura = th.temperature;

    if (umidadeSolo >= 0 && umidadeSolo <= 100)
    {
      if (indice >= TAM_HISTORICO)
        indice = 0;

      historicoTemp[indice] = temperatura;
      historicoUmiAr[indice] = umidadeAr;
      historicoUmi[indice] = umidadeSolo;
      horarios[indice] = getHoraAtual();
      indice++;
    }
  }
}

void handleUmidade()
{
  DynamicJsonDocument doc(1024);
  JsonArray root = doc.to<JsonArray>();
  JsonArray header = root.createNestedArray();
  header.add("Horário");
  header.add("Umidade");

  for (int i = 0; i < TAM_HISTORICO; i++)
  {
    if (horarios[i] == "")
      continue;
    JsonArray linha = root.createNestedArray();
    linha.add(horarios[i]);
    linha.add(historicoUmi[i]);
  }

  String json;
  serializeJson(doc, json);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

void handleTemperatura()
{
  DynamicJsonDocument doc(1024);
  JsonArray root = doc.to<JsonArray>();
  JsonArray header = root.createNestedArray();
  header.add("Horário");
  header.add("Temperatura");

  for (int i = 0; i < TAM_HISTORICO; i++)
  {
    if (horarios[i] == "")
      continue;
    JsonArray linha = root.createNestedArray();
    linha.add(horarios[i]);
    linha.add(historicoTemp[i]);
  }

  String json;
  serializeJson(doc, json);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

void setup()
{
  Serial.begin(9600);
  dht.setup(DHT_PIN, DHTesp::DHT22);
  analogSetPinAttenuation(SOIL_PIN, ADC_11db);

  pinMode(22, OUTPUT); // Pino do relé para irrigação
  digitalWrite(22, LOW); // Desliga a irrigação

  configTime(-3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  if (!WiFi.config(local_IP, gateway, subnet))
  {
    Serial.println("Falha IP fixo");
  }

  WiFi.begin(ssid, password);
  //while (WiFi.status() != WL_CONNECTED)
  while(0)
  {
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

void loop()
{
  //server.handleClient();

  if (millis() - ultimaLeitura > intervaloLeitura)
  {
    ultimaLeitura = millis();
    lerSensores();

    if (!irrigando)
    {                                        // Liga a irrigação
      float delta = (float)(millis() - ultimaIrrigacao) / 3600000.0; // horas desde a última irrigação
      Serial.printf("Última irrigação: %.8f horas atrás\n\r", delta);

      delta = 24;

      float tempoIrr = computeIrrigationTime(temperatura, umidadeSolo, umidadeAr, delta);
      if (tempoIrr > 2)
      {
        irrigando = true;
        inicioIrrigacao = millis();
        tempoIrrigacao = tempoIrr;
        digitalWrite(22, HIGH); // Liga a irrigação
        Serial.printf("Irrigação necessária: %.2f segundos\n\r", tempoIrr);
      }
      else
      {
        Serial.printf("Irrigação não necessária: %.2f segundos\n\r", tempoIrr);
      }
    }
  }

  if (irrigando)
  {
    if (millis() - inicioIrrigacao >= tempoIrrigacao * 1000)
    {
      irrigando = false;
      digitalWrite(22, LOW); // Liga a irrigação
      ultimaIrrigacao = millis();
      Serial.println("Irrigação concluída.");
    }
    else
    {
      float tempoRestante = inicioIrrigacao + tempoIrrigacao * 1000 - millis();
      Serial.printf("Irrigando... Tempo restante: %0.3f segundos\n\r", tempoRestante / 1000);
    }
  }
}

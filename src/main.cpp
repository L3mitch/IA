#include <Arduino.h>
#include "DHTesp.h"
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

#define MOTOR1_H 4
#define MOTOR1_AH 2
#define MOTOR1_PWM 15
#define MOTOR2_H 5
#define MOTOR2_AH 18
#define MOTOR2_PWM 19

// #define RGB_R 21
// #define RGB_G 19
// #define RGB_B 18

#define DHT22_PIN 21
// #define BUZZER 22
#define SENSOR_CORRENTE 13
// #define LDR 33
#define IR_RECEIVER 12

IRrecv irrecv(IR_RECEIVER);
decode_results results;

DHTesp dhtSensor;

QueueHandle_t filaSensorCorrente = xQueueCreate(5, sizeof(float));

TaskHandle_t verTempHandle = NULL;
TaskHandle_t lerCorrHandle = NULL;
TaskHandle_t abreCortHandle = NULL;
TaskHandle_t fechaCortHandle = NULL;
TaskHandle_t sensorIRhandle = NULL;

enum CortnStates
{
  FECHADA,
  ABERTA,
};

enum IRButtonStates
{
  POWER = 162,
  MENU = 226,
  PLAY = 168,
  CIMA = 2,
  BAIXO = 152,
  ESQUERDA = 224,
  DIREITA = 144,
};

CortnStates estadoCortina = FECHADA;

void verTempUmid(void *pvParameters);
void lerCorrente(void *pvParameters);
void abreCortina(void *pvParameters);
void fechaCortina(void *pvParameters);
void sensorIR(void *pvParameters);

void setup()
{
  delay(2000);
  Serial.begin(9600);
  dhtSensor.setup(DHT22_PIN, DHTesp::DHT22);

  pinMode(MOTOR1_H, OUTPUT);
  pinMode(MOTOR1_AH, OUTPUT);
  pinMode(MOTOR2_H, OUTPUT);
  pinMode(MOTOR2_AH, OUTPUT);
  pinMode(IR_RECEIVER, INPUT_PULLDOWN);
  irrecv.enableIRIn(); // Inicia o receptor IR

  xTaskCreate(
      verTempUmid,
      "verTemp&Umid",
      2048,
      NULL,
      1,
      &verTempHandle);

  xTaskCreate(
      lerCorrente,
      "lerCorrente",
      2048,
      NULL,
      1,
      &lerCorrHandle);
  vTaskSuspend(lerCorrHandle);

  xTaskCreate(
      abreCortina,
      "AbreCortina",
      2048,
      NULL,
      1,
      &abreCortHandle);
  vTaskSuspend(abreCortHandle);

  xTaskCreate(
      fechaCortina,
      "FechaCortina",
      2048,
      NULL,
      1,
      &fechaCortHandle);
  vTaskSuspend(fechaCortHandle);

  xTaskCreate(
      sensorIR,
      "SensorIR",
      2048,
      NULL,
      1,
      &sensorIRhandle);
}

void loop()
{
}

void verTempUmid(void *pvParameters)
{
  while (true)
  {
    Serial.println("Tarefa 1 em execução!");

    TempAndHumidity data = dhtSensor.getTempAndHumidity();
    Serial.println("Temp: " + String(data.temperature, 2) + "°C");
    Serial.println("Humidity: " + String(data.humidity, 1) + "%");

    Serial.println("------------------------------------------------");

    if ((estadoCortina == FECHADA) && (data.temperature >= 25))
    {
      vTaskResume(abreCortHandle);
    }
    else if ((estadoCortina == ABERTA) && (data.temperature <= 21))
    {
      vTaskResume(fechaCortHandle);
    }

    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

void lerCorrente(void *pvParameters)
{
  while (true)
  {
    Serial.println("Tarefa 2 em execução!");
    int leituraPura = analogRead(SENSOR_CORRENTE);
    float tensaoPura = (float)leituraPura * (3.3 / 4096.0);
    float corrente = (tensaoPura - 1.65) / 0.1181; // Conversão da tensão para corrente (em A)
    Serial.printf("leituraPura: %d \r\n", leituraPura);
    Serial.printf("tensaoPura: %f \r\n", tensaoPura);
    Serial.printf("Corrente: %.2f A\r\n", corrente);
    xQueueSend(filaSensorCorrente, &corrente, pdMS_TO_TICKS(250));
    Serial.println("------------------------------------------------");

    vTaskDelay(pdMS_TO_TICKS(250));
  }
}

void abreCortina(void *pvParameters)
{
  float valorCorrente;
  while (true)
  {
    Serial.println("Tarefa 3 em execução!");
    Serial.println("Abrindo cortina...");
    digitalWrite(MOTOR2_AH, HIGH);
    digitalWrite(MOTOR1_H, HIGH);
    analogWrite(MOTOR1_PWM, 100);
    analogWrite(MOTOR2_PWM, 100);
    vTaskSuspend(verTempHandle);
    vTaskResume(lerCorrHandle);
    Serial.println("------------------------------------------------");
    do
    {
      xQueueReceive(filaSensorCorrente, &valorCorrente, portMAX_DELAY);
    } while (valorCorrente < 0.8);
    estadoCortina = ABERTA;
    Serial.println("Cortina aberta!");
    digitalWrite(MOTOR2_AH, LOW);
    digitalWrite(MOTOR1_H, LOW);
    analogWrite(MOTOR1_PWM, 0);
    analogWrite(MOTOR2_PWM, 0);

    vTaskSuspend(lerCorrHandle);
    vTaskResume(verTempHandle);
    vTaskSuspend(abreCortHandle);
  }
}

void fechaCortina(void *pvParameters)
{
  float valorCorrente;
  while (true)
  {
    Serial.println("Tarefa 4 em execução!");
    Serial.println("Fechando cortina...");
    digitalWrite(MOTOR2_H, HIGH);
    digitalWrite(MOTOR1_AH, HIGH);
    analogWrite(MOTOR1_PWM, 100);
    analogWrite(MOTOR2_PWM, 100);
    vTaskSuspend(verTempHandle);
    vTaskResume(lerCorrHandle);
    Serial.println("------------------------------------------------");
    do
    {
      xQueueReceive(filaSensorCorrente, &valorCorrente, portMAX_DELAY);
    } while (valorCorrente < 0.6);
    estadoCortina = FECHADA;
    Serial.println("Cortina fechada!");
    digitalWrite(MOTOR2_H, LOW);
    digitalWrite(MOTOR1_AH, LOW);
    analogWrite(MOTOR1_PWM, 0);
    analogWrite(MOTOR2_PWM, 0);

    vTaskSuspend(lerCorrHandle);
    vTaskResume(verTempHandle);
    vTaskSuspend(fechaCortHandle);
  }
}

void sensorIR(void *pvParameters)
{
  while (true)
  {
    if (irrecv.decode(&results))
    {
      IRButtonStates comando = static_cast<IRButtonStates>(results.command);
      Serial.print("Comando (8 bits): ");
      Serial.println(comando); // Mostra como número decimal
      // results.decode_type = NEC;
      // Serial.print("Código IR recebido: ");
      // Serial.println(resultToHexidecimal(&results));
      // Serial.println(resultToHumanReadableBasic(&results));
      irrecv.resume(); // Pronto para o próximo
    }

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

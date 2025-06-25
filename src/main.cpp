#include <Arduino.h>
#include "DHTesp.h"
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

#define LED_INDICADOR 22
#define DHT22_PIN 21
#define BUZZER 23
#define UMID_SOLO 4
#define IR_RECEIVER 0

IRrecv irrecv(IR_RECEIVER);
decode_results results;

DHTesp dhtSensor;

enum IRButtonStates
{
  POWER = 162,
  MODO = 226,
  PLAY = 168,
  CIMA = 2,
  BAIXO = 152,
  ESQUERDA = 224,
  DIREITA = 144,
};


void setup()
{
  Serial.begin(9600);
  pinMode(BUZZER, OUTPUT);
  pinMode(LED_INDICADOR, OUTPUT);
  pinMode(IR_RECEIVER, INPUT_PULLDOWN);
  dhtSensor.setup(DHT22_PIN, DHTesp::DHT22);
  irrecv.enableIRIn(); // Inicia o receptor IR
  delay(250);
  Serial.println("------------------------------------------------");
  Serial.println("Iniciando o sistema...");
  Serial.println("------------------------------------------------");

  delay(250);
}

void loop()
{
}

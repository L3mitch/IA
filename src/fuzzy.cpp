#include <cmath>
#include <algorithm>
#include <array>

//---- parâmetros dos universos ----
// Ajuste TAM_UNIV_X conforme necessidade de discretização
constexpr int TAM_UNIV_TIME = 101; // 0…100

// Centroides aproximados dos conjuntos de saída
constexpr float CENTROID_FECHADO = 0.0f;
constexpr float CENTROID_POUCOS = (0 + 3 + 5) / 3.0f;  // ≃ 2.67
constexpr float CENTROID_MEDIO = (4 + 6 + 8) / 3.0f;   // ≃ 6.0
constexpr float CENTROID_MUITOS = (7 + 9 + 10) / 3.0f; // ≃ 8.67

//---- funções de pertinência ----
float trapmf(float x, float a, float b, float c, float d)
{
   if (x <= a || x >= d)
      return 0.0f;
   if (x <= b)
      return (x - a) / (b - a);
   if (x >= c)
      return (d - x) / (d - c);
   return 1.0f;
}

float trimf(float x, float a, float b, float c)
{
   if (x <= a || x >= c)
      return 0.0f;
   if (x == b)
      return 1.0f;
   if (x < b)
      return (x - a) / (b - a);
   // x > b
   return (c - x) / (c - b);
}

float gaussmf(float x, float mean, float sigma)
{
   float u = (x - mean) / sigma;
   return std::exp(-0.5f * u * u);
}

//---- computa o tempo de irrigação fuzzy ----
float computeIrrigationTime(float temp, float humAir, float humSoil, float hours)
{
   // 1) graus de pertinência das entradas
   float T_low = trapmf(temp, 0, 0, 12, 18);
   float T_opt = trapmf(temp, 15, 20, 30, 35);
   float T_high = trapmf(temp, 30, 40, 50, 50);

   float UA_low = trapmf(humAir, 0, 0, 20, 35);
   float UA_opt = trapmf(humAir, 30, 40, 60, 70);
   float UA_high = trapmf(humAir, 65, 80, 100, 100);

   float US_dry = trapmf(humSoil, 0, 0, 30, 40);
   float US_opt = trapmf(humSoil, 30, 40, 60, 70);
   float US_wet = trapmf(humSoil, 70, 80, 100, 100);

   float HR_recent = trapmf(hours, 0, 0, 12, 22);
   float HR_optimal = gaussmf(hours, 20, 5);
   float HR_prolonged = trapmf(hours, 26, 40, 50, 50);

   // 2) avaliação das regras (min = AND; max = OR)
   // r1: if recent → fechado
   float r1 = HR_recent;

   // r2: if optimal & dry → poucos
   float r2 = std::min(HR_optimal, US_dry);

   // r3: if prolonged & dry → medio
   float r3 = std::min(HR_prolonged, US_dry);

   // r4a: prolonged & optimal-soil & high temp & low air → poucos
   float r4a = std::min({HR_prolonged, US_opt, T_high, UA_low});

   // r4b: optimal & dry & high temp & low air → medio
   float r4b = std::min({HR_optimal, US_dry, T_high, UA_low});

   // r4c: optimal & dry & optimal temp & optimal air → poucos
   float r4c = std::min({HR_optimal, US_dry, T_opt, UA_opt});

   // r5: if soil optimal or wet → fechado
   float r5 = std::max(US_opt, US_wet);

   // 3) agregação das saídas: pega o máximo grau para cada termo
   float grau_fechado = std::max(r1, r5);
   float grau_poucos = std::max(r2, std::max(r4a, r4c));
   float grau_medio = std::max(r3, r4b);
   float grau_muitos = 0.0f;

   // 4) defuzzificação pelo centróide (método peso médio)
   float somaPesos = grau_fechado * CENTROID_FECHADO + grau_poucos * CENTROID_POUCOS + grau_medio * CENTROID_MEDIO + grau_muitos * CENTROID_MUITOS;

   float somaGrau = grau_fechado + grau_poucos + grau_medio + grau_muitos;

   if (somaGrau <= 0.0f)
      return 0.0f;
   return somaPesos / somaGrau;
}
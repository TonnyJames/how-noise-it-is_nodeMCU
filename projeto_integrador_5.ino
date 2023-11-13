#define BLYNK_TEMPLATE_ID "indentificação do template blynk"
#define BLYNK_TEMPLATE_NAME "Decibelimetro"
#define BLYNK_AUTH_TOKEN "codigo de autorização do blynk"
#define BLYNK_PRINT Serial


#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <BlynkSimpleEsp8266.h>
#include <LiquidCrystal_I2C.h>
#include <Arduino_JSON.h>

#define SENSOR_PIN A0
// LiquidCrystal_I2C lcd(0x3F, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

HTTPClient http;
WiFiClient client;

BlynkTimer blynkTimer;
const int sampleWindow = 1000;
unsigned int sample;
int db;

char auth[] = BLYNK_AUTH_TOKEN;  // <<<< Token
char ssid[] = "ssid da sua rede";
char pass[] = "senha do wifi";



void enviarLeituraBlynk() {
  Blynk.virtualWrite(V0, db);
}

void setup() {

  Serial.begin(9600);
  
  conectarWifi(ssid, pass);
  pinMode(SENSOR_PIN, INPUT);
  lcd.begin(16, 2);
  lcd.backlight();
  lcd.clear();
  Blynk.begin(auth, ssid, pass);
  blynkTimer.setInterval(1000L, enviarLeituraBlynk);
}

void loop() {
  Blynk.run();
  blynkTimer.run();

  unsigned long startMillis = millis();  // Início da janela de amostra
  float peakToPeak = 0;                  //nível pico a pico
  unsigned int signalMax = 0;            //valor minimo
  unsigned int signalMin = 1023;         //valor máximo
  // coletar dados para 50 ms
  while (millis() - startMillis < sampleWindow) {
    sample = analogRead(SENSOR_PIN);  //obter leitura do microfone
    if (sample < 1024) {              // jogue fora leituras espúrias
      if (sample > signalMax) {
        signalMax = sample;  // salve apenas os níveis máximos
      } else if (sample < signalMin) {
        signalMin = sample;  // salve apenas os níveis mínimos
      }
    }
  }


  peakToPeak = signalMax - signalMin;       //max - min = amplitude pico-pico
  db = map(peakToPeak, 20, 900, 49.5, 90);  //calibrar para decibéis

  lcd.print(db);
  lcd.print("dB");

  if (db <= 50) {
    lcd.setCursor(0, 1);
    lcd.print("Nível: Baixo");
  } else if (db > 50 && db < 75) {
    lcd.setCursor(0, 1);
    lcd.print("Nível: Moderado");
  } else if (db >= 75) {
    lcd.setCursor(0, 1);
    lcd.print("Nível: Alto");
  }
  delay(5000);
  lcd.clear();

  //comunicação com api rest
  Serial.println("Decibéis medidos : " + String(db));
  http.begin(client,"end point da aplicação backend que vai receber esses dados");
  http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.POST("{\"id\": null,\"nomeDispositivo\":\"NodeMCU\",\"valor\":" +String(db)+"}");

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String payload = http.getString();
    Serial.println(payload);
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
}

void conectarWifi(char SSID[], char PASS[]) {

  Serial.print("Conectando a rede ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi Conectado");
  Serial.println(WiFi.localIP());
}

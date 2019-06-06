#include <App.hpp>
#include <Arduino.h>
#include <DHT.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <Ticker.h>

volatile float h;
volatile float t;

ADC_MODE(ADC_VCC);

static double get_vbattery() { return ((double)ESP.getVcc()) / 930.0; }

// a static ip is faster to handle with.
// every second is counting for battery lifetime
// IPAddress ip(192, 168, x, x);
// IPAddress gateway(192, 168, x, x);
// IPAddress subnet(255, 255, 255, 0);

// DHT22
#define DHTPIN D4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

volatile int watchdogCounter;
static Ticker watchdogTicker;

void ISRwatchdog()
{
  if (watchdogCounter++ == 45)
  {
    Serial.println();
    Serial.println("*** RESET by watchdog ***");
    ESP.reset();
  }
}

void setup()
{
  Serial.begin(74880);
  pinMode(D0, WAKEUP_PULLUP);
  dht.begin();
  Serial.println();
  Serial.println();
  Serial.println(APP_NAME " - " APP_VERSION);
  Serial.println("BUILD: " __DATE__ " " __TIME__);

  watchdogCounter = 0;
  watchdogTicker.attach(1, ISRwatchdog);

  h = dht.readHumidity();
  t = dht.readTemperature();

  // WiFi.config(ip, gateway, subnet);
  WiFi.begin();
  WiFi.hostname(HOSTNAME);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print(".");
  }

  Serial.println(F("\nWiFi connected"));

  do
  {
    delay(250);
    h = dht.readHumidity();
    t = dht.readTemperature();
  } while (isnan(h) || isnan(t));

  Serial.printf(
      "Humidity: %0.01f%% -- Temperature: %0.01f*C -- Battery: %0.02f\n", h, t,
      get_vbattery());

  char buffer[256];

  sprintf(buffer,
          "temperature,host=" HOSTNAME " value=%f\n"
          "humidity,host=" HOSTNAME " value=%f\n"
          "battery_v,host=" HOSTNAME " value=%f\n",
          t, h, get_vbattery());

  WiFiClient wifiClient;
  HTTPClient httpClient;
  httpClient.begin(wifiClient, INFLUX_DB_URL);
  int httpCode = httpClient.POST(buffer);
  if (httpCode != 204)
  {
    Serial.printf("http return=%d : ", httpCode);
  }
  httpClient.end();

  Serial.println("\ndeep sleep ... ");
  //  ESP.deepSleep( 120l * 1000000l);
  ESP.deepSleep(10l * 1000000l);
  delay(500);
}

void loop()
{
  delay(10000);
}

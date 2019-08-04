// Required libraries (sketch -> include library -> manage libraries)
// - PubSubClient by Nick ‘O Leary
// - DHT sensor library by Adafruit
// - SoftwareSerial by Peter Lerup
// - Grove-Multichannel gas sensor by Seed Studio
// - Nova Fitness SDS dust sensors arduino library by lewapek

//Libraries
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Wire.h>
#include "MutichannelGasSensor.h"
#include "SdsDustSensor.h"

//Data of your WIFI network
#define wifi_ssid "SSID of your WIFI"
#define wifi_password "WIFI password"

//MQTT Data of your server
#define mqtt_server "IP of your MQTT Broker"
#define mqtt_user "MQTT username"
#define mqtt_password "MQTT password"
#define client_no "Client ID"

//Definition of MQTT topics
#define humidity_topic "humidity"
#define temperature_topic "temperature"
#define CO2_topic "co2"
#define CO_topic "co"
#define NH3_topic "nh3"
#define NO2_topic "no2"
#define pm25_topic "pm25_value"
#define pm10_topic "pm10_value"

//Pins for sensor communications
#define DHTTYPE DHT22
#define DHTPIN  D2

#define MH_Z19_RX D3
#define MH_Z19_TX D8

#define PM_sensor_RX D5
#define PM_sensor_TX D6

//Variables for different sensor values
float temp = 0.0;
float hum = 0.0;
int CO2 = 0;
float CO_data = 0;
float NH3_data = 0;
float NO2_data = 0;

// Interval of sensor reading
long INTERVAL = 60; //INTERVAL [60s]
long INTERVAL_real = 0;

WiFiClient espClient;
PubSubClient client(espClient);

DHT dht(DHTPIN, DHTTYPE, 11); //define temperature and humidity sensor

SoftwareSerial co2Serial(MH_Z19_RX, MH_Z19_TX); // define MH-Z19
SoftwareSerial dustSerial(PM_sensor_RX, PM_sensor_TX); // define nova PM sensor
SdsDustSensor sds(dustSerial);

//Configuration of Wifi
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  WiFi.begin(wifi_ssid, wifi_password);
  //Loop until WiFi connected
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

//Reading of CO2 sensor
int readCO2()
{

  byte cmd[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
  // command to ask for data
  char response[9]; // for answer

  co2Serial.write(cmd, 9); //request PPM CO2
  co2Serial.readBytes(response, 9);
  if (response[0] != 0xFF)
  {
    return -1;
  }

  if (response[1] != 0x86)
  {
    return -1;
  }

  int responseHigh = (int) response[2];
  int responseLow = (int) response[3];
  int newCO2 = (256 * responseHigh) + responseLow;
  return newCO2;
}

void setup() {
  Serial.begin(9600); //Init serial
  dht.begin(); //Init of communication with DHT22
  delay(100);
  Wire.begin(D7, D1); //Init one wire bus for communication with multichannel gas sensor
  gas.begin(0x04);//the default I2C address of the slave is 0x04
  gas.powerOn();
  setup_wifi(); //WiFi configuration
  client.setServer(mqtt_server, 1883); //Start communication with MQTT broker
  ESP.wdtDisable(); //Stop ESP software-watchdog
  ESP.wdtEnable(WDTO_8S); //Start ESP software-watchdog with time of 8 seconds
  delay(1000);
}

void loop() {
  INTERVAL_real = (INTERVAL * 1000000)-9000000; //Calculated time for depp sleep
  ESP.wdtFeed(); //Feed software-watchdog
  if (!client.connected()) //Analyze connection to MQTT broker. In case of no connection, try to reconnect.
  {
    while (!client.connected()) {
      if (client.connect(client_no, mqtt_user, mqtt_password)){
      }
      delay(500);
    }
  }
  client.loop(); //Prepare massaging with MQTT broker

  client.publish(client_no, "connected", true); //Client publish "connected"

  temp = dht.readTemperature(); //Measurement of temperaure
  if (isnan(temp)){
    while (isnan(temp)) {
      temp = dht.readTemperature();
      delay(500);
    }
  }
  ESP.wdtFeed(); //Feed software-watchdog
  hum = dht.readHumidity(); //Measurement of humidity
  if (isnan(hum)){
    while (isnan(hum)) {
      hum = dht.readHumidity();
      delay(500);
    }
  }
  ESP.wdtFeed(); //Feed software-watchdog
  delay(1000); //delay of 1s
  co2Serial.begin(9600); //Init sensor MH-Z19
  delay(1000); //delay of 1s
  CO2 = readCO2(); //Measurement of CO2
  delay(1000); //delay of 1s
  CO_data = gas.measure_CO(); //Measurement of CO
  co2Serial.end(); //End of serial connection to sensor MH-Z19
  ESP.wdtFeed(); //Feed software-watchdog
  delay(1000); //delay of 1s
  NH3_data = gas.measure_NH3(); //Measurement of NH3
  delay(1000); //delay of 1s
  NO2_data = gas.measure_NO2(); //Measurement of NO2
  sds.begin(); //Init Nova Fitness SDS dust sensors
  ESP.wdtFeed(); //Feed software-watchdog
  delay(1000); //delay of 1s
  PmResult pm = sds.readPm(); //Measurement of PM2.5 & PM10 and sending to MQTT broker
  if (pm.isOk()) {
    client.publish(pm25_topic, String(pm.pm25).c_str(), true);
    Serial.println("pm25 sent");
    delay(1000);
    client.publish(pm10_topic, String(pm.pm10).c_str(), true);
    Serial.println("pm10 sent");
    delay(1000);
  }
  if (temp > 0 && temp < 50) { //If temperature is between 0°C and 50°C, send the value to MQTT broker
    client.publish(temperature_topic, String(temp).c_str(), true);
    Serial.println("temp sent");
    ESP.wdtFeed();
    delay(1000);
  }
  if (hum > 20 && hum < 80) { //If humidity is between 20% and 80%, send the value to MQTT broker
    client.publish(humidity_topic, String(hum).c_str(), true);
    Serial.println("humid sent");
    ESP.wdtFeed();
    delay(1000);
  }
  if (CO2 > 200 && CO2 < 5000) { //If CO2 is between 200 and 5000, send the value to MQTT broker
    client.publish(CO2_topic, String(CO2).c_str(), true);
    Serial.println("co2 sent");
    ESP.wdtFeed();
    delay(1000);
  }
  if (CO_data > 0.01 && CO_data < 1300) {//If CO is between 0.01 and 1300, send the value to MQTT broker
    client.publish(CO_topic, String(CO_data).c_str(), true);
    Serial.println("CO sent");
    ESP.wdtFeed();
    delay(500);
  }
  if (NH3_data > 0.01 && NH3_data < 2000) {//If NH3 is between 0.01 and 2000, send the value to MQTT broker
    client.publish(NH3_topic, String(NH3_data).c_str(), true);
    Serial.println("NH3 sent");
    ESP.wdtFeed();
    delay(500);
  }
  if (NO2_data > 0.01 && NO2_data < 2000) {//If NO2 is between 0.01 and 2000, send the value to MQTT broker
    client.publish(NO2_topic, String(NO2_data).c_str(), true);
    Serial.println("NO2 sent");
    ESP.wdtFeed();
    delay(500);
  }
  client.disconnect(); //Disconnect from MQTT broker
  WiFi.disconnect(); //Disconnect from WiFi
  ESP.wdtFeed();
  ESP.deepSleep(INTERVAL_real); //Start of deep sleep
  delay(500);
}

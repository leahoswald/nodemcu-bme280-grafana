#include <Adafruit_BME280.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Wire.h>

// define timezone offset in seconds
#define UTC 0

// function declarations
char* getCharFromString(String);
void writeTCP(WiFiClient, String, float);

// create objects
ESP8266WiFiMulti WiFiMulti;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ptbtime1.ptb.de", UTC, 3600000);
Adafruit_BME280 bme; // I2C
WiFiClient client;
WiFiUDP Udp;

// set pins
const int sclPin = D1;
const int sdaPin = D2;

// strings for graphite path
const String location = "livingroom.";
const String baseString = "example.esp.";

// graphite server address and port
IPAddress remoteIP(198,51,100,42); //graphite server
unsigned int remotePort = 2003;  //graphite server port (tcp)

// enable VCC measurement
ADC_MODE(ADC_VCC);

void setup() {
  // init i2c connection
  Wire.begin(sdaPin, sclPin);
  bme.begin(0x76);

  // init serial connection for debugging
  Serial.begin(9600);
  delay(200);
  pinMode(LED_BUILTIN, OUTPUT);

  WiFiMulti.addAP("wifi1_ssid", "password");
  WiFiMulti.addAP("wifi2_ssid", "password2");

  Serial.println();
  Serial.println();
  Serial.print("Wait for WiFi... ");

  // connect to wifi
  while(WiFiMulti.run() != WL_CONNECTED) {
      Serial.print(".");
      delay(500);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  //blinking led if wifi is connected and time is synced
  for (int i=1;i<10;i++){
    digitalWrite(LED_BUILTIN, LOW);
    delay(50);                      
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50); 
  }
}

void loop() {
  // initial time update via ntp
  timeClient.update();

  // init variables
  float temp = 0;
  float hum = 0;
  float pres = 0;
  float vcc = 0;
  int rssi = 0;
  String ssid = "";

  // read sensor data
  temp = bme.readTemperature();
  hum = bme.readHumidity();
  pres = bme.readPressure();
  vcc = ESP.getVcc()/1000.0;
  rssi = WiFi.RSSI();
  ssid = WiFi.SSID();

  // connect to server and send data via tcp
  client.connect(remoteIP, remotePort);
  writeTCP(client, "temp", temp);
  writeTCP(client, "hum", hum);
  writeTCP(client, "pres", pres);
  writeTCP(client, "vcc", vcc);
  writeTCP(client, "rssi." + ssid, rssi);
  // close tcp connection
  client.stop();

  // wait arround one minute for next value
  delay(55000);
}

// to enable some string concatenations we need this function to get byte arrays again
char* getCharFromString(String input){
    static char output[64] = "";
    input.toCharArray(output, 64);
    return output;
}

void writeTCP(WiFiClient client, String graphiteElement, float value){
    // build line for graphite
    char* finalElement = getCharFromString(baseString + location + graphiteElement + " " + value + " " + timeClient.getEpochTime());
    // print line to serial for debugging
    Serial.println(finalElement);
    // send line over tcp
    client.println(finalElement);
}

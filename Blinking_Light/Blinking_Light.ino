// Some of the code was from my previous lab and the various resources in those labs
// I also found a great article from https://aaronnelson95.com/IT441Lab6.php by Aaron

#include <ESP8266WiFi.h>
#define LIGHT_PIN D6

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include <Adafruit_MQTT_FONA.h>

// Code from Aaron

#define AIO_SERVER      "io.adafruit.com"       // Pulling data from the Adafruit website
#define AIO_SERVERPORT  1883                    // use 8883 for SSL
#define AIO_USERNAME    "<username>"            // Username for Adafruit (goes before the /feed/# MQTT feed)
#define AIO_KEY         "<key>"                 // Obtained by going to io.adafruit.com and clicking AIO Key link in top right. Copy the "Active Key" here

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
WiFiClient client;
Adafruit_MQTT_Client mqttAda(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
// Publish a feed called 'notificaitonTirdder'
Adafruit_MQTT_Publish notificationTrigger = Adafruit_MQTT_Publish(&mqttAda, AIO_USERNAME "/feeds/final-project");

// End of code from Aaron

// ------- Connection Info for WIFI -------

const char* ssid = "<ssid>";                      // SSID of the wifi network
const char* password = "<password>";              // PSK of the wifi network
const char* server = "<home-assistant-ip>";       // IP Address of the MQTT Broker
char* garageTopic = "/garage";
char* carTopic = "/car";
char* connectTopic = "/example/connect";
char* disconnectTopic = "/example/disconnect";

// ------- Hostname of this Arduino -------
String macAddr = WiFi.macAddress();      // Store arduino MAC address as a string
String host = "<blinking-light>";

// ------- Global Variables and Classes -------

bool bool_blink = false;
unsigned long autolight_timer = 0;
bool garage_open = false;
bool car_gone = false;
bool old_garage_open = false;
bool old_car_gone = false;
String message;
long timer;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

void setup() {
  pinMode(LIGHT_PIN, OUTPUT);
  digitalWrite(LIGHT_PIN, LOW);
  Serial.begin(9600);
  Serial.println("Blinking Light"); 

  delay(10); 
  
  Serial.print("Connecting to '"); 
  Serial.print(ssid);
  Serial.println("' network");

  WiFi.hostname(host);  
  WiFi.begin(ssid, password);

  // Connect device to Wifi
  while (WiFi.status() != WL_CONNECTED) { 
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  delay (1000); 
  
  // Home Assistant MQTT connection code from previous labs
  mqttClient.setServer(server, 1883); 
  mqttClient.setCallback(callback);

  Serial.println("Connecting to MQTT Broker");
  char hostChar[host.length()+1]; 
  host.toCharArray(hostChar, host.length()+1 );

  if (mqttClient.connect(hostChar, "test", "hello")) {  
    Serial.println("MQTT Connected");
    mqttClient.publish(connectTopic, hostChar);
    Serial.println(mqttClient.state());

  } else {
    Serial.println("MQTT Connection Failure");
    Serial.println(mqttClient.state());
  }

  // ------- MQTT Subscribe to a topic -------
  mqttClient.subscribe(garageTopic);
  mqttClient.subscribe(carTopic);
}

void loop() {
  
  mqttClient.loop();  // Read MQTT messages about the state of the garage
  MQTT_connect();   // this is our 'wait for incoming subscription packets' busy subloop


  // Code to run if the garage is open and the car is gone
  if (garage_open && car_gone) {

    // Only send a message to Adafruit feed once so you don't get multiple calls
    if (garage_open && (garage_open != old_garage_open)) {
      if (car_gone && (car_gone != old_car_gone)){
        // Send notificaiton to Ada Fruit Feed
        if (! notificationTrigger.publish(1)) {
          Serial.println(F("Failed"));
        } else {
          Serial.println(F("OK!"));
        }
      }
    }
    old_car_gone = car_gone;
    old_garage_open = garage_open;

    // Configure Blinking Light to be a non blocking loop
    Serial.print("Blinking Light\n");
    autolight_timer = millis() + 450;
    if (bool_blink) {
      digitalWrite(LIGHT_PIN, HIGH);
    }
    else if (!bool_blink) {
      digitalWrite(LIGHT_PIN, LOW);
    }
    bool_blink = !bool_blink;
  }
  else if (!garage_open && !car_gone) {
    // Change the states if hte garage isn't open and the car is here
    old_car_gone = car_gone;
    old_garage_open = garage_open;
    // Make sure the light is off
    digitalWrite(LIGHT_PIN, LOW);
  }
  else {
    // Turn off the light
    digitalWrite(LIGHT_PIN, LOW);
  }
  delay(500);
}

// Callback function modified from previous labs 
// The callback function is defined above in mqttClient.setCallback(callback)
//  We now have to write a function that matches the name "callback" to handle
//  incoming messages from the broker for any topics that we subscribe to
void callback(char* topicChar, byte* payload, unsigned int length) { // defined by pubSubClient
  String topic = (String)topicChar;     // Convert topic from char* to String

  // Only process incoming garageTopic messages
  if (topic == (String)garageTopic) {      // If the topic is Garage Topic
    String message = "";                  // Convert payload from byte* to String
    // MQTT won't send a null character to terminate the byte*, but it sends the
    // length, so we iterate thorugh the payload and copy each character
    for (int i = 0; i < length; i++) {    // Iterate through the characters in the payload array
      message += (char)payload[i];     //    and display each character
    }
    // Change the state of the relay when the garage door state changes
    if (message == "OPEN") {
      garage_open = true;
    }
    else if (message == "CLOSED") {
      garage_open = false;
    }
  }
  else if (topic == (String)carTopic) {      // If the topic is Garage Topic
    String message = "";                  // Convert payload from byte* to String
    // MQTT won't send a null character to terminate the byte*, but it sends the
    // length, so we iterate thorugh the payload and copy each character
    for (int i = 0; i < length; i++) {    // Iterate through the characters in the payload array
      message += (char)payload[i];     //    and display each character
    }

    // Change the state of the relay when the garage door state changes
    if (message == "HERE") {
      car_gone = false;
    }
    else if (message == "GONE") {
      car_gone = true;
    }
  }
}

// Function provided by Aaron
// Connects and reconnects as necessary to the Adafruit.io MQTT server.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqttAda.connected()) {
    return;
  }

  Serial.print("Connecting to Adafruit... ");

  uint8_t retries = 3;                  // It will attempt to reconnect 3 times before quitting entirely
  while ((ret = mqttAda.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqttAda.connectErrorString(ret));
       Serial.println("Retrying Adafruit connection in 5 seconds...");
       mqttAda.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;    // Count against one of the retries because there was no success
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("Adafruit Connected!");  // Connection was successful!
}

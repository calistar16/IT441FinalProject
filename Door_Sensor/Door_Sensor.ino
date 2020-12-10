// Some code was adapted from Todd Berrett's distance and MQTT examples in class and my previous labs
// Modifications have been made to meet the requirements of this lab

#include <ESP8266WiFi.h>
#define SENSOR D5

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// ------- Connection Info for WIFI -------

const char* ssid = "<ssid>";
const char* password = "<password>";
const char* server = "<home-assistant-ip>";
char* garageTopic = "/garage";
char* connectTopic = "/example/connect";
char* disconnectTopic = "/example/disconnect";

// ------- Hostname of this Arduino -------
String macAddr = WiFi.macAddress();
String host = "arduino-" + macAddr.substring(15);

// ------- Global Variables and Classes -------
bool isOpen = false;
bool oldIsOpen = false;
int state;
String message; 
long timer; 
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);  

void setup() {
  pinMode(SENSOR,INPUT_PULLUP);         // Sets the ECHO as an INPUT
  Serial.begin(9600);
  Serial.println("Garage Magnetic Door Sensor"); 

  delay(10); 

  // Connect to Wifi
  Serial.print("Connecting to '"); 
  Serial.print(ssid);
  Serial.println("' network");

  WiFi.hostname(host);
  WiFi.begin(ssid, password); 

  while (WiFi.status() != WL_CONNECTED) { 
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  delay (1000); 

  mqttClient.setServer(server, 1883);
  mqttClient.setCallback(callback);

  // Connect to MQTT broker
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
}

void loop() {
  mqttClient.loop();                    // Loop to check for new messages

  oldIsOpen = isOpen;
  isOpen = digitalRead(SENSOR);
  unsigned int len = 8;

  // Only send a message when the door state changes
  if (isOpen && (isOpen != oldIsOpen)){
    Serial.println("Door open");
    char* buf = "OPEN";
    mqttClient.publish(garageTopic, "OPEN", true);
  }
  else if (isOpen != oldIsOpen){
    Serial.println("Door closed");
    mqttClient.publish(garageTopic, "CLOSED", true);
  }
  delay(200);
}

//  The callback function is defined above in mqttClient.setCallback(callback)
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
    Serial.print("Message arrived [");    // Serial Debug
    Serial.print(topic);                  //    Print the topic name [in brackets]
    Serial.print("] ");                   //
    Serial.println(message);              //    Print the message
  }
  
}

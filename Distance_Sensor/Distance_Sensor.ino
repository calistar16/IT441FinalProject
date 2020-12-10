// Some code was adapted from Todd Berrett's distance and MQTT examples in class and my previous labs
// Modifications have been made to meet the requirements of this lab

#include <ESP8266WiFi.h>
#define ECHO D5
#define TRIG D7

long duration;      // variable for the duration of sound wave travel
long distance;      // variable to grab the distance measured (inches)

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// ------- Connection Info for WIFI -------
const char* ssid = "<ssid>";
const char* password = "<password>";
const char* server = "<home-assistant-ip>";
char* carTopic = "/car";
char* connectTopic = "/example/connect";
char* disconnectTopic = "/example/disconnect";

// ------- Hostname of this Arduino -------
String macAddr = WiFi.macAddress();
String host = "arduino-" + macAddr.substring(15) ;

// ------- Global Variables and Classes -------
bool bool_blink = false;
bool car_gone = false;
unsigned long autolight_timer = 0;
String message;
long timer;  
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient); 

void setup() {
  pinMode(TRIG, OUTPUT);        // Sets the TRIG as an OUTPUT
  pinMode(ECHO, INPUT);         // Sets the ECHO as an INPUT
  Serial.begin(9600);
  Serial.println("Garage Stoplight Distance Sensor");

  delay(10); 
  Serial.print("Connecting to '"); 
  Serial.print(ssid);
  Serial.println("' network");

  // Connect to Wifi

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

  // Connect to MQTT

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
  mqttClient.subscribe(carTopic);
}

void loop() {
  mqttClient.loop();                    // Loop to check for new messages
  
  // Code provided by Todd Berrett
  digitalWrite(TRIG, LOW);    // Turn off the trigger and let things quiet down
  delay(20);                  // Let it sit for 20 milliseconds
  digitalWrite(TRIG, HIGH);   // Turn on the trigger to start measurement
  delayMicroseconds(10);      // Send a very short pulse (10us)
  digitalWrite(TRIG, LOW);    // Turn off TRIG pin - this will start the ECHO pin high
  duration = pulseIn(ECHO, HIGH);   // Reads ECHO, returns the travel time in microseconds

  distance = duration*0.013504/2;
  Serial.print("Distance: "); // debug out
  Serial.print(distance); // Calculate the distance in inches (13,504 in/s)
  Serial.print("in. \n");
  // -- End of code provided by Todd Berrett
  
  // If statements to publish if the car is here or gone

  if (distance > 12) {
    // Car is not in garage
    Serial.print("Car Gone\n");
    delay(20000);
    mqttClient.publish(carTopic, "GONE");   // Send "GONE" to the Car Topic
  }
  else if (distance <= 12){
    // Car is in garage
    Serial.print("Car Here\n");
    mqttClient.publish(carTopic, "HERE");   // Send "HERE" to the Car Topic
  }
  delay(500);
}

//  The callback function is defined above in mqttClient.setCallback(callback)
//  We now have to write a function that matches the name "callback" to handle
//  incoming messages from the broker for any topics that we subscribe to
void callback(char* topicChar, byte* payload, unsigned int length) { // defined by pubSubClient
  String topic = (String)topicChar;     // Convert topic from char* to String
  if (topic == (String)carTopic) {      // If the topic is Garage Topic
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

  
    if (message == "HERE") {
      car_gone = false;       // change garage state to true
    } 
    else if (message == "GONE"){
      car_gone = true;       // change garage state to false
    }
  }
  
}

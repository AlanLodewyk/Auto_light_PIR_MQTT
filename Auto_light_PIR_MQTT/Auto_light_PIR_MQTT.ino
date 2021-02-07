#include <ESP8266WiFi.h>        // Include the Wi-Fi library
#include <ESP8266WiFiMulti.h>   // Include the Wi-Fi-Multi library
#include <OneWire.h>            // One Wire Library
#include <DallasTemperature.h>  // Dallas Temp Sensor Library
#include <ESP8266WebServer.h>   // WebServer Library
#include <PubSubClient.h>       // MQTT library   
#include <LiquidCrystal_I2C.h>  //LCD Library
//
ESP8266WiFiMulti wifiMulti;     // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'
ESP8266WebServer server(80);    // Create a webserver object that listens for HTTP request on port 80
LiquidCrystal_I2C lcd(0x27, 20, 4);
//
const char* Ver_Sion = "21207";
String IP_ADDR; 
String CON_DISP = "/"; 
int CON_COUNT = 0;
int   count= 0;
// Pins
// Old int Temp_Pin= 4;    //GPIO4  - D2
int Temp_Pin= 2;    //GPIO2  - D4
int Led_Pin=10;     //GPIO10 - D12
int Light_Pin= 0;   //GPIO0  - D3
int Relay_1_Pin= 0; //GPIO0  - D3
int Pir_Pin= 12;    //GPIO12 - D6
//
int Light_Value= 0; int Light_Threshold = 50; 
int Relay_1_Pin_State= 0; float Relay_1_Pin_Time= 0; float Relay_1_Pin_Duration= 20000; // Duration the Rely will stay On after no PIR trigger
int Pir_Pin_State= 0; int Pir_Pin_Prev_State= 0; int Pir_Pin_Time= 0;
int Loop_Delay = 5000; // Main Loop Delay 
//
byte mac_address[6]; 
//
OneWire oneWire(Temp_Pin);               // Set up a OneWire instance to communicate with OneWire devices
DallasTemperature tempSensors(&oneWire); // Create an instance of the temperature sensor class
float temperatureC, temperatureF;
//
// MQTT Setup
const char* mqtt_server = "172.30.0.120";
int mqtt_ping_result;
const char* mqtt_username = "admin";
const char* mqtt_password = "H0me122!";
const char* mqtt_OutTopic = "PirOut";
const char* mqtt_IntTopic = "PirIn";
const char* mqtt_ClientID = "bedroom1";
const char* Temp_Msg;
char mqtt_Msg[50], mqtt_Temp[50], mqtt_Light[50], mqtt_Relay[50];
int  mqtt_State = 0 ; 
float MQTT_Send = 0; float MQTT_Delay = 60000; // Send MQTT Message on this delay
//
String Temp_String, Light_String, Relay_String , Mqtt_String;
//
WiFiClient wifiClient; 
PubSubClient MQTT_Client(mqtt_server, 1883, wifiClient ); 
//
void MQTTConnect() {
        if (MQTT_Client.connect(mqtt_ClientID, mqtt_username, mqtt_password)) {
          //Serial.println("Connected to MQTT Broker!"); Serial.print(" rc="); Serial.println(MQTT_Client.state());
          delay(500);
        }
        else {
          Serial.println();
          Serial.print("Connection to MQTT Broker failed, rc="); Serial.print(MQTT_Client.state()); Serial.println(" try again in 2 seconds");
          delay(5000);
        }
    }
//
void MQTTSend() {
     MQTTConnect();
     dtostrf(temperatureC ,4 , 2, mqtt_Msg);
     Temp_String = String(temperatureC);
     Light_String = String(Light_Value);
     Relay_String = String(Relay_1_Pin_State);
     Mqtt_String = String(mqtt_ClientID) + ',' + Temp_String + ',' + Light_String + ',' + Relay_String ;
     //Serial.print("Temp_String=< "); Serial.print(Temp_String); Serial.println(" >");
     //Serial.print("Light_String=< "); Serial.print(Light_String); Serial.println(" >");
     //Serial.print("Relay_String=< "); Serial.print(Relay_String); Serial.println(" >");
     //Serial.print("Mqtt_String="); Serial.print(Mqtt_String); Serial.print(" -- Length="); Serial.println(Mqtt_String.length());
     Mqtt_String.toCharArray(mqtt_Msg,Mqtt_String.length()+1);
     MQTT_Client.publish(mqtt_OutTopic, mqtt_Msg);
     Serial.println(); Serial.print("mqtt_Msg="); Serial.print(mqtt_Msg);Serial.print(" rc="); Serial.println(MQTT_Client.state());
}
//
void setup() {
  // Start the Serial communication to send messages to the computer
  Serial.begin(115200); delay(10); Serial.println('\n');
  // Start the LCD to display messages
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);   lcd.print("This is AL_I0T.com");
  lcd.setCursor(10, 3); lcd.print("ver="); lcd.print(Ver_Sion); 
  // Pin Initialize
  pinMode(Led_Pin, OUTPUT); pinMode(Relay_1_Pin, OUTPUT);  pinMode(Pir_Pin, INPUT);  
  WiFi.mode(WIFI_STA); // Set mode to Station    
  // add Wi-Fi networks you want to connect to
  wifiMulti.addAP("NodeNCU", "PassWord");
  //wifiMulti.addAP("ASPNET", "PassWord");  
  // wifiMulti.addAP("ASPNETRpi", "PassWord");
  //
  Serial.println("Connecting ..."); 
  WiFi.macAddress(mac_address);
  Serial.print("MAC: 01-05__ "); 
  Serial.print(mac_address[0],HEX); Serial.print(":"); Serial.print(mac_address[1],HEX); Serial.print(":"); Serial.print(mac_address[2],HEX); Serial.print(":");
  Serial.print(mac_address[3],HEX); Serial.print(":"); Serial.print(mac_address[4],HEX); Serial.print(":"); Serial.println(mac_address[5],HEX);
  int i = 0;
  // Wait for the Wi-Fi to connect: scan for Wi-Fi networks, and connect to the strongest of the networks above
  while (wifiMulti.run() != WL_CONNECTED) { 
    delay(250);
    lcd.setCursor(0, 1);   lcd.print("Connecting ");
    CON_COUNT = CON_COUNT + 1;
    if (CON_DISP == "1") {
        Serial.print("+");    lcd.setCursor(12, 1);  lcd.print("+");
        CON_DISP = "0";
    }
    else {
        Serial.print("/");    lcd.setCursor(12, 1);  lcd.print("/");
        CON_DISP = "1";      
    }
    lcd.setCursor(14, 1);  lcd.print(CON_COUNT);
  }
  // Tell us what network we're connected to and Send the IP address of the ESP8266 to the computer
  //String IP_ADDR = WiFi.localIP();
  Serial.println('\n'); Serial.print("Connected to "); Serial.println(WiFi.SSID()); Serial.print(" My IP address:\t"); Serial.println(WiFi.localIP());
  lcd.setCursor(0, 2); lcd.print("IP="); lcd.print(WiFi.localIP());
  //
  digitalWrite(Relay_1_Pin, HIGH);
  delay(2000);
  lcd.clear();
  //
}
//
void loop() {
   // Read Temperature
   Light_Value= analogRead(Light_Pin);    Light_Value= Light_Value / 10; 
   tempSensors.requestTemperatures(); temperatureC = tempSensors.getTempCByIndex(0); temperatureF = tempSensors.getTempFByIndex(0); 
   //
   Serial.print("Vers="); Serial.print(Ver_Sion);
   Serial.print(" Count ="); Serial.print(++count);
   Serial.print(" _Light LEVEL : "); Serial.print(Light_Value); Serial.print("/"); Serial.print(Light_Threshold);
   Serial.print(" _Temp in C is: "); Serial.print(temperatureC);
   //Read PIR State
   Pir_Pin_State= digitalRead(Pir_Pin);
   Relay_1_Pin_State= digitalRead(Relay_1_Pin);
   //
   Serial.print(" _PIR Time is: "); Serial.print(Pir_Pin_Time / 1000);
   Serial.print(" _PIR State was: "); Serial.print(Pir_Pin_Prev_State);  Serial.print(" and is: "); Serial.print(Pir_Pin_State);
   Serial.print(" _Relay State is: "); Serial.print(Relay_1_Pin_State);
   // 
   if(Pir_Pin_State!= Pir_Pin_Prev_State && Light_Value > Light_Threshold ) {
        Serial.print("  *** PIR State Change from ");       Serial.print(Pir_Pin_Prev_State);
        Serial.print("  to "); Serial.print(Pir_Pin_State); Serial.print("  After ");
        Pir_Pin_Prev_State = Pir_Pin_State; Pir_Pin_Time= 0;
        //Activate relay and timer        
        digitalWrite(Relay_1_Pin, LOW);
        digitalWrite(Led_Pin, HIGH);
        Relay_1_Pin_Time = Relay_1_Pin_Duration;
        MQTTSend();
        Serial.print(" Above threshold");
        Serial.print("  Relay Time: "); Serial.println(Relay_1_Pin_Time);
   }
   //
   if (Relay_1_Pin_Time > 0 ) {
      Relay_1_Pin_Time = Relay_1_Pin_Time - Loop_Delay;
   }
   if (Relay_1_Pin_Time == 0 ) {
        Relay_1_Pin_Time= -1;
        digitalWrite(Relay_1_Pin, HIGH);
        digitalWrite(Led_Pin, LOW);
        Serial.println(""); Serial.println("  *** Relay State Change from ");
   }
   Serial.print(" _Relay Time is: "); Serial.print(Relay_1_Pin_Time);
   Serial.print(" _MQTT_Send Time is: "); Serial.print(MQTT_Send);
   Serial.println(" ");
   //
   // Publish MQTT data
   if (MQTT_Send > 0 ) {
      MQTT_Send = MQTT_Send - Loop_Delay;
   }
   else {
      MQTTSend();
      MQTT_Send = MQTT_Delay ;
   }
   //    lcd.setCursor(C, R)
   lcd.clear();
   lcd.setCursor(0, 0); lcd.print("IP="); lcd.print(WiFi.localIP());
   lcd.setCursor(0, 1); lcd.print("CTemp="); lcd.print(temperatureC);
   lcd.setCursor(12, 1); lcd.print("PIR="); lcd.print(Pir_Pin_State);
   lcd.setCursor(0, 2); lcd.print("Light="); lcd.print(Light_Value);
   lcd.setCursor(12, 2); lcd.print("Relay="); lcd.print(Relay_1_Pin_State);
   lcd.setCursor(10, 3); lcd.print("ver="); lcd.print(Ver_Sion);    
   //
   delay(Loop_Delay);
}

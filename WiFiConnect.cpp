#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "IoT.h"
#include "WiFiConnect.h"

char ssid[33];
char* password = (char*) malloc(65);
boolean havePassword = false;

const char APpassword[] = "thereisnospoon";                // for demo perposes hardcode password
ESP8266WebServer server(80);                              // HTTP server will listen at port 

String form = 
"<!DOCTYPE html>"
"<html>"
"<body>"
"<p>"
"<center>"
"<h1>WiFi setup V1.0</h1>"
"<form action=\"msg\">"
"SSID: <input type=\"text\" name=\"SSID\"><br>"
"PASSWORD: <input type=\"text\" name=\"Password\"><br>"
"<input type=\"submit\" value=\"Submit\">"
"</form>"

"<p>Click the \"Submit\" button and the ESP8266 will try to connect to SSID given.</p>"
"</body>"
"</html>";

int LED = LOW;
const int  buttonPin = 0;    // the pin that the pushbutton is attached to
int lastButtonState = LOW;   // the previous reading from the input pin
uint32_t buttonTime = millis();

void testWifi(char*, char*);    
void setupAP(void);     //setup softAP
void handle_msg(void);  //handle_msg callback for dealing with html pages 
boolean handleButton();;

void setupWiFi () {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  Serial.println("\nscan start - V.02");

  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; ++i) {
    int str_len = WiFi.SSID(i).length() + 1;
    WiFi.SSID(i).toCharArray(ssid, str_len);
Serial.print(ssid);Serial.print("=");Serial.println(ePromGet (0,32));
    if (strcmp(ssid, ePromGet (0,32)) == 0) {
      havePassword = true;
Serial.println(String("Have Password") + ePromGet(33,64));
    }
  }

    testWifi(ePromGet (0,32), ePromGet (33,64));
//  digitalWrite(BUILTIN_LED, LED = LOW);
}

void testWifi(char* ssid, char* password) {
  WiFi.begin(ssid,password);
  Serial.print("Waiting for Wifi to connect. ");  
  while (true){
    if (handleButton()) return;
      Serial.print(".");
      digitalWrite(BUILTIN_LED, LED = !LED);
      if (WiFi.status() == WL_CONNECTED) {
Serial.println (String("Try SSID=")+ssid+"="+ password);
        if (!havePassword) {
          ePromPut (0, ssid, 32);
Serial.println (String("Save SSID=") +ssid+ "="+ password);
          ePromPut (33,password,64);
//          EEPROM.commit();
        }
        Serial.println("");
        Serial.println("WiFi connected.");
        Serial.println(WiFi.SSID());
        Serial.println(WiFi.localIP());     
        return;
      } 
      delay(500);
  }
} 

void setupAP(void){
  Serial.println ("\nOpen AccessPoint");
  havePassword = false;
//  EEPROM.commit();

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  
  uint8_t mac[WL_MAC_ADDR_LENGTH];
  WiFi.softAPmacAddress(mac);
  String macID = String(mac[WL_MAC_ADDR_LENGTH - 2], HEX) +
                 String(mac[WL_MAC_ADDR_LENGTH - 1], HEX);

  macID.toUpperCase();
  String AP_NameString = "ESP8266-" + macID;

  char AP_NameChar[AP_NameString.length() + 1];
  memset(AP_NameChar, 0, AP_NameString.length() + 1);

  for (int i=0; i<AP_NameString.length(); i++)
    AP_NameChar[i] = AP_NameString.charAt(i);

  WiFi.softAP(AP_NameChar, APpassword);
  server.on("/", []()
  {
    server.send(200, "text/html", form);
  });
  server.on("/msg", handle_msg);
  server.begin();
  long blink = millis();
  while(WiFi.status() != WL_CONNECTED){
    if (millis()-blink > 100) {
      blink = millis();
      digitalWrite(BUILTIN_LED, LED = !LED);
      yield();
    }
    server.handleClient();
  }
}

boolean handleButton() {
    int buttonState = digitalRead(buttonPin);
    if (lastButtonState != buttonState) {
      if (buttonState == LOW) {
         setupAP();
         return true;
      }
      lastButtonState = buttonState;
    }
  return false;
}

void handle_msg(void){
  server.arg("SSID").toCharArray(ssid, server.arg("SSID").length() + 1);
  server.arg("Password").toCharArray(password, server.arg("Password").length() + 1);

  Serial.println("Connecting to local Wifi"); //Close the AP and connect with new SSID and P/W
  WiFi.softAPdisconnect(true);
  delay(500);
Serial.println(String("New SSID=") + ssid + " / " + password);
  testWifi(ssid, password);
}


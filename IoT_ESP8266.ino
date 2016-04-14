#include <ESP8266WiFi.h>
#include "IoT.h"
#include "WiFiConnect.h"

extern "C" {  //required for read Vdd Voltage
#include "user_interface.h"
  // uint16 readvdd33(void);
}

int count=0;
char countBuf[20];

#define Version 2


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  delay(10);
  setupIoT(Version);
  properties.declarePersistent ("Name", 32);
  properties.declarePersistent ("Description", 256);
  properties.updateProperty ("Description", "Now is the time to describe all devices in the system");
  properties.updateProperty ("LED", "0");
  properties.updateProperty ("Counter", "0");

#if Version == 1
Serial.println ("Version 1");  // 13943879
  properties.updateProperty ("Name", "IoT - 1");
  properties.updateProperty ("HTML", "<table border=0 id='dataValues'><tr><td><input type='text' id='Name' value='*'></td></tr><tr><td><input type='text' id='Description' value='..'></td></tr><tr><td><select name='LED' id='LED' data-role='slider' data-theme='c'><option value=0>Off</option><option value=1>On</option></select></td></tr> <tr><td><input type='text' id='Counter' value='*'></td></tr></table>");

#elif Version == 2 
Serial.println ("Version 2");  // 13945914
  properties.updateProperty ("Name", "IoT - 2");
  properties.updateProperty ("HTML", "<table border=0 id='dataValues'><tr><td><input type='text' id='Name' value='*'></td></tr><tr><td><input type='text' id='Description' value='..'></td></tr><tr><td><select name='LED' id='LED' data-role='slider' data-theme='c'><option value=0>Off</option><option value=1>On</option></select></td></tr> <tr><td><input type='text' id='Counter' value='*'></td></tr></table>");
  rules.updateProperty ("LedRule", "Change,LED,Send,13943879,LED,");

#elif Version == 3 
Serial.println ("Version 3"); // 13945482
  properties.updateProperty ("Name", "IoT - 3");
  rules.updateProperty ("LedRule", "Change,LED,Send,13945914,LED,");
  properties.updateProperty ("HTML", "<table border=0 id='dataValues'><tr><td><input type='text' id='Name' value='*'></td></tr><tr><td><input type='text' id='Description' value='..'></td></tr><tr><td><select name='LED' id='LED' data-role='slider' data-theme='c'><option value=0>Off</option><option value=1>On</option></select></td></tr><tr><td><div class='gauge1'></div></td></tr><tr><td><input type='text' id='Counter' value='*'></td></tr></table>");
  scripts.updateProperty ("Counter", "var gauge = new Gauge($('.gauge1'), {value: ($('#Counter').val()%10)*10})");
#endif
}

void loop() {
  if (handleButton()) return;
  count++;
  if (count%2000 == 0) {
    snprintf(countBuf, 20, "%d", count/2000);
    properties.updateProperty ("Counter", countBuf);
//    Serial.println(properties.getValue("Counter"));
  }
  readUDP();
  delay(5);
}

void Properties::doAction (char* name, char* value) {
  if (strcmp(name, "LED")==0) {
    if (strcmp(value, "0")==0) LED = HIGH;
    else                  LED = LOW;
    digitalWrite(BUILTIN_LED, LED);
//  } else if (strcmp(name, "Counter")==0){
//    IPAddress ipAddr = (byte*) ipTable.getValue("0");
//    writeUDP (ipAddr, String("Write,") + "Counter" +","+ properties.getValue("Counter") +",");  
  }
}



/*............................................................................................
 * WiFiSetup
 * ESP8266 setup code to create an access point and publish a web page to ask for the 
 * SSID and Password of the user's home network and then connects.  Once a succesful 
 * connectin has been made the SSID and Password are stored to automatically log into 
 * the user's network the next time the device is powered up. 
 * 
 *   The led blinks once per second while connecting and 5 times per second if it created
 *   an access point and is waiting for user input.  
 *   
 *   The user can press for 1 second the flash button at any time to cause the device to 
 *   forget the current SSID and password and set up an access point and web page to ask 
 *   for a new SSID and Password
 *   
 *   Once the connection to the user's network is made, the device sends a UDP
 *   message to discover other IoT devices on the network and stores the results
 *   in a table of device serial numbers and addresses.
 *   
 * Code by members of the Artisan's Asylum, Somerville MA
 *
 *
\*.............................................................................................*/


#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <WiFiUDP.h>
#include "IoT.h"
#include "WiFiConnect.h"

extern "C" {  //required for read Vdd Voltage
#include "user_interface.h"
  // uint16 readvdd33(void);
}

const char* US = "\037";  //UNIT SEPARATOR

void discover();
char localIP[16];
void readUDP();
void writeUDP (IPAddress, String);
unsigned int localPort = 12345;      // local port to listen for UDP packets
char packetBuffer[512]; //buffer to hold incoming and outgoing packets
byte broadcastIP[] = {255,255,255,255};
void debug (String);
ValueTable scripts, rules, ipTable;
Properties properties;
int version;

// A UDP instance to let us send and receive packets over UDP
WiFiUDP Udp;

void setupIoT(int _version) {
  char systemId[5];
  EEPROM.begin(4096);
  version = _version;
  Udp.begin(localPort);
  setupWiFi();
        WiFi.localIP().toString().toCharArray(localIP,16);
        ipTable.updateProperty (ltoa(system_get_chip_id(), systemId, 10),localIP);
        discover();
}

char* nuChars (char* value, int size=-1) {
    if (size == -1)  size = strlen(value)+1;
    char* buffer = (char*) malloc(size); 
    strcpy (buffer, value);
    return buffer;
}
int ePromAddr = 100;

char* ePromGet (int addr, int size) {
  char* value = (char*) malloc (size+1);
  for (int j=0; j<size+1; j++) {
    value[j] = EEPROM.read(addr++);
  }
  return value; 
}

int ePromPut (int addr, char* value, int size) {
  for (int j=0; j<size+1; j++) {
    EEPROM.write(addr++, value[j]);
  }
  EEPROM.commit();
  return addr; 
}

int ValueTable::findProperty (char* name) {
  for (int propIdx=0; propIdx<numProperties; propIdx++) {
    if (strcmp(valueTable[propIdx]->name, name) == 0) return propIdx;
  }
  return -1;
}

int ValueTable::findIP (char* value) {
  int j;
  for (int propIdx=0; propIdx<numProperties; propIdx++) {
    for (j=0; j<4; j++) {
      if (valueTable[propIdx]->value[j] != value[j]) break;
    }
//Serial.println (String("findIP=")+valueTable[propIdx]->value[j]);
    if (j == 4) return propIdx;
  }
  return -1;
}

int ValueTable::ePromWrite (char* value, int size, int addr) {
debug (String("ePromWrite, value=") + value + ", size=" + size + ", addr=" + addr);
    for (int i=0; i<size-1; i++) {
        if (addr > 4094) {
            EEPROM.write(addr, 0);
            return addr;
        }
        EEPROM.write(addr++, value[i]);
    }
 debug ("ePromWrite Done");
    EEPROM.commit();
    return addr;
}

char* ValueTable::ePromRead() {
    int count =  (uint) EEPROM.read(ePromAddr++);
    char* field = (char*) malloc(count);
    for (int i=0; i<count; i++) {
        if (ePromAddr > 4094) return "";
        field[i] = EEPROM.read(ePromAddr++);
    }
    return field;
}

void ValueTable::nuEpromField (char* value, int size) {
    EEPROM.write(ePromAddr++, size+1);
    ePromAddr = ePromWrite (value, size, ePromAddr);
}
    
int ValueTable::nuEpromProperty (char* name, char* value, int size) {
    nuEpromField (name, strlen(name));
    int result = ePromAddr +1;
    nuEpromField(value, size);
    return result;
}

int ValueTable::declareProperty (char* name, int size, int eProm) {
debug (String("declareProperty=")+ name + ", size=" + size + ", eProm=" + eProm + ",numProperties=" + numProperties);
    valueTable[numProperties] = (struct NamedValue *) malloc(sizeof(struct NamedValue));
    valueTable[numProperties]->eProm = eProm;
    valueTable[numProperties]->name = nuChars (name);
    valueTable[numProperties]->value = nuChars ("", size);
    valueTable[numProperties]->changed = false;
    valueTable[numProperties]->size=size;
    return numProperties++;
}

void ValueTable::loadEprom () {
    char *name;
    while (ePromAddr) {
      valueTable[numProperties]->name, ePromRead();
      valueTable[numProperties]->eProm, ePromAddr +1;
      valueTable[numProperties]->size = (uint) EEPROM.read(ePromAddr);
      valueTable[numProperties]->value, ePromRead();
      valueTable[numProperties]->changed = false;
      if (numProperties++ > 99) return; 
    }
}

void ValueTable::updateProperty (char* name, String value, int size) {
  char byteValue[value.length() + 1];
  value.toCharArray(byteValue, value.length() + 1);
  updateProperty (name, byteValue, size);
}

void ValueTable::updateProperty (char* name, char* value, int size) {
  int propIdx = findProperty (name);
  if (size == -1) size = strlen(value) +1;
  if (size<32) size= 32;
  if (propIdx == -1) propIdx = declareProperty (name, size);
debug (String("updateProperty=")+ name + ", value=" + value + ", size=" + size + ",  changed=" + valueTable[propIdx]->changed);
  if (strcmp(valueTable[propIdx]->value, value) !=0) valueTable[propIdx]->changed = true;
  strcpy (valueTable[propIdx]->value, value);
  if (valueTable[propIdx]->eProm != -1)
      ePromWrite (value, size, valueTable[propIdx]->eProm);
}

int ValueTable::initializePersistent (char* name, char* value, int size) {
    int propIdx = findProperty (name);
    if (propIdx = -1) {
        declareProperty (name, size, nuEpromProperty (name, value, size));
        updateProperty (name, value);
    }
}

int ValueTable::declarePersistent (char* name, int size) {
debug (String("declarePersistent=")+ name + ", size=" + size);
    declareProperty (name, size, nuEpromProperty(name, "", size));
}

char* ValueTable::getValue (char* name) {
  int propIdx = findProperty(name);
  if (propIdx == -1) return"";
  return valueTable[propIdx]->value;
}

char* ValueTable::getValue (int propIdx) {
  if (propIdx == -1) return"";
  return valueTable[propIdx]->value;
}


char* ValueTable::getFromIP (char* ip) {
  int propIdx = findIP(ip);
  if (propIdx == -1) return "";
  return valueTable[propIdx]->name;
}

boolean ValueTable::changed (char* name) {
  int propIdx = findProperty(name);
  if (propIdx == -1 || !valueTable[propIdx]->changed) return false;
//debug (String("changed, ") + name + " = " + valueTable[propIdx]->changed);
//  Serial.println (String("Changed = ") + name);
  valueTable[propIdx]->changed = false;
  return true;
}

  void Properties::updateProperty (char* name, char* value, int size) {
    ValueTable::updateProperty (name, value, size);
    doAction(name, value);
  }
//  void doAction (char* name, char* value);


void discover() {
  debug ("\nSend Discover");
  writeUDP (broadcastIP, String("Discover") + US + String(system_get_chip_id()) +US+ version +US);
     delay(50);
}

void writeUDP (IPAddress ip, String value) {// byte*
//debug (String("++..SendUDP=") + ", " + value + ", " +ip);
  char byteValue[value.length() + 1];
  value.toCharArray(byteValue, value.length() + 1);
  delay (random(3, 23));
  Udp.beginPacket(ip, localPort);
  Udp.write(byteValue, value.length());
  int err = Udp.endPacket(); 
debug (String("*...SendUDP=") +err+ ", " + value + ", " +ip);
}

void ruleAction () { // Set,name,value  Send,serial,name, read,serial,name
  char* action = (strtok(NULL, ","));
//debug (String("ruleAction = ") + action);
  if (strcmp(action, "Set")==0) {
    properties.updateProperty (strtok(NULL, ","), strtok(NULL, ","));
  } if (strcmp(action, "Send")==0) {
    IPAddress ipAddr = (byte*) ipTable.getValue(strtok(NULL, ","));
    char* name = strtok(NULL, ",");
    writeUDP (ipAddr, String("Write") + US + name +US+ properties.getValue(name) +US);
  } if (strcmp(action, "Read")==0) {
    IPAddress ipAddr = (byte*) ipTable.getValue(strtok(NULL, ","));
    char* name = strtok(NULL, ",");
    writeUDP (ipAddr, String("Read") + US + name +US);
  }
}

boolean changeTrigger () {
//debug ("ChangeTrigger");
  return properties.changed(strtok(NULL, ","));
}

boolean greaterTrigger () {
  char* name = strtok(NULL, ",");
  int value = atoi( strtok(NULL, ","));
  return atoi(properties.getValue(name)) > value;
}

boolean lessTrigger () {
  char* name = strtok(NULL, ",");
  int value = atoi( strtok(NULL, ","));
  return( atoi(properties.getValue(name)) < value);
}

boolean intervalTrigger() {
  char* name = strtok(NULL, ",");
  int start  = atoi( strtok(NULL, ","));
  int seconds = atoi( strtok(NULL, ","));
  return (start%seconds == 0);
}
void runRules () {
  char rule[256];
  for (int ruleIdx=0; ruleIdx<rules.numProperties; ruleIdx++) {
    char* ruleBuf = rules.valueTable[ruleIdx]->value;
    strcpy (rule, ruleBuf);
//debug(String("runRule=")+rule + ", num=" + rules.numProperties);
    char* trigger = (strtok(rule, ","));
    if (strcmp(trigger, "Change")==0) {  //Change,name,delta,send,serial,name
      if (changeTrigger()) ruleAction();
    } else if (strcmp(trigger, "Less")==0) {
      if (lessTrigger()) ruleAction();
    } else if (strcmp(trigger, "Greater")==0) {
      if (greaterTrigger()) ruleAction();
    } else if (strcmp(trigger, "Interval")==0) { //Interval, <now>, seconds, name, 
      if (intervalTrigger()) ruleAction();
    }
  }
}

void readUDP () {
  int noBytes = Udp.parsePacket();
  String received_command = "";
  
  if ( noBytes ) {
    // We've received a packet, read the data from it
    Udp.read(packetBuffer,noBytes); // read the packet into the buffer
debug(String("*...ReceiveUDP=") + packetBuffer+", IP=" + (IPAddress) Udp.remoteIP());
    char* command = (strtok(packetBuffer, US));
    Udp.flush();
    if (strcmp(command, "Discover")==0) {
      char* serial =  (strtok(NULL, US));
      ipTable.updateProperty (serial, (char*) &Udp.remoteIP()[0], 4);
      writeUDP (broadcastIP, String("Update") + US + String(system_get_chip_id()) +US+properties.getValue("Name") +US+ version +US);

    } else if (strcmp(command, "Update")==0) {
      char* serial =  (strtok(NULL, US));
      ipTable.updateProperty (serial, (char*) &Udp.remoteIP()[0], 4);

    } else if (strcmp(command, "Write")==0) {
      char* name = strtok(NULL, US);
      properties.updateProperty (name, strtok(NULL, US));
      writeUDP (Udp.remoteIP(), String("Ack") + US + name + US);

    } else if (strcmp(command, "Read")==0) {
      char* name = strtok(NULL, US);
      writeUDP (Udp.remoteIP(), String("Write") + US + name + US + properties.getValue(name)+US);

    } else if (strcmp(command, "Formats")==0) {
      writeUDP (Udp.remoteIP(), String("Write") + US + "HTML" + US + properties.getValue("HTML")+US);
      for (int propIdx=0; propIdx<scripts.numProperties; propIdx++) {
        writeUDP (Udp.remoteIP(), String("Script") + US + scripts.valueTable[propIdx]->name + US + scripts.valueTable[propIdx]->value+US);
      }  
    } else if (strcmp(command, "Subscribe")==0) { //Subscribe, name, delta
      char* name = strtok(NULL, US);
//      char* delta = ( strtok(NULL, US));
      char* serial = ipTable.getFromIP((char*) &Udp.remoteIP()[0]);
      char* ruleName = (char*) malloc(strlen(name)+25);
      snprintf(ruleName, strlen(name)+25, "%s%s", serial, name);
      rules.updateProperty (ruleName, String("Change,") + name + ",Send," + serial + "," + name);
//debug (String("Rule=") + ruleName + "--"+ rules.getValue(ruleName));
    }
    while ((command = strtok(NULL, " ")) != NULL);
  }
    runRules();
}

void debug (String msg) {
//  return;
  Serial.println (msg);
  Serial.flush();
}




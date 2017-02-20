#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         
#include <ESP8266SSDP.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <ESP8266NetBIOS.h>

#define LISTEN_WEBSERVER_PORT 80
#define DEVICE_ID "DM0004"
#define DEVICE_NAME "DM_moving_device"
#define DEVICE_MODEL_NAME "DM4MB"
#define COMM_DEBUG_MODE false
int myChipId =666666;
ESP8266WebServer HTTP(LISTEN_WEBSERVER_PORT);
char static_ip[16] = "2.1.1.95";
char static_gw[16] = "2.1.1.1";
char static_sn[16] = "255.255.255.0";
const char *form = "<center><form action='/'>" 
   "<button name='dir' type='submit' value='1'>Forward</button><br>"
   "<button name='dir' type='submit' value='2'>Reverse</button><p>"
   "</form></center>";  
#define STEPS 200
#define T_STEP 800
#define PIN_STEP 4
#define PIN_DIRE 5
int previous = 0;
int val=0;

void forward(void) {
  digitalWrite(PIN_DIRE,HIGH);
  for(val = 0; val < STEPS; val++) { 
    digitalWrite(PIN_STEP,HIGH); 
    delayMicroseconds(T_STEP); 
    digitalWrite(PIN_STEP,LOW); 
    delayMicroseconds(T_STEP); 
  } 
}  

void backward(void) {
  digitalWrite(PIN_DIRE,LOW);
  for(val = 0; val < STEPS; val++) { 
    digitalWrite(PIN_STEP,HIGH); 
    delayMicroseconds(T_STEP); 
    digitalWrite(PIN_STEP,LOW); 
    delayMicroseconds(T_STEP); 
  } 
}   

void handle_form() {     
     if (HTTP.arg("dir"))  {
         int direction = HTTP.arg("dir").toInt();
         switch (direction)   {
             case 1:  forward();
                  break;             
             case 2:  backward();
                 break;
          }           
     }       
   HTTP.send(200, "text/html", form); 
}   

void SSDP_init(void){
        SSDP.setDeviceType("upnp:rootdevice");
        SSDP.setSchemaURL("description.xml");
        SSDP.setHTTPPort(LISTEN_WEBSERVER_PORT);
        SSDP.setName(DEVICE_NAME);
        myChipId=ESP.getChipId();
        SSDP.setSerialNumber(myChipId);
        SSDP.setURL("index.html");
        SSDP.setModelName(DEVICE_MODEL_NAME);
        SSDP.setModelNumber(DEVICE_MODEL_NAME);
        SSDP.setModelURL("http://digitalmy.ru/wSmartHome.php");
        SSDP.setManufacturer("DM");
        SSDP.setManufacturerURL("http://digitalmy.ru");
        SSDP.setTTL(2);
        SSDP.begin();
}

void HTTP_init(void){
        HTTP.on("/index.html", handle_form);
        HTTP.on("/description.xml", HTTP_GET, [](){
          SSDP.schema(HTTP.client());
        });
        HTTP.on("/", handle_form);
        HTTP.onNotFound(handle_form);
        HTTP.begin();
}
void VIMA_init(void){
        WiFiManager wifiManager;
        wifiManager.setDebugOutput(COMM_DEBUG_MODE);
        wifiManager.setAPStaticIPConfig(IPAddress(1,1,1,1), IPAddress(1,1,1,1), IPAddress(255,255,255,0));
        if(static_ip){
          IPAddress _ip,_gw,_sn;
          _ip.fromString(static_ip);
          _gw.fromString(static_gw);
          _sn.fromString(static_sn);
          wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn);
        }
        wifiManager.autoConnect("ESP8266DM");
}

void setup() {
   VIMA_init();
   HTTP_init();
   SSDP_init();
        if (MDNS.begin("ESP8266DM")) {
          MDNS.addService("http", "tcp", LISTEN_WEBSERVER_PORT);
        }
        NBNS.begin("ESP8266DM");
        String adDr=WiFi.localIP().toString();
        EEPROM.begin(512);
    
    pinMode(PIN_DIRE, OUTPUT); 
    pinMode(PIN_STEP, OUTPUT); 
    digitalWrite(PIN_DIRE,LOW);
    digitalWrite(PIN_STEP,LOW);
}   
void loop() {
  HTTP.handleClient(); 
}


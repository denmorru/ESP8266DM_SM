/* DM stepper motor control with esp8266 */
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         
#include <ESP8266SSDP.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <ESP8266NetBIOS.h>

#define USE_ENCODER
//#define USE_JOYSTICK
#define ESP8266DM_SM
#define LISTEN_WEBSERVER_PORT 80
#define DEVICE_ID "DM0004"
#define DEVICE_NAME "DM_moving_device"
#define DEVICE_MODEL_NAME "DM4MB"
#define COMM_DEBUG_MODE true
#define COMM_SERIAL_SPEED 57600
#define COMM_DEBUG_PORT Serial 
int myChipId =666666;
ESP8266WebServer HTTP(LISTEN_WEBSERVER_PORT);
char static_ip[16] = "2.1.1.95";
char static_gw[16] = "2.1.1.1";
char static_sn[16] = "255.255.255.0";
const char *form = "<center><form action='/'>" 
   "<button name='dir' type='submit' value='1'>Forward</button><br>"
   "<button name='dir' type='submit' value='4'>Stop</button>"
   "<button name='dir' type='submit' value='3'>Origin</button><br>"
   "<button name='dir' type='submit' value='2'>Rearward</button><p>"
   "<button name='spd' type='submit' value='1'>Faster</button><br>"
   "<button name='spd' type='submit' value='2'>Slower</button><p>"
   "</form></center>";  
String webSiteContent="";
#ifdef USE_JOYSTICK
  #define PIN_JOY_A A0 //analog read angle
  #define PIN_JOY_B 2 //digital read
  #define DEADBANDLOW 482  
  #define DEADBANDHIGH 542
#endif
#ifdef USE_ENCODER
  #define PIN_ENC_A 12 //digital read A
  #define PIN_ENC_B 13 //digital read B
  #define PIN_ENC_C 14 //digital read button pressed
#endif
#define PIN_LSW_1 2 //digital read limit switch (endstop)

#define PIN_STEP 4
#define PIN_DIRE 5
#define MIN_SPEED 5 //mm per second
#define MAX_SPEED 55 //mm per second
#define INC_SPEED 10 //change speed, mm per second
#define STEPS 5 //steps per mm, GT2 belt 20T pulley gives on full step 200/20=10
#define T_STEP 500 //microsecond (full step 800)
#define HOME_POS 260 //mm from endstop
#define MIN_POS 10 //mm from endstop
#define MAX_POS 510 //mm from endstop
int previous = 0;
int val=0; //speed, mm per second
double pon=0.00; //current position, mm
int dir=1; // 1-forward, 2-rearward
int JA_value;
int JB_value;
#ifdef USE_ENCODER
  int EA_value;
  int EB_value;
  int EC_value;
#endif
int LS_value;
unsigned long previousMillis = 0;

void forward(void) {
  dir=1;
  digitalWrite(PIN_DIRE,HIGH);
 return;
}  
void backward(void) {
  dir=2;
  digitalWrite(PIN_DIRE,LOW);
 return;
}   
void stopward(void) {
  dir=0;
  //digitalWrite(PIN_DIRE,LOW);
 return;
} 
void originward(void) {
  dir=3;
  val=(MIN_SPEED+MAX_SPEED)/2;
  pon=MAX_POS;
  digitalWrite(PIN_DIRE,LOW);
 return;
}  
void faster(void){
val=val+INC_SPEED;
  if (val < MIN_SPEED){val =MIN_SPEED;}
  if (val > MAX_SPEED){val =MAX_SPEED;}  
  return;
}
void slower(void){
val=val-INC_SPEED;
  if (val < MIN_SPEED){val =MIN_SPEED;}
  if (val > MAX_SPEED){val =MAX_SPEED;}  
  return; 
}
#ifdef USE_JOYSTICK
void handle_joystick ()
{
  JA_value = analogRead(PIN_JOY_A);
  JB_value = digitalRead(PIN_JOY_B);
  if(COMM_DEBUG_MODE){
  COMM_DEBUG_PORT.print (JA_value, DEC); COMM_DEBUG_PORT.print (" , "); COMM_DEBUG_PORT.print (JB_value, DEC); COMM_DEBUG_PORT.println(" ");
  //delay (100);
  }
}
#endif
void handle_encoder ()
{
  EA_value = digitalRead(PIN_ENC_A);
  EB_value = digitalRead(PIN_ENC_B);
  EC_value = digitalRead(PIN_ENC_C);
  if(COMM_DEBUG_MODE){
  COMM_DEBUG_PORT.print (EA_value, DEC);COMM_DEBUG_PORT.print (" , ");
  COMM_DEBUG_PORT.print (EB_value, DEC);COMM_DEBUG_PORT.print (" , ");
  COMM_DEBUG_PORT.print (EC_value, DEC);COMM_DEBUG_PORT.println(" ");
  }
}
void motor_go(){
  boolean dostep=false;
  unsigned long currentMillis = millis();
  double chg_pon=(1.00/STEPS); //mm from pon.
  int i;
  if(val>0 & dir>0){
  long interval = 1000/(STEPS*val); //millisecond per step = 1000ms/se / (st/mm * mm/se)
  if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      LS_value=digitalRead(PIN_LSW_1);
        if(LS_value>0){
          dostep=true;
          //LS_value=digitalRead(PIN_LSW_1);
        }else{
          dostep=false;
           if(dir==3){
              pon=0.00;
              stopward();
          }
        }

      if(dir==1 || dir==2){        
        if(pon>=MAX_POS){
          backward();
          //COMM_DEBUG_PORT.println(" back ");
          //new_pos=(pos/STEPS)+1;
        }
        if(pon<=MIN_POS){
          forward();
          //COMM_DEBUG_PORT.println(" forw ");
          //if(dir=1){}else{}
        }
      }
       if(dostep){  
        digitalWrite(PIN_STEP,HIGH); 
        delayMicroseconds(T_STEP); 
        digitalWrite(PIN_STEP,LOW); 
        delayMicroseconds(T_STEP); 
        if(dir==1){pon+=chg_pon;}
        else{pon+=-chg_pon;}
       }
        if(COMM_DEBUG_MODE){
        COMM_DEBUG_PORT.print (" Position: ");COMM_DEBUG_PORT.print (pon, DEC);
        COMM_DEBUG_PORT.print (" , Speed: ");COMM_DEBUG_PORT.print (val, DEC);
        COMM_DEBUG_PORT.print (" , Direction: ");COMM_DEBUG_PORT.print (dir, DEC);COMM_DEBUG_PORT.println(" ");
        }
     }
  }
}
void go_to_origin(){
}
void handle_form() {     
     if (HTTP.arg("dir"))  {
         int direction = HTTP.arg("dir").toInt();
         switch (direction)   {
             case 4:  stopward();
                  break;
             case 1:  forward();
                  break;             
             case 2:  backward();
                 break;
             case 3:  originward();
                 break;
        }  }
       if (HTTP.arg("spd"))  {
         int direction = HTTP.arg("spd").toInt();
         switch (direction)   {
             case 1:  faster();
                  break;             
             case 2:  slower();
                 break;
        }  }
        webSiteContent+="<div>Speed: "; 
        webSiteContent+=val; 
        webSiteContent+=" </div>";   
        webSiteContent+="<div>Direction: "; 
        webSiteContent+=dir; 
        webSiteContent+="</div>";
        webSiteContent+="<div>Position: "; 
        webSiteContent+=pon; 
        webSiteContent+="</div>";        
     
   webSiteContent+=form;      
   HTTP.send(200, "text/html", webSiteContent);
   webSiteContent="";
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
        wifiManager.autoConnect(DEVICE_MODEL_NAME);
}

void setup() {
   COMM_DEBUG_PORT.begin(COMM_SERIAL_SPEED);
   COMM_DEBUG_PORT.println(" ");
   COMM_DEBUG_PORT.println("Flash: ");
        COMM_DEBUG_PORT.println(ESP.getFlashChipSize());
   VIMA_init();
      COMM_DEBUG_PORT.println("WiFiManager initialized");
        HTTP_init();
        COMM_DEBUG_PORT.println("HTTP initialized");
        SSDP_init();
        COMM_DEBUG_PORT.println("SSDP initialized");
        if (MDNS.begin(DEVICE_MODEL_NAME)) {
          MDNS.addService("http", "tcp", LISTEN_WEBSERVER_PORT);
        }
        NBNS.begin(DEVICE_MODEL_NAME);
        String adDr=WiFi.localIP().toString();
        EEPROM.begin(512);
#ifdef USE_ENCODER
   pinMode(PIN_ENC_A,INPUT); 
   pinMode(PIN_ENC_B,INPUT);
   pinMode(PIN_ENC_C,INPUT);
#endif   
    pinMode(PIN_DIRE,OUTPUT); 
    pinMode(PIN_STEP,OUTPUT); 
#ifdef USE_JOYSTICK
    pinMode(PIN_JOY_A,INPUT); 
    pinMode(PIN_JOY_B,INPUT); //INPUT_PULLUP
#endif      
    pinMode(PIN_LSW_1,INPUT);
    digitalWrite(PIN_DIRE,LOW);
    digitalWrite(PIN_STEP,LOW);
}   
void loop() {
  HTTP.handleClient(); 
 // handle_encoder ();
//handle_joystick ();
  motor_go();
}



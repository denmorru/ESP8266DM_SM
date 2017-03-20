/* DM stepper motor control with esp8266 */
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         
#include <ESP8266SSDP.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <ESP8266NetBIOS.h>
#include <AccelStepper.h>

#define USE_ENCODER
//#define USE_JOYSTICK
#define ESP8266DM_SM
#define LISTEN_WEBSERVER_PORT 80
#define DEVICE_ID "DM0004"
#define DEVICE_NAME "DM_moving_device"
#define DEVICE_MODEL_NAME "DM4MB"
#define COMM_DEBUG_MODE false
#define COMM_SERIAL_SPEED 57600
#define COMM_DEBUG_PORT Serial 
int myChipId =666666;
ESP8266WebServer HTTP(LISTEN_WEBSERVER_PORT);
char static_ip[16] = "1.1.1.95";
char static_gw[16] = "1.1.1.1";
char static_sn[16] = "255.255.255.0";
const char *form = "<center><form action='/'>" 
   "<button name='dir' type='submit' value='1'>Forward</button><br>"
   "<button name='dir' type='submit' value='4'>Stop</button>"
   "<button name='dir' type='submit' value='3'>Origin</button><br>"
   "<button name='dir' type='submit' value='2'>Rearward</button><p>"
   "<button name='spd' type='submit' value='1'>Faster</button><br>"
   "<label>Speed</label><input name='val' type='text' value=''/><input type='submit' value='Go'/><br>"   
   "<button name='spd' type='submit' value='2'>Slower</button><p>"
   "<button name='ref' type='submit' value='1'>Refresh</button><p>"
   "</form></center>";  
String webSiteContent="";
#ifdef USE_JOYSTICK
  #define PIN_JOY_A A0 //analog read angle
  #define PIN_JOY_B 2 //digital read
  #define DEADBANDLOW 482  
  #define DEADBANDHIGH 542
  int JA_value;
int JB_value;
#endif
#ifdef USE_ENCODER
  #include <Encoder.h>
  #define PIN_ENC_A 12 //digital read A
  #define PIN_ENC_B 14 //digital read B
  #define PIN_ENC_C 13 //digital read button pressed
  Encoder myEnc(PIN_ENC_A, PIN_ENC_B);
  int EA_value;
  int EB_value;
  int EC_value;
  int EC_static=0;
  long oldPosition  = -999;
#endif
#define PIN_LSW_1 2 //digital read limit switch (endstop) PIN
#define LSW_ACT 2 //1- stop, 2-reverse movement
//#define MOT_ACT //AccelStepper
#define PIN_STEP 4
#define PIN_DIRE 5
#define MIN_SPEED 1 //mm per second
#define MAX_SPEED 500 //mm per second (100)
#define INC_SPEED 1 //change speed, mm per second
#define STEPS 80 //steps per mm, hardware dependent, GT2 belt 20T pulley gives: on full step: 200/40=5, on 0.0625step: 3200steps/40mm=80 steps per mm
#define M_STEP 16 //multiplier for microsteps
#define T_STEP 1 //microsecond delay on each step PWM (in full step - 200 steps round, in 0.0625step - 3200 steps round)
#define HOME_POS 110 //mm from endstop
#define MIN_POS 10 //mm from endstop
#define MAX_POS 210 //mm from endstop
#define ACC_RATE 50 //acceleration, mm/s^2
int previous = 0;
int val=50; //speed, mm per second
double realSpeed=0.00;  //mm per second
double pon=0.00; //current position, mm
int dir=1; // 1-forward, 2-rearward
int oldDir=3;
int LS_value;
unsigned long previousMillis = 0;
#ifdef MOT_ACT
AccelStepper stepper(1,PIN_STEP,PIN_DIRE); //1-use driver
#endif

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
  dir=4;
  //digitalWrite(PIN_DIRE,LOW);
 return;
} 
void originward(void) {
  dir=3;
  //val=(MIN_SPEED+MAX_SPEED)/2;
  pon=MAX_POS;
#ifdef MOT_ACT
     stepper.setCurrentPosition(MAX_POS*STEPS); stepper.moveTo(0);
#endif
  digitalWrite(PIN_DIRE,LOW);
 
 return;
}  
void faster(void){
val=val+INC_SPEED;
  if (val < MIN_SPEED){val =MIN_SPEED;}
  if (val > MAX_SPEED){val =MAX_SPEED;}  
#ifdef MOT_ACT
  stepper.setSpeed(val*STEPS); stepper.setMaxSpeed(val*STEPS);
  stepper.setAcceleration(1.00*val*STEPS);
#endif
 
}
void slower(void){
val=val-INC_SPEED;
  if (val < MIN_SPEED){val =MIN_SPEED;}
  if (val > MAX_SPEED){val =MAX_SPEED;}  
#ifdef MOT_ACT
  stepper.setSpeed(val*STEPS); stepper.setMaxSpeed(val*STEPS);
  stepper.setAcceleration(1.00*val*STEPS);
#endif
}
#ifdef USE_JOYSTICK
void handle_joystick ()
{
  JA_value = analogRead(PIN_JOY_A);
  JB_value = digitalRead(PIN_JOY_B);
  if(COMM_DEBUG_MODE){
  COMM_DEBUG_PORT.print (JA_value, DEC); COMM_DEBUG_PORT.print (" , "); COMM_DEBUG_PORT.print (JB_value, DEC); COMM_DEBUG_PORT.println(" ");
  }
}
#endif
#ifdef USE_ENCODER
void handle_encoder ()
{
   long newPosition = myEnc.read();
  if (newPosition != oldPosition) {
       if(newPosition>oldPosition){
          faster();
        }else{
          slower();
        }
    oldPosition = newPosition;
 
  }
  /*EA_value = digitalRead(PIN_ENC_A);
  EB_value = digitalRead(PIN_ENC_B);*/
  EC_value = digitalRead(PIN_ENC_C);
  if(EC_value==1){EC_static = EC_value;}
  if(EC_value==0 & EC_static==1){EC_static = EC_value;
    if(dir==1 || dir==2 || dir==3){  
      oldDir=dir;
      dir=4;}
    else {if(dir==4){
     dir=oldDir;  
    }}
   }
  if(COMM_DEBUG_MODE){
  COMM_DEBUG_PORT.print("Encoder: "); COMM_DEBUG_PORT.println(newPosition); 
  COMM_DEBUG_PORT.print("Button: ");COMM_DEBUG_PORT.println (EC_value, DEC);
  }
}
#endif

void motor_go(){
    boolean dostep=false;
    LS_value=digitalRead(PIN_LSW_1);
        if(LS_value>0){
          dostep=true;
        }else{
          pon=0.00;
     #ifdef MOT_ACT
          stepper.setCurrentPosition(0);
    #endif
          if(LSW_ACT==1){
            
          dostep=false;
           if(dir==3){ 
              oldDir=1;
              dir=4;
            }else{
              oldDir=2;
              dir=4;
            }
          }else{
            dir=1;
            dostep=true;
            //stepper.moveTo(MAX_POS*STEPS);
          }
        }
if(val>0 & dir>0 & dir<4){
#ifndef MOT_ACT

  unsigned long currentMillis = millis();
  double chg_pon=(1.00*M_STEP/STEPS); //next position, mm from pon.
  int i=0;
  
  double interval = ((1000.00*M_STEP)/(STEPS*val)); //millisecond per step = 1000ms/se / (st/mm * mm/se)
    if(COMM_DEBUG_MODE){
    COMM_DEBUG_PORT.print (" Intervval: ");COMM_DEBUG_PORT.println (interval, DEC);COMM_DEBUG_PORT.print(" Time: ");COMM_DEBUG_PORT.println (currentMillis, DEC);
    }
  if (currentMillis - previousMillis >= interval) {
    realSpeed=1000.00*chg_pon/(currentMillis - previousMillis);
      previousMillis = currentMillis;
     
          

      if(dir==1 || dir==2){        
        if(pon>=MAX_POS){
          backward();
        }
        if(pon<=MIN_POS){
          forward();
        }
      }
       if(dostep){  
        if(dir==1){pon+=chg_pon;}
        else{pon+=-chg_pon;}
       int delayinterval=1000*interval/(M_STEP*4); //microsecond
         //int delayinterval=T_STEP;
      for(i=0;i<M_STEP;i++){
          digitalWrite(PIN_STEP,LOW); 
          delayMicroseconds(delayinterval);
          digitalWrite(PIN_STEP,HIGH); 
          delayMicroseconds(delayinterval);  
          digitalWrite(PIN_STEP,HIGH); 
          delayMicroseconds(delayinterval);
          digitalWrite(PIN_STEP,LOW); 
          delayMicroseconds(delayinterval);
        } 
       }
     }
   
#endif
#ifdef MOT_ACT
        pon=stepper.currentPosition()/STEPS;
        if (stepper.distanceToGo() == 0){
          stepper.setSpeed(val*STEPS);
          stepper.setMaxSpeed(val*STEPS);
          if(stepper.currentPosition() >= MAX_POS*STEPS){
            dir=2;
            stepper.moveTo(MIN_POS*STEPS);
          }
          if(stepper.currentPosition() <= MIN_POS*STEPS){
            dir=1;
            stepper.moveTo(MAX_POS*STEPS);
          }
           if((stepper.currentPosition() > MIN_POS*STEPS )& (stepper.currentPosition() < MAX_POS*STEPS)){
            if(dir==1){stepper.moveTo(MAX_POS*STEPS);}
            else{stepper.moveTo(MIN_POS*STEPS);}
        }
      }
        //stepper.runSpeed();
        stepper.run();
 #endif  
 }

   if(COMM_DEBUG_MODE){
        COMM_DEBUG_PORT.print ("Position: ");COMM_DEBUG_PORT.print (pon, DEC);
        COMM_DEBUG_PORT.print (" , Speed: ");COMM_DEBUG_PORT.print (val, DEC);
        COMM_DEBUG_PORT.print (" , Direction: ");COMM_DEBUG_PORT.print (dir, DEC);
        COMM_DEBUG_PORT.print (" , RealSpeed: ");COMM_DEBUG_PORT.print (realSpeed, DEC);
        COMM_DEBUG_PORT.println(" ");
   }
}
void go_to_origin(){
  dir=3;
}
void handle_form() {     
     if (HTTP.arg("dir"))  {
         int directionc = HTTP.arg("dir").toInt();
         switch (directionc)   {
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
         int speedchg = HTTP.arg("spd").toInt();
         switch (speedchg)   {
             case 1:  faster();
                  break;             
             case 2:  slower();
                 break;
        }  }
        if (HTTP.arg("val"))  {
         int speedval = HTTP.arg("val").toInt();
         if(speedval>0)   {
          val=speedval; 
   #ifdef MOT_ACT
            stepper.setSpeed(val*STEPS);
            stepper.setMaxSpeed(val*STEPS);
            stepper.setAcceleration(1.00*val*STEPS);
   #endif
          }  
        }
        webSiteContent+="<div>Speed: "; 
        webSiteContent+=val; 
        webSiteContent+=" </div>";   
        webSiteContent+="<div>Direction: "; 
        webSiteContent+=dir; 
        webSiteContent+="</div>";
        webSiteContent+="<div>Position: "; 
        webSiteContent+=pon; 
        webSiteContent+="</div>";
        webSiteContent+="<div>RealSpeed: "; 
        webSiteContent+=realSpeed; 
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
  /* pinMode(PIN_ENC_A,INPUT); 
   pinMode(PIN_ENC_B,INPUT);*/
   pinMode(PIN_ENC_C,INPUT_PULLUP); //INPUT with resistor
#endif   
    pinMode(PIN_DIRE,OUTPUT); 
    pinMode(PIN_STEP,OUTPUT); 
#ifdef USE_JOYSTICK
    pinMode(PIN_JOY_A,INPUT); 
    pinMode(PIN_JOY_B,INPUT); //INPUT_PULLUP without resistor
#endif      
    pinMode(PIN_LSW_1,INPUT);
    //digitalWrite(PIN_DIRE,LOW);
    //digitalWrite(PIN_STEP,LOW);
#ifdef MOT_ACT
    stepper.setSpeed(val*STEPS);
    stepper.setCurrentPosition(MAX_POS*STEPS);
    stepper.setMaxSpeed(val*STEPS);
    stepper.setAcceleration(1.00*val*STEPS);
#endif
    originward();
}   
void loop() {
  handle_encoder ();
  //handle_joystick ();
  motor_go();
  
  HTTP.handleClient(); 

}




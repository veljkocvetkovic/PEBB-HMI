#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_GFX.h> // Core graphics library
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <HardwareSerial.h> // secondary serial port for ESP32
#include <SPI.h>
//#include <semphr.h>

#define RX 10
#define TX 11

// Use dedicated hardware SPI pins
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

const char* WLAN_SSID = "VTRI-Wireless";         
const char* WLAN_PASS = "ballstonwifi";  
const char* mqtt_user="mde_test";
const char* mqtt_pass="mde_test";
const char* mqtt_server="137.184.70.171";
const char* mqtt_topic_voltage="/pebb/voltage";
const char* mqtt_topic_current="/pebb/current";
const char* mqtt_topic_power="/pebb/power";
const char* mqtt_topic_setVoltage="/pebb/setVoltage";
const char* mqtt_topic_setCurrent="/pebb/setCurrent";

WiFiClient espclient;
PubSubClient client(espclient);
char msg[50];

const char* device_name="esp32";
String command;
String vorc;
char outmessage[100];
char outmessagetemp[100];
bool flag = false; //false == voltage, true == current

String tempV = "";
String tempC = "";

// global values updated from mqtt
String nVoltage = "";
String nCurrent = "";

String tempAmp = "";
String amplitude = "";

//SemaphoreHandle_t semaphor;

void setup(){

  // turn on backlite
  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, HIGH);

  // turn on the TFT / I2C power supply
  pinMode(TFT_I2C_POWER, OUTPUT);
  digitalWrite(TFT_I2C_POWER, HIGH);
  delay(10);

  // initialize TFT
  tft.init(135, 240); // Init ST7789 240x135
  tft.setRotation(3);
  tft.fillScreen(ST77XX_BLACK);

  Serial1.begin(115200, SERIAL_8N1, RX, TX);
  Serial.begin(115200);
  
  setup_wifi();
  client.setServer(mqtt_server,1883);
  client.setCallback(callback);

  tft.setCursor(0, 0);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setTextSize(3);
  tft.println("PEBB voltage: ");
  tft.setCursor(0, 70);
  tft.println("PEBB current: ");
  tft.setCursor(0, 40);

  //xTaskCreatePinnedToCore(MyTask_pointer, "task_name", 100, Parameter, Priority, TaskHandle, Core (0 or 1)) - create task
  //xTaskCreatePinnedToCore(Task_readValues, "Task_readValues", 4096, NULL, 1, NULL, 1); - not used
  xTaskCreatePinnedToCore(Task_sendValues, "Task_sendValues", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(Task_sendScreen, "Task_sendScreen", 4096, NULL, 1, NULL, 1);

  //semaphor = xSemaphoreCreateCounting(1, 1);
  //xSemaphoreGive(semaphor);

  //if(semaphor == NULL) Serial.println("UNABLE TO CREATE SEMAPHOR");
  //else Serial.println("SEMAPHOR CREATED SUCCESSFULLY");

  vTaskStartScheduler();
}

void Task_readValues(void *parameters) {
  (void)parameters;
  
  for(;;) {

    //xSemaphoreTake(semaphor, portMAX_DELAY);
    //xSemaphoreGive(semaphor);
  }
}

// send values through serial
void Task_sendValues(void *parameters) {
  for(;;) {
    
    //Serial1.print("Current: ");
    //Serial1.println(nCurrent);

    //Serial1.print("Voltage: ");
    //Serial1.println(nVoltage);
  }
}

// update tft screen with new values
void Task_sendScreen(void *parameters) {
  (void)parameters;
  
  for(;;) {
    tft.setCursor(0, 0);
    tft.println("PEBB voltage: ");
    tft.setCursor(0, 70);
    tft.println("PEBB current: ");
    tft.setCursor(20, 40);
    //tft.print(nVoltage);
    tft.print(amplitude);
    tft.setCursor(20, 100);
    tft.print(nCurrent);
  }
}

void callback(char *topic, byte* message, unsigned int length){
  Serial.println(topic);

  if((String)topic == "/pebb/voltage") {
    flag = false;  
  }

  if((String)topic == "/pebb/current") {
    flag = true;  
  }

  if((String)topic == "/pebb/setVoltage") {
     for(int i = 0; i < length; i++) {
      tempAmp += char(message[i]);
     }
     amplitude = tempAmp;
     tempAmp = "";

     // print twice to prevent 0 values
     delay(10);
     Serial1.print(amplitude);
     delay(10);
     Serial1.print(amplitude);
  }
  
  for(int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    if(!flag) 
      tempV += (char)message[i];  
    else 
      tempC += (char)message[i];
  }

  nVoltage = tempV;
  nCurrent = tempC;

  // use if values write over each other on tft display
  /*if(nVoltage == "0.00") {
    nVoltage = "0.00     ";  
  }

  if(nCurrent == "0.00") {
    nCurrent = "0.00     ";  
  }*/

  // reset temporary fields
  if(flag) 
    tempV = "";
  else 
    tempC = "";
  
  Serial.println(); 
}

void reconnect() {
  while (!client.connected()){
    Serial.print("attmepting mqtt connection");
    if (client.connect(mqtt_server,mqtt_user,mqtt_pass)){
      Serial.println("connected");
      client.subscribe(mqtt_topic_voltage);
      client.subscribe(mqtt_topic_current);
      client.subscribe(mqtt_topic_setVoltage);
    } else {
      Serial.print(client.state());
      Serial.println("try again in 5s");
      delay(5000);
    }
  }
}

void setup_wifi() {
  Serial.println();
  Serial.print("connecting to");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID,WLAN_PASS);
  int a=0;
  while (WiFi.status()!= WL_CONNECTED){
    delay(500);
    a++;
    if(a>3){
     ESP.restart();}
     Serial.print(".");
    }
  Serial.println();
  Serial.println("wifi connected");
  Serial.println("IP ");
  Serial.println(WiFi.localIP());
}

void loop(){
  if(!client.connected()){
    reconnect();
  }

/*if(Serial.available()){
        command = Serial.readStringUntil('\n');
         command.toCharArray(outmessage, 125);
        if(command.equals("voltage")){
            vorc="voltage";
           }

           if(command.equals("current")){
            vorc="current";
           }

        if (vorc.equals("voltage")){ 
         for(int i=0;i<1001;i++){
        if(command.toInt()==i) {
            client.publish(mqtt_topic_setVoltage,outmessage); 
          }
        }
         }
        
        if (vorc.equals("current")){
        for(int i=0;i<501;i++){ 
        if(command.toInt()==i) { 
         client.publish(mqtt_topic_setCurrent,outmessage);
           }
        }
         }
        
        if (command.equals("on")){
          client.publish(mqtt_topic_power,outmessage);
        }
        if (command.equals("off")){
          client.publish(mqtt_topic_power,outmessage);
        }        
        
    }*/

  client.loop();
}

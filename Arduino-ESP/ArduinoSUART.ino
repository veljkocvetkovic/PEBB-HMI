#include <SoftwareSerial.h>

SoftwareSerial mySUART (2, 3); // pin2 = SRX, pin3 = STX

float angle = 0;
float amplitude = 1000;
float sinwave;
float stepSin = 2*(2*PI)/100; 

void setup() {
  mySUART.begin(115200); // serial for communication with ESP-32
  Serial.begin(115200);
}

void loop() {

  // read amplitude from ESP
  if(mySUART.available()) {
    String temp = mySUART.readString();
    if(temp != "\n")
      amplitude = temp.toFloat();
    Serial.println(amplitude); 
  }

  // generate 100 voltages (sinwave)
  if(Serial.available()) {
    if(Serial.read() == 'U'){
      for(int j = 1; j < 4; j++) {
       Serial.print("U");
       Serial.print(j);
       Serial.print(",");
       for(int i = 0; i<100 ;i++) {
        sinwave = amplitude * sin(angle);
        Serial.print(sinwave);
        if(i != 99) {
          Serial.print(",");
        }
        angle = angle + stepSin;  
       }
       Serial.print("\n\r"); 
      }
    } 
  }

  // use for printing values from ESP
  /*if(mySUART.available()) {
    Serial.write(mySUART.read());  
  }*/
}

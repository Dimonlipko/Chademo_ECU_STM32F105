#include <Arduino.h>
#include <MEcan.h>
#include "Chademo.h"
#include "Global.h"
#include "Can_send.h"



// Ініціалізація CAN-каналів
meCAN1 can1;
meCAN2 can2;

#define LED PA8
#define IN0_pin PC2
#define IN1_pin PC1
#define IN2_pin PC0
#define OUT1_pin PB11
#define OUT2_pin PB10


uint32_t timer_send = 0;


void setup() {

  pinMode(LED, OUTPUT);
  Serial.begin(115200);
  delay(100);
  bool success1 = can1.begin(500, PB8, PB9, STD_ID_LEN);
  bool success2 = can2.begin(500, PB5, PB6, STD_ID_LEN); 

  if (success1) {
    Serial.println("CAN1 initialized successfully");
  } else {
    Serial.println("Failed to initialize CAN1");
  }

  if (success2) {
    Serial.println("CAN2 initialized successfully");
  } else {
    Serial.println("Failed to initialize CAN2");
  }
  
}

void loop() {
 
  if(millis() - timer_send >= 500)
  {
    timer_send = millis();
    CAN1AliveStatus();
    digitalToggle(LED);
  }

  readCAN2();
  readCAN1();

  CAN1Answer();

  chademo.loop(millis());

  in0 = digitalRead(IN0_pin);
  in1 = digitalRead(IN1_pin);
  in2 = digitalRead(IN2_pin);
  digitalWrite(OUT1_pin, out1);
  digitalWrite(OUT2_pin, out2);

}


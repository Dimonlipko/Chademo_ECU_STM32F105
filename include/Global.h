#ifndef GLOBAL_H_
#define GLOBAL_H_

#define MAX_CHARGE_V	158
#define MAX_CHARGE_A	130
#define TARGET_CHARGE_V	160
#define MIN_CHARGE_A	20
#define CAPACITY 180
#include <cstdint>

extern bool in1, in2, in0;
extern bool out1, out2;
extern uint8_t Soc;

typedef struct
{
  float ampHours; 
  float kiloWattHours; 
  float packSizeKWH; 
  unsigned short int  maxChargeVoltage; 
  unsigned short int targetChargeVoltage; 

  unsigned char maxChargeAmperage; 
  unsigned char minChargeAmperage; 
  unsigned char capacity; 
} EESettings;

extern EESettings settings;
extern float Voltage;
extern float Current;
extern float Power;
//extern unsigned long CurrentMillis;
extern int Count;

extern unsigned short int errorDoProcessing;
extern unsigned short int errorHandle;
#endif
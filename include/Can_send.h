#pragma once

#include <Arduino.h>
#include <MEcan.h>
#include "Chademo.h"
#include "Global.h"

// Extern declarations for CAN objects and shared variables
extern meCAN1 can1;
extern meCAN2 can2;

//bool can1_send_06[10];

// Function declarations
void reportCAN(int canport, int id, int len, uint8_t data[]);
void readCAN1();
void readCAN2();
void CAN1AliveStatus();
void CAN1Answer();
void Chademo_loop();
void sendChademoBattSpecsFromCAN2();
void sendChademoChargingTimeFromCAN2();
void sendChademoStatusFromCAN2();
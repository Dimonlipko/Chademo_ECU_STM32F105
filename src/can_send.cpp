#include <Arduino.h>
#include <MEcan.h>
#include "Chademo.h"
#include "Global.h"
#include "Can_send.h"


// Ініціалізація CAN-каналів (ці об'єкти мають бути зовнішніми, якщо оголошені в main.cpp)
extern meCAN1 can1;
extern meCAN2 can2;
bool can1_send_06[10];
uint8_t Soc = 0;


void reportCAN(int canport,int id,int len,uint8_t data[])
{
  Serial1.print("CAN port = ");
  Serial1.print(canport);
  Serial1.print(" Rx <id> ");
  Serial1.println(id, HEX);
  for (int i=0 ;i<len;i++)
  {
    Serial1.print(data[i], HEX);Serial1.print(" ");
  }
  Serial1.println();
}

void readCAN1() {
  uint8_t rxdata[8];
  can1.rxMsgLen = can1.receive(can1.id, can1.fltIdx, rxdata);
  if (can1.rxMsgLen < 1) return;

  if(can1.id == 0x423)
  {
    Voltage = rxdata[0] + rxdata[1];
  }

  if(can1.id == 0x423)
  {
    int16_t signedCurrent = (static_cast<int16_t>(rxdata[0]) << 8) | rxdata[1];
    Current = signedCurrent;
    Soc = rxdata[2]; 
  }

  if(can1.id == 0x79b)
  {
    if(rxdata[1] == 0x22 && rxdata[2] == 0x06)
    {
        if(rxdata[3] == 0x01)
        {
            can1_send_06[1] = true;
        }
        if(rxdata[3] == 0x02)
        {
            can1_send_06[2] = true;
        }
        if(rxdata[3] == 0x03)
        {
            can1_send_06[3] = true;
        }
        if(rxdata[3] == 0x04)
        {
            can1_send_06[4] = true;
        }
         if(rxdata[3] == 0x05)
        {
            can1_send_06[5] = true;
        }
    }
  }
}

void readCAN2() {
  uint8_t rxdata[8];
  can2.rxMsgLen = can2.receive(can2.id, can2.fltIdx, rxdata);
  if (can2.rxMsgLen < 1) return;
  //reportCAN(2, can2.id, can2.rxMsgLen, rxdata);

  chademo.handleCANFrame(millis(), can2.id);

  if (can2.id == EVSE_PARAMS_ID)
  {
    chademo.evse_params.supportWeldCheck = rxdata[0];
    chademo.evse_params.availVoltage = rxdata[1] + rxdata[2] * 256;
    chademo.evse_params.availCurrent = rxdata[3];
    chademo.evse_params.thresholdVoltage = rxdata[4] + rxdata[5] * 256;
  }
  
   if (can2.id == EVSE_STATUS_ID)
  {
    chademo.evse_status.presentVoltage = rxdata[1] + 256 * rxdata[2];
    chademo.evse_status.presentCurrent  = rxdata[3];
    chademo.evse_status.status = rxdata[5];
  }

}

void CAN1AliveStatus() {
  uint8_t out[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
  bool result = can1.transmit(0x100, out, 8);
  //Serial.print("CAN1 Transmit = ");
  //Serial.println(result);
}

void CAN1Answer()
{
  if (can1_send_06[1] == true) {
    uint8_t data[8] = {0x07, 0x62, 0x06, 0x01};
    data[4] = highByte(chademo.carStatus.targetCurrent);
    data[5] = lowByte(chademo.carStatus.targetCurrent);
    data[6] = highByte(chademo.carStatus.targetVoltage);
    data[7] = lowByte(chademo.carStatus.targetVoltage);
    can1.transmit(0x7BB, data, 8);
    can1_send_06[1] = false;
}

if (can1_send_06[2] == true) {
    uint8_t data[8] = {0x07, 0x62, 0x06, 0x02};
    data[4] = chademo.askingAmps;
    data[5] = chademo.faults;
    data[6] = chademo.status; 
    data[7] = chademo.chademoState;
    can1.transmit(0x7BB, data, 8);
    can1_send_06[2] = false;
}

if (can1_send_06[3] == true) {
    uint8_t data[8] = {0x07, 0x62, 0x06, 0x03};
    data[4] = highByte(chademo.evse_params.availVoltage);
    data[5] = lowByte(chademo.evse_params.availVoltage);
    data[6] = chademo.evse_params.availCurrent;
    data[7] = chademo.evse_params.supportWeldCheck;
    can1.transmit(0x7BB, data, 8);
    can1_send_06[3] = false;
}

if (can1_send_06[4] == true) {
    uint8_t data[8] = {0x07, 0x62, 0x06, 0x04};
    data[4] = highByte(chademo.evse_params.thresholdVoltage);
    data[5] = lowByte(chademo.evse_params.thresholdVoltage);
    data[6] = highByte(chademo.evse_status.presentVoltage);
    data[7] = lowByte(chademo.evse_status.presentVoltage);
    can1.transmit(0x7BB, data, 8);
    can1_send_06[4] = false;
}

if (can1_send_06[5] == true) {
    uint8_t data[8] = {0x07, 0x62, 0x06, 0x05};
    data[4] = chademo.evse_status.status;
    data[5] = highByte(chademo.evse_status.remainingChargeSeconds);
    data[6] = lowByte(chademo.evse_status.remainingChargeSeconds);
    data[7] = chademo.evse_status.presentCurrent;
    can1.transmit(0x7BB, data, 8);
    can1_send_06[5] = false;
}

if (can1_send_06[6] == true) {
    uint8_t data[8] = {0x07, 0x62, 0x06, 0x06};
    data[4] = in1;
    data[5] = in2;
    data[6] = out1;
    data[7] = out2;
    can1.transmit(0x7BB, data, 8);
    can1_send_06[6] = false;
}

if (can1_send_06[7] == true) {
    uint8_t data[8] = {0x07, 0x62, 0x06, 0x07};
    data[4] = in0;
    data[5] = 0;
    data[6] = 0;
    data[7] = 0;
    can1.transmit(0x7BB, data, 8);
    can1_send_06[7] = false;
}

}

void sendChademoBattSpecsFromCAN2()
{
    uint8_t canMsg[8];
    canMsg[0] = 0x00; // Not Used
    canMsg[1] = 0x00; // Not Used
    canMsg[2] = 0x00; // Not Used
    canMsg[3] = 0x00; // Not Used
    canMsg[4] = lowByte(settings.maxChargeVoltage);
    canMsg[5] = highByte(settings.maxChargeVoltage);
    canMsg[6] = (uint8_t)settings.packSizeKWH;
    canMsg[7] = 0; //not used
    can2.transmit(CARSIDE_BATT_ID, canMsg, 8);
}

void sendChademoChargingTimeFromCAN2()
{
    uint8_t canMsg[8];
    canMsg[0] = 0;
    canMsg[1] = 0xFF;
    canMsg[2] = 90; //ask for how long of a charge? It will be forceably stopped if we hit this time
    canMsg[3] = 60; //how long we think the charge will actually take
    canMsg[4] = 0; //not used
    canMsg[5] = 0; //not used
    canMsg[6] = 0; //not used
    canMsg[7] = 0; //not used
    can2.transmit(CARSIDE_CHARGETIME_ID, canMsg, 8);
}

void sendChademoStatusFromCAN2()
{   
    uint8_t canMsg[8];
	canMsg[0] = 2; //tell EVSE we are talking 1.0 protocol chademo.bChademo10Protocol
	canMsg[1] = lowByte(chademo.carStatus.targetVoltage);
	canMsg[2] = highByte(chademo.carStatus.targetVoltage);
	canMsg[3] = chademo.askingAmps;
	canMsg[4] = chademo.faults;
	canMsg[5] = chademo.status;
	canMsg[6] = (uint8_t)settings.kiloWattHours;
	canMsg[7] = 0; //not used
    can2.transmit(CARSIDE_CONTROL_ID, canMsg, 8);
}
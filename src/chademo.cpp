#include "Chademo.h"
#include "Can_send.h"
#include <Arduino.h>
using namespace std;

CHADEMO chademo;
bool in0, in1, in2;
bool out1, out2;
float Voltage;
float Current;
float Power;
EESettings settings;
int Count;

unsigned short int errorDoProcessing;
unsigned short int errorHandle;

CHADEMO::CHADEMO()
{
  //part of the initialization

  chademo.bStartedCharge = 0;
  chademo.bChademoMode = 0;
  chademo.bChademoSendRequests = 0;
  chademo.bChademoRequest = 0;
  chademo.bChademo10Protocol = 0;
  chademo.askingAmps = 0;
  chademo.bListenEVSEStatus = 0;
  chademo.bDoMismatchChecks = 0;
  chademo.insertionTime = 0;
  chademo.chademoState = STOPPED;
  chademo.stateHolder = STOPPED;
  chademo.carStatus.contactorOpen = 1;
  chademo.carStatus.battOverTemp = 0;

  chademo.ChargeTimeRefSecs = 0;
  chademo.sendBatt = 0;
  chademo.sendStatus = 0;
  chademo.sendTime = 0;
  chademo.faults = 0;
  chademo.status = 0;
  chademo.vMismatchCount = 0;
  chademo.cMismatchCount = 0;
  chademo.vCapCount = 0;
  chademo.vOverFault = 0;
  chademo.faultCount = 0;
  chademo.mismatchStart = 0;
  chademo.mismatchDelay = 10000;
  chademo.stateMilli = 0;
  chademo.stateDelay = 0;
  chademo.lastCommTime = 0;
  chademo.lastCommTimeout = 1000;
}

void CHADEMO::setDelayedState(int newstate, unsigned short int delayTime, unsigned long CurrentMillis)
{
  //will wait delayTime milliseconds and then transition to new state. Sets state to LIMBO in the meantime

  chademo.chademoState = LIMBO;
  chademo.stateHolder = (CHADEMOSTATE)newstate;
  chademo.stateMilli = CurrentMillis;
  chademo.stateDelay = delayTime;
}

CHADEMOSTATE CHADEMO::getState()
{
  return chademo.chademoState;
}

void CHADEMO::setTargetAmperage(unsigned char t_amp)
{
  carStatus.targetCurrent = t_amp;
}

void CHADEMO::setTargetVoltage(unsigned short int t_volt)
{
  carStatus.targetVoltage = t_volt;
}

void CHADEMO::setChargingFault()
{
  carStatus.chargingFault = 1;
}

void CHADEMO::setBattOverTemp()
{
  carStatus.battOverTemp = 1;
}

//stuff that should be frequently run (as fast as possible) 
void CHADEMO::loop(unsigned long CurrentMillis)
{
    static unsigned char frameRotate;

  if (in1 == 1) //IN1 goes HIGH if we have been plugged into the chademo port
  {
    if (chademo.insertionTime == 0)
    {
      //Set the outputs LOW in case they have been set outside of Chademo Mode.
      out1 = 0;
      out2 = 0;

      chademo.insertionTime = CurrentMillis;
    }
    else if (CurrentMillis > (unsigned int)(chademo.insertionTime + 500)) 
    {
      if (chademo.bChademoMode == 0)
      {
        chademo.bChademoMode = 1;
        if (chademo.chademoState == STOPPED && !chademo.bStartedCharge) 
        {
          chademo.chademoState = STARTUP;
          carStatus.battOverTemp = 0;
          carStatus.battOverVolt = 0;
          carStatus.battUnderVolt = 0;
          carStatus.chargingFault = 0;
          carStatus.chargingEnabled = 0;
          carStatus.contactorOpen = 1;
          carStatus.currDeviation = 0;
          carStatus.notParked = 0;
          carStatus.stopRequest = 0;
          carStatus.voltDeviation = 0;
          chademo.bChademo10Protocol = 0;
        }
      }
    }
  }
  else  //IN1 is LOW, so we haven't been plugged into the chademo port
  {
    chademo.insertionTime = 0;

    if (chademo.bChademoMode == 1)
    {
      chademo.bChademoMode = 0;
      chademo.bStartedCharge = 0;
      chademo.chademoState = STOPPED;

      out2 = 0;
      out1 = 0;
    }
  } 

  if (chademo.bChademoMode)
  {
    if (!chademo.bDoMismatchChecks && chademo.chademoState == RUNNING)
    {
      if ((chademo.CurrentMillis - chademo.mismatchStart) >= chademo.mismatchDelay) chademo.bDoMismatchChecks = 1;
    }

    if (chademo.chademoState == LIMBO && (chademo.CurrentMillis - chademo.stateMilli) >= chademo.stateDelay)  //set newState, from function setDelayedState
    {
      chademo.chademoState = chademo.stateHolder;
    }

    //sending messages
    if (chademo.bChademoSendRequests && chademo.bChademoRequest)
    {
      chademo.bChademoRequest = 0;
      frameRotate++;
      frameRotate %= 3;

      switch (frameRotate)
      {
        case 0:  //sending EV status to EVSE
        {
        chademo.sendCANStatus();
        chademo.sendStatus = 1;
        break; }

        case 1:  //sending battery specifications from EV to EVSE
        {
        chademo.sendBatt = 1;
        break; }

        case 2:  //sending charging time from EV to EVSE
        {
        chademo.sendTime = 1;
        break; }
      }
    }

    switch (chademo.chademoState)
    {
      case STARTUP: 
      {
      chademo.bDoMismatchChecks = 0; //reset it for now
      setDelayedState(SEND_INITIAL_PARAMS, 50, CurrentMillis);
      break; }

      case SEND_INITIAL_PARAMS:  
      {
      chademo.bChademoSendRequests = 1; //causes chademo frames to be sent out every 100ms
      setDelayedState(WAIT_FOR_EVSE_PARAMS, 50, CurrentMillis);
      break; }

      case WAIT_FOR_EVSE_PARAMS:
        //for now do nothing while we wait
      break;

      case SET_CHARGE_BEGIN:
      { 
      out1 = 1; //signal that we're ready to charge
      setDelayedState(WAIT_FOR_BEGIN_CONFIRMATION, 50, CurrentMillis);
      break; }

      case WAIT_FOR_BEGIN_CONFIRMATION:
      {
      if (in2) 
      {
          setDelayedState(CLOSE_CONTACTORS, 100, CurrentMillis);
      }
      break; }

      case CLOSE_CONTACTORS:
      {   
      out2 = 1;

      setDelayedState(RUNNING, 50, CurrentMillis);
      chademo.carStatus.contactorOpen = 0;   //its closed now
      chademo.carStatus.chargingEnabled = 1; //charge
      chademo.bStartedCharge = 1;
      chademo.mismatchStart = CurrentMillis;;
      break; }

      case RUNNING:  
        
      break;

      case CEASE_CURRENT:
      {
      chademo.carStatus.targetCurrent = 0;
      chademo.chademoState = WAIT_FOR_ZERO_CURRENT;
      break; }

      case WAIT_FOR_ZERO_CURRENT: 
      {
      if (chademo.evse_status.presentCurrent == 0)
      {
          setDelayedState(OPEN_CONTACTOR, 150, CurrentMillis);
      }
      break; }

      case OPEN_CONTACTOR: 
      {
      out2 = 0;

      chademo.carStatus.contactorOpen = 1;
      chademo.carStatus.chargingEnabled = 0;
      sendCANStatus(); //we probably need to force this right now
      setDelayedState(STOPPED, 100, CurrentMillis);
      break; }

      case FAULTED:  
      {
      chademo.chademoState = CEASE_CURRENT;
      break; }

      case STOPPED:  
      {
      if (chademo.bChademoSendRequests == 1)
      {
          out1 = 0;
          out2 = 0;

          chademo.bChademoSendRequests = 0; //don't need to keep sending anymore.
          chademo.bListenEVSEStatus = 0;    //don't want to pay attention to EVSE status when we're stopped
      }
      break; }
    }
  }
}

//things that are less frequently run - run on a set schedule
void CHADEMO::doProcessing()
{
    unsigned char tempCurrVal;

  if (chademo.chademoState == RUNNING && ((chademo.CurrentMillis - chademo.lastCommTime) >= chademo.lastCommTime))
  {
    //this is BAD news. We can't do the normal cease current procedure because the EVSE seems to be 
    //unresponsive. yes, this isn't ideal - this will open the contactor and send the shutdown signal. 
    //it's better than letting the EVSE potentially run out of control.

      errorDoProcessing = 1;  //flag to see what is the reason of stopping

    chademo.chademoState = OPEN_CONTACTOR;
  }

  if (chademo.chademoState == RUNNING && chademo.bDoMismatchChecks)
  {
    if (Voltage > settings.maxChargeVoltage && !carStatus.battOverVolt)  //battery voltage must be less than maxChargeVoltage!
    {
      chademo.vOverFault++;
      if (chademo.vOverFault > 9)
      {
        carStatus.battOverVolt = 1;
        chademoState = CEASE_CURRENT;

        errorDoProcessing = 2;
      }
    }
    else chademo.vOverFault = 0;

    //Constant Current/Constant Voltage Taper checks. If minimum current is set to zero, we terminate once
    //target voltage is reached. If not zero, we will adjust current up or down as needed to maintain 
    //voltage until current decreases to the minimum entered

    if (Count == 20)
    {
      if (evse_status.presentVoltage > settings.targetChargeVoltage - 1) //All initializations complete and we're running. We've reached charging target
      {
        if (settings.minChargeAmperage == 0 || carStatus.targetCurrent < settings.minChargeAmperage) 
        {
          //put SOC, ampHours and kiloWattHours reset in here once we actually reach the termination point.
          settings.ampHours = 0;                         // Amp hours count up as used
          settings.kiloWattHours = settings.packSizeKWH; // Kilowatt Hours count down as used
          chademo.chademoState = CEASE_CURRENT;          // Terminate charging

          errorDoProcessing = 3;
        } else
          carStatus.targetCurrent--;  //Taper. Actual decrease occurs in sendChademoStatus
      }
      else //Only adjust upward if we have previous adjusted downward and do not exceed max amps
      {
        if (carStatus.targetCurrent < settings.maxChargeAmperage) carStatus.targetCurrent++;
      }
    }
  }
}

void CHADEMO::checkChargingState()
{
    unsigned long CurrentSecs = chademo.CurrentMillis / 1000;

    if (Current < -2.0 && Current > -20 && Voltage >= settings.targetChargeVoltage)
    {
        if (Current > -15 && (CurrentSecs - chademo.ChargeTimeRefSecs) > 30)
        {
            ResetChargeState();
            chademo.ChargeTimeRefSecs = CurrentSecs;
        }
    }
    else
    {
        chademo.ChargeTimeRefSecs = CurrentSecs;
    }
}

void CHADEMO::ResetChargeState()
{
    settings.ampHours = 0.0;
    settings.kiloWattHours = settings.packSizeKWH;
}

void CHADEMO::updateTargetAV()
{
    chademo.setTargetAmperage(settings.maxChargeAmperage);
    chademo.setTargetVoltage(settings.targetChargeVoltage);
}

//handling of received messages
void CHADEMO::handleCANFrame(unsigned long CurrentMillis, unsigned int receiveID)
{
    unsigned char tempCurrVal;
    unsigned char tempAvailCurr; 

    if (receiveID == EVSE_PARAMS_ID)
    {
        chademo.lastCommTime = CurrentMillis;

        if (chademo.chademoState == WAIT_FOR_EVSE_PARAMS)
        {
            setDelayedState(SET_CHARGE_BEGIN, 100, CurrentMillis);
        }
    }

    tempAvailCurr = evse_params.availCurrent > 0 ? evse_params.availCurrent - 1 : 0;

    //if charger cannot provide our requested voltage then:
    if ((evse_params.availVoltage < carStatus.targetVoltage) && (chademo.chademoState <= RUNNING))  //target Voltage must be less than available voltage
    {
      chademo.vCapCount++;
      if (chademo.vCapCount > 9)
      {
        chademo.chademoState = CEASE_CURRENT;

        errorHandle = 1;
      }
    }
    else chademo.vCapCount = 0;

    //if we want more current then it can provide then revise our request to match max output
    if (tempAvailCurr < carStatus.targetCurrent)
    {
        carStatus.targetCurrent = tempAvailCurr;
    }

    //If not in running then also change our target current up to the minimum between the
    //available current reported and the max charge amperage. This should fix an issue where
    //the target current got wacked for some reason and left at zero.
    if (chademo.chademoState != RUNNING && tempAvailCurr > carStatus.targetCurrent)
    {
      carStatus.targetCurrent = min(tempAvailCurr, settings.maxChargeAmperage);
    }

  if (receiveID == EVSE_STATUS_ID)
  {
      chademo.lastCommTime = CurrentMillis;

    if (chademo.chademoState == RUNNING && chademo.bDoMismatchChecks)
    {
        unsigned short int VoltageMinusPresentVoltage = 0;
        unsigned char CurrentMinusPresentCurrent = 0;

        if (Voltage - evse_status.presentVoltage < 0)  //instead of abs(Voltage - evse_status.presentVoltage)
        {
          VoltageMinusPresentVoltage += 2 * (-(Voltage - evse_status.presentVoltage));
          
          if (VoltageMinusPresentVoltage > (evse_status.presentVoltage >> 3) && !carStatus.voltDeviation)  
          {
              chademo.vMismatchCount++;
              if (chademo.vMismatchCount > 4)
              {
                  carStatus.voltDeviation = 1;
                  chademo.chademoState = CEASE_CURRENT;

                  errorHandle = 2;
              }
          }
          else chademo.vMismatchCount = 0;
        }
        else
        {
          if (Voltage - evse_status.presentVoltage > (evse_status.presentVoltage >> 3) && !carStatus.voltDeviation) 
          {
              chademo.vMismatchCount++;
              if (chademo.vMismatchCount > 4)
              {
                  carStatus.voltDeviation = 1;
                  chademo.chademoState = CEASE_CURRENT;

                  errorHandle = 3;
              }
          }
          else chademo.vMismatchCount = 0;
        }

        tempCurrVal = evse_status.presentCurrent >> 3;
        if (tempCurrVal < 3) tempCurrVal = 3;

        if (Current - evse_status.presentCurrent < 0)  //instead of abs(Current - evse_status.presentCurrent)
        {
            CurrentMinusPresentCurrent += 2 * (-(Current  - evse_status.presentCurrent));

          if (CurrentMinusPresentCurrent > tempCurrVal && !carStatus.currDeviation)  
          {
              chademo.cMismatchCount++;
              if (chademo.cMismatchCount > 4)
              {
                  carStatus.currDeviation = 1;
                  chademo.chademoState = CEASE_CURRENT;

                  errorHandle = 4;
              }
          }
          else chademo.cMismatchCount = 0;
        }
        else
        {
          if (Current - evse_status.presentCurrent > tempCurrVal && !carStatus.currDeviation)
          {
              chademo.cMismatchCount++;
              if (chademo.cMismatchCount > 4)
              {
                  carStatus.currDeviation = 1;
                  chademo.chademoState = CEASE_CURRENT;

                  errorHandle = 5;
              }
          }
          else chademo.cMismatchCount = 0;
        }
    }

    //on fault try to turn off current immediately and cease operation
    if ((evse_status.status & 0x1A) != 0) //if bits 1, 3, or 4 are set then we have a problem.
    {
      chademo.faultCount++;
      if (chademo.faultCount > 3)
      {
        if (chademo.chademoState == RUNNING) chademo.chademoState = CEASE_CURRENT;

        errorHandle = 9;
      }
    }
    else chademo.faultCount = 0;

    if (chademo.chademoState == RUNNING)
    {
      if (chademo.bListenEVSEStatus)
      {
        if ((evse_status.status & EVSE_STATUS_STOPPED) != 0)
        {
          chademo.chademoState = CEASE_CURRENT;

          errorHandle = 6;
        }

        //if there is no remaining time then shut down
        if ((evse_status.remainingChargeSeconds == 0) | (evse_status.remainingChargeMinutes == 0))
        {
          chademo.chademoState = CEASE_CURRENT;

          errorHandle = 7;
        }
      }
      else
      {
        //if charger is not reporting being stopped and is reporting remaining time then enable the checks.
          if ((evse_status.status & EVSE_STATUS_STOPPED) == 0 && (evse_status.remainingChargeSeconds > 0 | evse_status.remainingChargeMinutes > 0)) chademo.bListenEVSEStatus = 1;
      }
    }
  }
}

void CHADEMO::sendCANStatus()
{
  if (carStatus.battOverTemp) chademo.faults |= CARSIDE_FAULT_OVERT;
  if (carStatus.battOverVolt) chademo.faults |= CARSIDE_FAULT_OVERV;
  if (carStatus.battUnderVolt) chademo.faults |= CARSIDE_FAULT_UNDERV;
  if (carStatus.currDeviation) chademo.faults |= CARSIDE_FAULT_CURR;
  if (carStatus.voltDeviation) chademo.faults |= CARSIDE_FAULT_VOLTM;

  if (carStatus.chargingEnabled) chademo.status |= CARSIDE_STATUS_CHARGE;
  if (carStatus.notParked) chademo.status |= CARSIDE_STATUS_NOTPARK;
  if (carStatus.chargingFault) chademo.status |= CARSIDE_STATUS_MALFUN;
  if (chademo.bChademo10Protocol)
  {
    if (carStatus.contactorOpen) chademo.status |= CARSIDE_STATUS_CONTOP;
    if (carStatus.stopRequest) chademo.status |= CARSIDE_STATUS_CHSTOP;
  }

  if (chademo.chademoState == RUNNING && chademo.askingAmps < carStatus.targetCurrent)
  {
    if (chademo.askingAmps == 0) chademo.askingAmps = 3;
    int offsetError = chademo.askingAmps - evse_status.presentCurrent;
    if ((offsetError <= 1) || (evse_status.presentCurrent == 0)) chademo.askingAmps++;
  }
  //We're allowed to change requested amps by +/- 20A per second. We send the above frame
  //every 100ms so a single increment means we can ramp up 10A per second. 
  //But, we want to ramp down quickly if there is a problem
  //so do two which gives us -20A per second.
  if (chademo.chademoState != RUNNING && chademo.askingAmps > 0) chademo.askingAmps--;
  if (chademo.askingAmps > carStatus.targetCurrent) chademo.askingAmps--;
  if (chademo.askingAmps > carStatus.targetCurrent) chademo.askingAmps--;

  sendChademoBattSpecsFromCAN2();
  sendChademoChargingTimeFromCAN2();
  sendChademoStatusFromCAN2();


}
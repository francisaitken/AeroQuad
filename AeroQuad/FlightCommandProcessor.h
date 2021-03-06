/*
  AeroQuad v3.0.1 - February 2012
  www.AeroQuad.com
  Copyright (c) 2012 Ted Carancho.  All rights reserved.
  An Open Source Arduino based multicopter.
 
  This program is free software: you can redistribute it and/or modify 
  it under the terms of the GNU General Public License as published by 
  the Free Software Foundation, either version 3 of the License, or 
  (at your option) any later version. 

  This program is distributed in the hope that it will be useful, 
  but WITHOUT ANY WARRANTY; without even the implied warranty of 
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
  GNU General Public License for more details. 

  You should have received a copy of the GNU General Public License 
  along with this program. If not, see <http://www.gnu.org/licenses/>. 
*/

// FlightCommand.pde is responsible for decoding transmitter stick combinations
// for setting up AeroQuad modes such as motor arming and disarming

#ifndef _AQ_FLIGHT_COMMAND_READER_
#define _AQ_FLIGHT_COMMAND_READER_


/**
 * readPilotCommands
 * 
 * This function is responsible to read receiver
 * and process command from the users
 */
void readPilotCommands() {
  
  readReceiver(); 
  if (receiverCommand[THROTTLE] < MINCHECK) {
    zeroIntegralError();
    // Disarm motors (left stick lower left corner)
    if (receiverCommand[ZAXIS] < MINCHECK && motorArmed == ON) {
      commandAllMotors(MINCOMMAND);
      motorArmed = OFF;
            
      #ifdef OSD
        notifyOSD(OSD_CENTER|OSD_WARN, "MOTORS UNARMED");
      #endif
      
      #if defined BattMonitorAutoDescent
        batteryMonitorAlarmCounter = 0;
        batteryMonitorStartThrottle = 0;
        batteyMonitorThrottleCorrection = 0.0;
      #endif
    }    
    
    // Zero Gyro and Accel sensors (left stick lower left, right stick lower right corner)
    if ((receiverCommand[ZAXIS] < MINCHECK) && (receiverCommand[XAXIS] > MAXCHECK) && (receiverCommand[YAXIS] < MINCHECK)) {
      calibrateGyro();
      computeAccelBias();
      storeSensorsZeroToEEPROM();
      calibrateKinematics();
      zeroIntegralError();
      pulseMotors(3);
    }   
    
    // Arm motors (left stick lower right corner)
    if (receiverCommand[ZAXIS] > MAXCHECK && motorArmed == OFF && safetyCheck == ON) {
      #if defined (UseGPS)
        if (!isHomeBaseInitialized()) {  // if GPS, wait for home position fix!
          return;
        }
      #endif 

      #ifdef OSD_SYSTEM_MENU
        if (menuOwnsSticks) {
          return;
        }
      #endif

      zeroIntegralError();
      for (byte motor = 0; motor < LASTMOTOR; motor++) {
        motorCommand[motor] = MINTHROTTLE;
      }
      motorArmed = ON;
    
      #ifdef OSD
        notifyOSD(OSD_CENTER|OSD_WARN, "!MOTORS ARMED!");
      #endif  
      
      
    }
    // Prevents accidental arming of motor output if no transmitter command received
    if (receiverCommand[ZAXIS] > MINCHECK) {
      safetyCheck = ON; 
    }
  }
  
  // Check Mode switch for Acro or Stable
  if (receiverCommand[MODE] > 1500) {
    flightMode = ATTITUDE_FLIGHT_MODE;
    digitalWrite(LED_Yellow, HIGH);
  }
  else {
    flightMode = RATE_FLIGHT_MODE;
    digitalWrite(LED_Yellow, LOW);
  }

  
  #if defined AltitudeHoldBaro || defined AltitudeHoldRangeFinder
    if (receiverCommand[AUX] < 1750) {
      if (altitudeHoldState != ALTPANIC ) {  // check for special condition with manditory override of Altitude hold
        if (isStoreAltitudeNeeded) {
          #if defined AltitudeHoldBaro
            baroAltitudeToHoldTarget = getBaroAltitude();
            PID[BARO_ALTITUDE_HOLD_PID_IDX].integratedError = 0;
            PID[BARO_ALTITUDE_HOLD_PID_IDX].lastPosition = baroAltitudeToHoldTarget;
          #endif
          #if defined AltitudeHoldRangeFinder
            sonarAltitudeToHoldTarget = rangeFinderRange[ALTITUDE_RANGE_FINDER_INDEX];
            PID[SONAR_ALTITUDE_HOLD_PID_IDX].integratedError = 0;
            PID[SONAR_ALTITUDE_HOLD_PID_IDX].lastPosition = sonarAltitudeToHoldTarget;
          #endif
          altitudeHoldThrottle = receiverCommand[THROTTLE];
          isStoreAltitudeNeeded = false;
        }
        altitudeHoldState = ON;
      }
    } 
    else {
      isStoreAltitudeNeeded = true;
      altitudeHoldState = OFF;
    }
  #endif
  
  #if defined (UseGPSNavigator)
    if (receiverCommand[AUX] < 1750) {
      if (positionHoldState != ALTPANIC ) {  // check for special condition with manditory override of Altitude hold
        if (isStorePositionNeeded) {
          positionHoldState = ON;
          gpsRollAxisCorrection = 0;
          gpsPitchAxisCorrection = 0;
          positionToReach.latitude = currentPosition.latitude;
          positionToReach.longitude = currentPosition.longitude;
          previousPosition.latitude = currentPosition.latitude;
          previousPosition.longitude = currentPosition.longitude;
          isStorePositionNeeded = false;
        }
        positionHoldState = ON;
      }
    } 
    else {
      isStorePositionNeeded = true;
      positionHoldState = OFF;
    }
  #endif
}

#endif // _AQ_FLIGHT_COMMAND_READER_



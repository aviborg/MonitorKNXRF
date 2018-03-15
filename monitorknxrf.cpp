#include <stdint.h>
#include <string>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <wiringPi.h>
#include <iostream>
#include "cc1101.h"
#include "sensorKNXRF.h"
#include "openhabRESTInterface.h"

#define GDO0idx 0
#define GDO2idx 1

// globalCounter:
//	Global variable to count interrupts
//	Should be declared volatile to make sure the compiler doesn't cache it.

// static volatile uint8_t globalCounter[2];

// Setting debug level to !0 equal verbose
uint8_t cc1101_freq_select = 3;
uint8_t cc1101_mode_select = 2;
uint8_t cc1101_channel_select = 0;
uint8_t cc1101_debug = 0;

// Global variables
CC1101 cc1101;
SensorKNXRF *sensorBuffer = NULL;

/*
 * Interrupts:
 *********************************************************************************
 */

/* void cc1101InterruptGDO0(void) {
	piLock(GDO0idx);
	++globalCounter[GDO0idx];
	printf("\r\nglobalCounter[GDO0idx]: %02d\r\n",globalCounter[GDO0idx]);
	piUnlock(GDO0idx); 
} */
void cc1101InterruptGDO2(void) {
	uint8_t dataBuffer[CC1101_BUFFER_LEN] ={0xFF}, dataLen;
	piLock(GDO2idx);
	cc1101.rx_payload_burst(dataBuffer, dataLen);
	saveSensorData(dataBuffer, dataLen, sensorBuffer);
	piUnlock(GDO2idx);
} 

/*
 *********************************************************************************
 * main
 *********************************************************************************
 */

int main (int argc, char* argv[])
{
	std::string s;
	OpenhabItem *itemList = NULL;
	uint32_t timeStart, timeout;	
	uint8_t addrCC1101 = 0;
	if (argc > 1) {
		timeout = atoi(argv[1])*1000; // runtime in seconds
	} else {
		timeout = 60000;
	}
	if (argc > 2) {
		cc1101_debug = atoi(argv[2]);
	} else {
		cc1101_debug = 0;
	}
	
	
	cc1101.begin(addrCC1101);			//setup cc1101 RF IC
	
	wiringPiSetup();			//setup wiringPi library
/* 	wiringPiISR (GDO0, INT_EDGE_RISING, &cc1101InterruptGDO0) ; */
	wiringPiISR (GDO2, INT_EDGE_RISING, &cc1101InterruptGDO2) ;



	//cc1101.show_register_settings();
	
	timeStart = millis();
	try {
		while (millis() - timeStart < timeout) { 
			delay(15000);
			parseOpenhabItems(getOpenhabItems("RoomThermostat"), itemList);
			while (sensorBuffer) {
				piLock(GDO2idx);
				sendSensorData(sensorBuffer, itemList);
				piUnlock(GDO2idx);
				delay(1);
			}
		}
	} catch (const std::exception& e) {
		std::cout << " a standard exception was caught, with message '" << e.what() << "'\n";
    }

	cc1101.end();
	return 0;
}
#ifndef _SENSORKNXRF_H
#define _SENSORKNXRF_H

#include <stdint.h>
#include <string>
#include "openhabRESTInterface.h"

#define MAXSENSORDATA 30

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

/**
 * Class: SensorKNXRF
 * 
 * Description:
 * SensorKNXRF data packet class
 */
class SensorKNXRF
{
	public:
		SensorKNXRF(void);
	    uint32_t timeStamp;
		uint32_t lastDataAge;
		uint16_t serialNoHighWord;
		uint32_t serialNoLowWord;
		uint8_t uniDirectional;
		uint8_t batteryOK;
		uint16_t sourceAddress;
		uint16_t destAddress;
		uint8_t groupAdress;
		uint8_t maxCounter;
		uint8_t frameNo;
		uint8_t addExtType;
		uint8_t TPCI;   // Transport Layer service indication
		uint8_t seqNumber;
		uint16_t APCI;  // Application Layer service indication
		uint16_t sensorData[MAXSENSORDATA];
		int8_t rssi;
		uint8_t crcOK;
		uint8_t lqi;
		SensorKNXRF *next;
		uint32_t getSize(void);
};

uint8_t manchesterDecode(uint8_t *dataSource, uint8_t *dataDest, int32_t len);

uint8_t saveSensorData(uint8_t* data, uint32_t len, SensorKNXRF *&sensorList);

uint16_t transformTemperature(uint16_t data);

void sendSensorData(SensorKNXRF *&currentSensor, OpenhabItem *itemList);

#endif

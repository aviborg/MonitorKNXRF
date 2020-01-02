#include <iostream>
#include <stdio.h>
#include <vector>
#include <syslog.h>
#include "sensorKNXRF.h"
#include "cc1101.h"
#include "Crc16.h"


extern uint8_t cc1101_debug;

SensorKNXRF::SensorKNXRF(void) {
	timeStamp = 0;
	lastDataAge = 0;
	serialNoHighWord = 0;
	serialNoLowWord = 0;
	uniDirectional = 0;
	batteryOK = 0;
	sourceAddress = 0;
	destAddress = 0;
	groupAdress = 0;
	maxCounter = 0;
	frameNo = 0;
	addExtType = 0;
	TPCI = 0;   // Transport Layer service indication
	seqNumber = 0;
	APCI = 0;  // Application Layer service indication
	for (uint32_t n = 0;n < MAXSENSORDATA; ++n) sensorData[n] = 0xFFFF;
	rssi = 0;
	crcOK = 0;
	lqi = 0;
	next = NULL;
}
	
uint32_t SensorKNXRF::getSize(void) {
	if (this->next) {
		return (1U+this->next->getSize());
	} else {
		return 1U;
	}
}	

//------------------------[saveSensorData]--------------------------------------
uint8_t saveSensorData(uint8_t* dataBuffer, uint32_t len, SensorKNXRF *&sensorList)
{
	uint8_t updatedData = false;
	uint8_t crcError = true, crcFailIdx = 0;
	uint32_t crcValue, serialNoLowWord = 0;
	uint8_t crcFailed, startIdx, blockIdx;
	uint16_t packetLength = 0, serialNoHighWord = 0;
	uint8_t xBuffer[CC1101_DATA_LEN] = {0};
	Crc16 crc;
	SensorKNXRF *currentSensor, *previousSensor = NULL;
	currentSensor = sensorList;
	
	if(cc1101_debug > 1) {
		syslog(LOG_INFO, "MonitorKNXRF: Saving sensor of size %d\r\n", len);
	}
	
	manchesterDecode(dataBuffer, xBuffer, 6);
	if ((xBuffer[1]==0x44) && (xBuffer[2]==0xFF)) {  // Check for KNX-RF
		packetLength = (14+((xBuffer[0]-9)%16)+(((xBuffer[0]-9)/16)*18))*2;  //Total data length including CRC
		manchesterDecode(dataBuffer+6*sizeof(dataBuffer[0]), xBuffer+3*sizeof(xBuffer[0]), packetLength-6); // readout the remaining data
		packetLength = packetLength>>1; // Divide by two 
	}

	// Calulate the CRC on the data
    if (packetLength > 2) {
      crcError = false;
      startIdx = 0; 
      blockIdx = 12;
      crcFailIdx = 0;
      crcFailed = 0;
      while ((startIdx<packetLength)) {          
          crcValue = xBuffer[min(packetLength-2,startIdx+blockIdx-2)];
          crcValue = (crcValue<<8)+xBuffer[min(packetLength-1,startIdx+blockIdx-1)];
          if (((crc.fastCrc(xBuffer,startIdx,min(packetLength-startIdx-2,blockIdx-2),false,false,0x3D65,0,0,0x8000,0)^0xFFFF)&0xFFFF)!=crcValue) {
            crcError = true;
            crcFailed = crcFailed | (1<<crcFailIdx);
          }
          startIdx +=blockIdx;         
          blockIdx = 18;
          ++crcFailIdx;
      }
    }

	// Save the data to the sensor class array
    if (!crcError) {
        serialNoHighWord    = xBuffer[4];
        serialNoHighWord    = (serialNoHighWord<<8) + xBuffer[5];
        serialNoLowWord     = xBuffer[6];
        serialNoLowWord     = (serialNoLowWord<<8) + xBuffer[7];
        serialNoLowWord     = (serialNoLowWord<<8) + xBuffer[8];
        serialNoLowWord     = (serialNoLowWord<<8) + xBuffer[9];
		while (!updatedData) {
			if (!currentSensor) {
				if(cc1101_debug > 1) {
					syslog(LOG_INFO, "MonitorKNXRF: Allocating space for: %04X%08X \r\n", serialNoHighWord, serialNoLowWord);
				}
				try {
					currentSensor = new SensorKNXRF;
				} catch (const std::exception& e) {
					syslog(LOG_ERR, "MonitorKNXRF: A standard exception was caught, with message %s\r\n" , e.what());
					throw;
				}
				if (previousSensor) {
					previousSensor->next = currentSensor;
				}
				currentSensor->serialNoHighWord = serialNoHighWord;
				currentSensor->serialNoLowWord = serialNoLowWord;				
				if (!sensorList) { // if the list was empty from the beginning, point to the first element
					sensorList = currentSensor;
				}
				if(cc1101_debug > 1) {
					syslog(LOG_INFO, "MonitorKNXRF: Added new sensor succesfully: %04X%08X \r\n", serialNoHighWord, serialNoLowWord);
				}
			}
			if (currentSensor->serialNoHighWord == serialNoHighWord && currentSensor->serialNoLowWord == serialNoLowWord) {
				currentSensor->uniDirectional       = xBuffer[3]&0x1;
                currentSensor->batteryOK            = (xBuffer[3]&0x2)>>1;
                currentSensor->sourceAddress        = xBuffer[13];
                currentSensor->sourceAddress        = (currentSensor->sourceAddress<<8) + xBuffer[14];
                currentSensor->destAddress   		= xBuffer[15];
                currentSensor->destAddress   		= (currentSensor->destAddress<<8) + xBuffer[16];
                currentSensor->groupAdress          = (xBuffer[17]&0x80)>>7;
                currentSensor->maxCounter           = (xBuffer[17]&0x70)>>4;
                currentSensor->frameNo              = (xBuffer[17]&0xE)>>1;
                currentSensor->addExtType 			= xBuffer[17]&0x1;
                currentSensor->TPCI                 = (xBuffer[18]&0xC0)>>6;
                currentSensor->seqNumber       		= (xBuffer[18]&0x3C)>>2;
                currentSensor->APCI                 = xBuffer[18]&0x3;
                currentSensor->APCI                 = (currentSensor->APCI<<8) + xBuffer[19];
                blockIdx = min(MAXSENSORDATA-1,xBuffer[16]);
                currentSensor->sensorData[blockIdx] = xBuffer[20];
                if (xBuffer[0]+2 >= 21) {
                  currentSensor->sensorData[blockIdx] = (currentSensor->sensorData[blockIdx]<<8) + xBuffer[21];
                }
                if (dataBuffer[CC1101_DATA_LEN] >= 128) {  // Conversion from unsigned to signed
                  currentSensor->rssi = dataBuffer[CC1101_DATA_LEN] - 256; 
                } else {
                  currentSensor->rssi = dataBuffer[CC1101_DATA_LEN];
                }
                currentSensor->rssi = currentSensor->rssi/2;
                currentSensor->crcOK                = (dataBuffer[CC1101_DATA_LEN+1]&0x80)>>7;
                currentSensor->lqi = (dataBuffer[CC1101_DATA_LEN+1]&0x7F);
                updatedData = true;
				if(cc1101_debug > 1){                                    //debug output messages
						syslog(LOG_INFO, "MonitorKNXRF: Updated sensor: %04X%08X \r\n", serialNoHighWord, serialNoLowWord);
				}
			} else {
				previousSensor = currentSensor;
				currentSensor = currentSensor->next;
			}
			
		}
	}
	
	return updatedData;
}

uint16_t transformTemperature(uint16_t data) {
	if (data&0x800) data = (data&0x7FF)*2;
	return data;
}



// Print first element in sensor data, and reference to the next element
void sendSensorData(SensorKNXRF *&currentSensor, OpenhabItem *itemList) {
	if (currentSensor) {
		SensorKNXRF *tempSensor;
		uint8_t validData;
		char charBuffer[16] = {0};
		std::string itemPrefix,itemName, itemState, itemNo;
		std::vector <std::string> itemType;
		
		itemPrefix.assign("roomThermostat");
		
		itemType.push_back("Battery");
		itemType.push_back("ActTemp");
		itemType.push_back("SetTemp");
		itemType.push_back("RSSI");
		tempSensor = currentSensor;
		
		sprintf(charBuffer,"%04X%08X", currentSensor->serialNoHighWord, currentSensor->serialNoLowWord);
		itemNo.assign(charBuffer);
		
		while (itemList) {
			for (uint16_t i = 0; i < itemType.size(); ++i) {
				if (itemList->name.compare(itemPrefix + itemType[i] + itemNo) == 0) {
					switch(i) {
						case 0 : 
							sprintf(charBuffer,"%d", currentSensor->batteryOK);
							validData = 1;
							break;    
						case 1 : 
							if (currentSensor->sensorData[1] != 0xFFFF) {
								sprintf(charBuffer,"%d", transformTemperature(currentSensor->sensorData[1]));
								validData = 1;
							} else {
								validData = 0;
							}
							break;   
						case 2 : 
							if (currentSensor->sensorData[2] != 0xFFFF) {
								sprintf(charBuffer,"%d", transformTemperature(currentSensor->sensorData[2]));
								validData = 1;
							} else {
								validData = 0;
							}
							break;    
						case 3 : 
							sprintf(charBuffer,"%d", currentSensor->rssi);
							validData = 1;
							break;
						default :
							validData = 0;
							break;
					}
					if (validData) {
						itemState.assign(charBuffer);
						putOpenhabItem(itemPrefix + itemType[i] + itemNo, itemState);
					}
				}
			}
			itemList = itemList->next;
		}
		// Delete this sensor and point to next
		tempSensor = currentSensor;
		currentSensor = currentSensor->next;
		delete tempSensor;
	}
}

//------------------------[Manchester decode]-----------------------------------
uint8_t manchesterDecode(uint8_t *dataSource, uint8_t *dataDest, int32_t lenSource)
{
	uint8_t res = 1;
	if (lenSource > 1) {
		uint8_t testNibble = 0;

		if (lenSource%2 > 0) {
			if(cc1101_debug > 0){                                   	//debut output
				syslog(LOG_ERR, "MonitorKNXRF: Bad length to manchester decode (must be even): %d\r\n", lenSource);
			}
			lenSource -= 1;  //Skip last byte to make it possible to manchester decode
		}
		for (uint32_t n = 0; n < static_cast<uint32_t>(lenSource); ++n) {
			for (uint8_t m = 0; m < 2; ++m) {
				testNibble = testNibble << 2;
				switch ((dataSource[n]>>(4*(1-m))) & 0xF) {
					case 0x5 :
						testNibble = testNibble | 3;				
						break;
					case 0x6 :
						testNibble = testNibble | 2; 
						break;
					case 0x9 :
						testNibble = testNibble | 1;
						break;
					case 0xA :
						// testNibble = testNibble | 0; doesn't makes sense, only the break is needed					
						break;
					default :
						if (res == 0) lenSource = 0;  //break for loop if more than one violation is found
						res = 0;
						if(cc1101_debug > 1){                                    //debug output messages
							syslog(LOG_INFO, "MonitorKNXRF: Manchester violation in byte %d, nibble %d \r\n", n, m);
						}
				}			
			}
			if (n%2 > 0) {
				dataDest[(n-1)/2] = testNibble;
				testNibble = 0;
			}
		}
	} else { 
		if(cc1101_debug > 0){                                   	//debut output
			syslog(LOG_ERR, "MonitorKNXRF: Bad length to manchester decode (must be greater than 1): %d\r\n", lenSource);
		}
		res = 0;
	}
	return res;
}
//-------------------------------[end]------------------------------------------
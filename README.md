# MonitorKNXRF

## General

This program is developed to be used on Raspberry Pi 3 together with a CC1101 radiomodule.

The program will listen to interrupts generated from the CC1101. When an interrupt is recieved the data will be stored to a variable length buffer of class SensorKNXRF.
Every 15th second the program will request items from OpenHab named RoomThermostatYYYXXX (these have to be created as items in the OpenHab config), YYY is the data type and XXX will be the serial identifier for each unique thermostat. If there is a matching serial identifier in the buffer the OpenHab item will be updated with new values.

## Pin layout

| Raspberry pin # | Name | CC1101 |
| --: | --- | --- |
| 17 | 3.3V              | VCC |
| 18 | GPIO24            | GDO0 |
| 19 | GPIO10 (SPI_MOSI) | SI |
| 20 | Ground            | GND |
| 21 | GPIO09 (SPI_MISO) | SO |
| 22 | GPIO25            | GDO2 |
| 23 | GPIO11 (SPI_CLK)  | SCLK1 |
| 24 | GPI008 (SP_CE0_N) | CSn |

Note: GDO0 is not used in the program. which GDO for interrupts to use is specifed in the configuration of the chip, see cc1101.cpp (look for IOCFG[0..2] and the data sheet for the CC1101.

## Example

Compile the program by running make:
```
openhabian@hab:~/MonitorKNXRF$ make
g++ -c -Wall -I/home/openhabian/jsmn -Iinclude/ monitorknxrf.cpp include/*.cpp
g++  -o monknxrf monitorknxrf.o cc1101.o openhabRESTInterface.o sensorKNXRF.o -L/home/openhabian/jsmn -lcurl -ljsmn -lwiringPi -lsystemd
```

Run the program with debug level 2:
```
./monknxrf 2
```
Change set temperature on any of your thermostats, and wait a minium 15 seconds.
Press Ctrl+C to stop the program.
Then check the log:
```
grep -in monitorknxrf /var/log/syslog
...
24447:Jan  2 12:09:00 hab monknxrf: MonitorKNXRF is requesting data from Openhab.
24448:Jan  2 12:09:00 hab monknxrf: MonitorKNXRF got data from sensor 007402363C12 reading 2232 and 1950.
24451:Jan  2 12:09:00 hab monknxrf: MonitorKNXRF: marcstate: 0x01

```
In this case the actual temperature is 22.32 C and the set temperature is 19.5 C.

To make this thermostat as an item in Openhab just add it as an Openhab item in a items-file:
```
Number roomThermostatBattery007402363C12 "Kontor battery [%f]" [RoomThermostat]
Number roomThermostatActTemp007402363C12 "Kontor actual temp [%f]" [RoomThermostat]
Number roomThermostatSetTemp007402363C12 "Kontor set temp [%f]" [RoomThermostat]
Number roomThermostatRSSI007402363C12 	 "Kontor rssi [%f]" [RoomThermostat]
```

To make the program running as a system service you need to copy the the systemd files as this:
```
sudo cp monknxrf /usr/bin/.
sudo cp monitorknxrf.service /usr/lib/systemd/system/.
```

When the Raspberry is restarted the program should start automatically. It is possible to manually start/stop and check the status of the program:
```
[12:33:15] openhabian@hab:~/MonitorKNXRF$ sudo systemctl start monitorknxrf.service
[12:33:25] openhabian@hab:~/MonitorKNXRF$ sudo systemctl status monitorknxrf.service
● monitorknxrf.service - Service to collect KNX RF data and send to openhab2 via the REST API
   Loaded: loaded (/usr/lib/systemd/system/monitorknxrf.service; enabled)
   Active: active (running) since Thu 2020-01-02 12:33:25 CET; 2s ago
 Main PID: 16235 (monknxrf)
   CGroup: /system.slice/monitorknxrf.service
           └─16235 /usr/bin/monknxrf

Jan 02 12:33:25 hab systemd[1]: Started Service to collect KNX RF data and send to openhab2 via the REST API.
Jan 02 12:33:25 hab monknxrf[16235]: MonitorKNXRF started
```
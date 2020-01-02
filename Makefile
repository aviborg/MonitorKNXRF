# What to call the final executable
TARGET = monknxrf

OBJS= monitorknxrf.o cc1101.o openhabRESTInterface.o sensorKNXRF.o

# What compiler to use
CC = g++

CFLAGS = -c -Wall -I/home/openhabian/jsmn -Iinclude/

LDFLAGS = -L/home/openhabian/jsmn

# We need -lcurl for the curl stuff
LIBS = -lcurl -ljsmn -lwiringPi -lsystemd 

# Link the target with all objects and libraries
$(TARGET) : $(OBJS)
	$(CC)  -o $(TARGET) $(OBJS) $(LDFLAGS) $(LIBS)
		
$(OBJS) : monitorknxrf.cpp
	$(CC) $(CFLAGS) $< include/*.cpp
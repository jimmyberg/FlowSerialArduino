/*
flowSerial.h - Library for register based communication over rs-232 or usb
Registers at both parties wich can be read or writed from.
Is able to ferify the data with a checksum

Copyright 2014
Created by Jimmy van den Berg.
June to August 2014.
*/

#ifndef flowSerial_h
#define flowSerial_h

#include "Arduino.h"

class FlowSerial{
public:
	FlowSerial(long baudrate, uint8_t* iflowReg, size_t regSize);
	char update();
	char inboxBuffer[256];
	char read();
	int available();
	void write(uint8_t address, uint8_t out[], int quantity);
	void write(uint8_t address, uint8_t out);
private:
	uint8_t* flowReg;
	size_t sizeOfFlowReg;
	unsigned char inboxRegisterAt = 0;
	unsigned char inboxAvailable = 0;
	unsigned char argumentBuffer[256];
	unsigned char argumentBufferAt = 0;
	long timeoutTime;
	uint16_t checksumIn;
	uint16_t checksumInbox;
	int instruction;
	void readCommand();
	void writeCommand();
	void recieveData();
	enum FsmStates{
		idle,
		startRecieved,
		instructionRecieved,
		argumentsRecieved,
		LSBchecksumRecieved,
		MSBchecksumRecieved,
		executeInstruction
	}process = idle;

	//instruction codes
	enum instruction{
		readRequest,
		writeInstruction,
		dataReturn
	};
};

#endif

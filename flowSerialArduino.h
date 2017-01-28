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
	FlowSerial(int32_t baudrate, uint8_t* iflowReg, size_t regSize);
	char update();
	char read();
	uint8_t available();
	void sendReadRequest(uint8_t address, uint8_t size);
	void write(uint8_t address, uint8_t out[], uint8_t quantity);
	void write(uint8_t address, uint8_t out);
private:
	uint8_t* const flowReg;
	char inboxBuffer[64];
	const size_t sizeOfInbox = sizeof(inboxBuffer) / sizeof(inboxBuffer[0]);
	size_t sizeOfFlowReg;
	uint8_t inboxRegisterAt = 0;
	uint8_t inboxAvailable = 0;
	uint8_t argumentBuffer[64];
	uint8_t argumentBufferAt = 0;
	unsigned long timeoutTime;
	uint16_t checksumIn;
	uint16_t checksumInbox;
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
	enum Instruction{
		readRequest,
		writeInstruction,
		dataReturn
	}currentInstruction;
};

#endif

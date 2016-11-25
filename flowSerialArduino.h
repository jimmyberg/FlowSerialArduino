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
	FlowSerial(int baudrate, int regSize, char IDinit);
	char update();
	char ID;
	unsigned char serialReg[256];
	char inboxBuffer[256];
	boolean debugEnable;
	char read();
	int available();
	void write(byte address, byte out[], int quantity);
	void write(byte address, byte out);
private:
	unsigned char inboxRegisterAt;
	unsigned char inboxAvailable;
	unsigned char process;
	unsigned char argumentBuffer[256];
	unsigned char argumentBufferAt;
	long timeoutTime;
	int LSBchecksumIn;
	int MSBchecksumIn;
	int checksumInbox;
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
	};

	//instruction codes
	enum instruction{
		readRequest,
		writeInstruction,
		dataReturn
	};
};

#endif

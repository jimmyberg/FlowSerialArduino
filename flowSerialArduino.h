/*
flowSerial.h - Library for register based communication over rs-232 or USB
Registers at both parties which can be read or write from.
Is able to verify the data with a checksum

Copyright 2014
Created by Jimmy van den Berg.
June to August 2014.
*/

#ifndef flowSerial_h
#define flowSerial_h

#include "Arduino.h"
#include "CircularBuffer/CircularBuffer.hpp"
#include "LinearBuffer/LinearBuffer.h"

/**
 * @brief      Class for flow serial communication.
 *
 * @warning    This class can handle limited sized packet. See
 *             FlowSerail::inputBuffer and FlowSerail::argumentBuffer.
 */
class FlowSerial{
public:
	FlowSerial(int32_t baudrate, uint8_t* iflowReg, size_t regSize);
	char update();
	size_t getReturnedData(uint8_t *data, size_t size){
		return inboxBuffer.get(data, size);
	}
	void clearReturnedData(){
		inboxBuffer.clearAll();
	}
	uint8_t available(){
		return inboxBuffer.getStored();
	}
	void sendReadRequest(uint8_t address, uint8_t size);
	void write(uint8_t address, uint8_t out[], uint8_t quantity);
	void write(uint8_t address, uint8_t out);
	uint8_t* const flowReg;
private:
	/**
	 * Buffer used for returned data. The second template parameter implies the
	 * max bytes that can be returned and so deterrence the max read operation
	 *
	 * @warning    When receiving packets that are to large to store in this
	 *             buffer, then it will simply ignore the last bytes.
	 */
	CircularBuffer<uint8_t, 64> inboxBuffer;
	const size_t sizeOfFlowReg;
	/**
	 * Buffer used when receiving a packet. Here the argument data will be
	 * buffered. The second template parameters implies the max size write
	 * packet it can handle.
	 * @warning    When receiving packets that are to large to store in this
	 *             buffer, then it will simply ignore the last bytes.
	 */
	LinearBuffer<uint8_t, 64> argumentBuffer;
	uint16_t timeoutTime;
	uint16_t checksumIn;    // These two are compared when a transmission packet is received as checksum.
	uint16_t checksumInbox; // These two are compared when a transmission packet is received as checksum.
	void readCommand();
	void writeCommand();
	void receiveData();
	enum FsmStates{
		idle,
		startReceived,
		instructionReceived,
		argumentsReceived,
		LSBchecksumReceived,
		MSBchecksumReceived,
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

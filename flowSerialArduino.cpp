
#include "Arduino.h"
#include "flowSerialArduino.h"

//#define _DEBUG_FLOWSERIALARDUINO_

FlowSerial::FlowSerial(long baudrate, uint8_t* iflowReg, size_t regSize):
	flowReg(iflowReg),
	sizeOfFlowReg(regSize)
{
	Serial.begin(baudrate);
	Serial.flush(); 
}

char FlowSerial::update(){
	while(Serial.available() != 0){
		uint8_t byteIn = Serial.read();
		#ifdef _DEBUG_FLOWSERIALARDUINO_
		Serial.print("Received ");
		Serial.println(byteIn);
		#endif
		switch(process){
			case idle:
				if(byteIn == 0xAA){
					checksumInbox = 0xAA;
					timeoutTime = millis();
					#ifdef _DEBUG_FLOWSERIALARDUINO_
					Serial.println("start-byte received");
					#endif
					process = startReceived;
				}
				break;
			case startReceived:
				currentInstruction = static_cast<Instruction>(byteIn);
				checksumInbox += byteIn;
				argumentBuffer.clearAll();
				process = instructionReceived;
				#ifdef _DEBUG_FLOWSERIALARDUINO_
				Serial.println("instruction received");
				#endif
				break;
			case instructionReceived:
				checksumInbox += byteIn;
				argumentBuffer.set(&byteIn);
				switch(currentInstruction){
					case readRequest:
						if(argumentBuffer.getStored() >= 2){
							process = argumentsReceived;
							#ifdef _DEBUG_FLOWSERIALARDUINO_
							Serial.println("arguments received");
							#endif
						}
						break;
					case writeInstruction:
						if(argumentBuffer.getStored() >= 2 && argumentBuffer.getStored() >= argumentBuffer[1] + 2){
							process = argumentsReceived;
							#ifdef _DEBUG_FLOWSERIALARDUINO_
							Serial.println("arguments received");
							#endif
						}
						break;
					case dataReturn:
						if(argumentBuffer.getStored() >= 1 && argumentBuffer.getStored() >= argumentBuffer[0] + 1){
							process = argumentsReceived;
							#ifdef _DEBUG_FLOWSERIALARDUINO_
							Serial.println("arguments received");
							#endif
						}
						break;
				}
				break;
			case argumentsReceived:
				checksumIn = byteIn & 0xFF;
				process = MSBchecksumReceived;
				#ifdef _DEBUG_FLOWSERIALARDUINO_
				Serial.print("LSB received = ");
				Serial.println(byteIn);
				#endif
				break;
			case MSBchecksumReceived:
				process = idle;
				checksumIn |= byteIn << 8;
				#ifdef _DEBUG_FLOWSERIALARDUINO_
				Serial.print("MSB received = ");
				Serial.println(byteIn);
				#endif
				if(checksumIn != checksumInbox){
					#ifdef _DEBUG_FLOWSERIALARDUINO_
					Serial.print("checksum failed. received = ");
					Serial.println(checksumIn);
					Serial.print("counted                   = ");
					Serial.println(checksumInbox);
					#endif
					return -1;
				}
				#ifdef _DEBUG_FLOWSERIALARDUINO_
				else{
					Serial.println("Checksum ok. Execute instruction");
				}
				#endif
				switch(currentInstruction){
					case readRequest:
						readCommand();
						break;
					case writeInstruction:
						writeCommand();
						break;
					case dataReturn:
						receiveData();
						break;
				}
				return 1;
				break;
			default:
				process = idle;
		}
		if(process != idle && (millis() & 0xFFFF) - timeoutTime > 150){
			process = idle;
			return -1;
		}
	}
	return 0;
}

void FlowSerial::readCommand(){
	#ifdef _DEBUG_FLOWSERIALARDUINO_
	Serial.println("Executing read command");
	#endif
	Serial.write(0xAA);
	Serial.write(dataReturn);
	Serial.write(argumentBuffer[1]);
	uint16_t serialSum = 0xAA + dataReturn + argumentBuffer[1];
	for(int i = 0;i < argumentBuffer[1]; i++){
		Serial.write(flowReg[i + argumentBuffer[0]]);
		serialSum += flowReg[i + argumentBuffer[0]];
	}
	Serial.write(static_cast<uint8_t>(serialSum & 0xFF));
	Serial.write(static_cast<uint8_t>(serialSum >> 8));
}

void FlowSerial::writeCommand(){
	#ifdef _DEBUG_FLOWSERIALARDUINO_
	Serial.println("Executing write command");
	#endif
	/*
	argumentBuffer[0] the start address
	argumentBuffer[1] number of bytes wanted to write
	*/
	if(argumentBuffer[0] >= sizeOfFlowReg){
		// Something went wrong. Peer wanted to write outside the the array?
		// Make everything 0 effectively doing nothing from now on.
		argumentBuffer[0] = 0;
		argumentBuffer[1] = 0;
	}
	else if(argumentBuffer[0] + argumentBuffer[1] > sizeOfFlowReg){
		// Apparently the peer wanted to write outside the array. Just make the
		// package smaller.
		argumentBuffer[1] = sizeOfFlowReg - argumentBuffer[0];
		#ifdef _DEBUG_FLOWSERIALARDUINO_
		Serial.println("Tried writing outside of buffer");
		#endif
	}
	#ifdef _DEBUG_FLOWSERIALARDUINO_
	Serial.print("Writing from ");
	Serial.print(argumentBuffer[0]);
	Serial.print(" to ");
	Serial.println(argumentBuffer[1] + argumentBuffer[0]);
	#endif

	for(int i = 0;i < argumentBuffer[1]; i++){
		flowReg[i + argumentBuffer[0]] = argumentBuffer[i + 2];
		#ifdef _DEBUG_FLOWSERIALARDUINO_
		Serial.print("flowReg[");
		Serial.print(i + argumentBuffer[0]);
		Serial.print("] = ");
		Serial.println(argumentBuffer[i + 2]);
		#endif
	}
}

void FlowSerial::receiveData(){
	#ifdef _DEBUG_FLOWSERIALARDUINO_
	Serial.println("Executing receiveData command");
	#endif
	inboxBuffer.set(&argumentBuffer[1], argumentBuffer[0]);
}

void FlowSerial::sendReadRequest(uint8_t address, uint8_t size){
	Serial.write(0xAA);
	Serial.write(readRequest);
	Serial.write(address);
	Serial.write(size);
	uint16_t serialSum = 0xAA + readRequest + address + size;
	Serial.write(static_cast<uint8_t>(serialSum & 0xFF));
	Serial.write(static_cast<uint8_t>(serialSum >> 8));
}

void FlowSerial::write(uint8_t address, uint8_t out[], uint8_t quantity){
	Serial.write(0xAA);
	Serial.write(writeInstruction);
	Serial.write(address);
	Serial.write(quantity);
	uint16_t serialSum = 0xAA + writeInstruction + address + quantity;
	for(int i = 0; i < quantity; i++){
		Serial.write(out[i]);
		serialSum += out[i];
	}
	Serial.write(static_cast<uint8_t>(serialSum & 0xFF));
	Serial.write(static_cast<uint8_t>(serialSum >> 8));
}

void FlowSerial::write(uint8_t address, uint8_t out){
	write(address, &out, 1);
}

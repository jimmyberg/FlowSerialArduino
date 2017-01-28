
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
					Serial.println("startbyte recieved");
					#endif
					process = startRecieved;
				}
				break;
			case startRecieved:
				currentInstruction = static_cast<Instruction>(byteIn);
				checksumInbox += byteIn;
				argumentBufferAt = 0;
				process = instructionRecieved;
				#ifdef _DEBUG_FLOWSERIALARDUINO_
				Serial.println("instruction recieved");
				#endif
				break;
			case instructionRecieved:
				checksumInbox += byteIn;
				switch(currentInstruction){
					case readRequest:
						argumentBuffer[argumentBufferAt] = byteIn;
						argumentBufferAt++;
						if(argumentBufferAt >= 2){
							process = argumentsRecieved;
							#ifdef _DEBUG_FLOWSERIALARDUINO_
							Serial.println("arguments recieved");
							#endif
						}
						break;
					case writeInstruction:
						argumentBuffer[argumentBufferAt] = byteIn;
						argumentBufferAt++;
						if(argumentBufferAt >= 2 && argumentBufferAt >= argumentBuffer[1] + 2){
							process = argumentsRecieved;
							#ifdef _DEBUG_FLOWSERIALARDUINO_
							Serial.println("arguments recieved");
							#endif
						}
						break;
					case dataReturn:
						argumentBuffer[argumentBufferAt] = byteIn;
						argumentBufferAt++;
						if(argumentBufferAt >= 1 && argumentBufferAt >= argumentBuffer[0] + 1){
							process = argumentsRecieved;
							#ifdef _DEBUG_FLOWSERIALARDUINO_
							Serial.println("arguments recieved");
							#endif
						}
						break;
				}
				break;
			case argumentsRecieved:
				checksumIn = byteIn & 0xFF;
				process = MSBchecksumRecieved;
				#ifdef _DEBUG_FLOWSERIALARDUINO_
				Serial.print("LSB recieved = ");
				Serial.println(byteIn);
				#endif
				break;
			case MSBchecksumRecieved:
				process = idle;
				checksumIn |= byteIn << 8;
				#ifdef _DEBUG_FLOWSERIALARDUINO_
				Serial.print("MSB recieved = ");
				Serial.println(byteIn);
				#endif
				if(checksumIn != checksumInbox){
					#ifdef _DEBUG_FLOWSERIALARDUINO_
					Serial.print("checksum failed. recieved = ");
					Serial.println(checksumIn);
					Serial.print("counted                   = ");
					Serial.println(checksumInbox);
					#endif
					return -1;
				}
				#ifdef _DEBUG_FLOWSERIALARDUINO_
				else{
					Serial.println("checksum ok. exucute instruction");
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
						recieveData();
						break;
				}
				return 1;
				break;
			default:
				process = idle;
		}
		if(process != idle && millis() - timeoutTime > 150){
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
	argumentBuffer[0] the start ardres
	argumentBuffer[1] number of bytes wanted to write
	*/
	if(argumentBuffer[0] > sizeOfFlowReg){
		//Something went wrong. Peer wanted to write outside the array? Make everything 0 effectifly doing nothing from now on.
		argumentBuffer[0] = 0;
		argumentBuffer[1] = 0;
	}
	else if(argumentBuffer[0] + argumentBuffer[1] > sizeOfFlowReg){
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
	// memory protection
	if(argumentBuffer[0] > sizeOfFlowReg)
		argumentBuffer[0] = sizeOfFlowReg;
	if(argumentBuffer[0] + argumentBuffer[1] > sizeOfFlowReg)
		argumentBuffer[1] = sizeOfFlowReg - argumentBuffer[0];

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

void FlowSerial::recieveData(){
	#ifdef _DEBUG_FLOWSERIALARDUINO_
	Serial.println("Executing recieveData command");
	#endif
	if(argumentBuffer[0] + inboxRegisterAt > sizeOfInbox){
		argumentBuffer[0] = sizeOfInbox - inboxRegisterAt;
	}
	for(int i = 0; i < argumentBuffer[0]; i++){
		inboxBuffer[inboxRegisterAt] = argumentBuffer[i + 1];
		inboxRegisterAt = (inboxRegisterAt + 1) % sizeOfInbox;
		inboxAvailable++;
	}
}

char FlowSerial::read(){
	if(inboxAvailable > 0){
		char charOut = inboxBuffer[(inboxRegisterAt + sizeOfInbox - inboxAvailable) % sizeOfInbox];
		inboxAvailable--;
		return charOut;
	}
	return 0;
}

uint8_t FlowSerial::available(){
	return inboxAvailable;
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


#include "Arduino.h"
#include "flowSerialArduino.h"

FlowSerial::FlowSerial(int baudrate, int regSize, char IDinit){
	Serial.begin(baudrate);
	ID = IDinit;
	debugEnable = false;
	Serial.flush();
	inboxRegisterAt = 0;
	inboxAvailable = 0;
}

char FlowSerial::update(){
	while(Serial.available() != 0){
		unsigned char byteIn = Serial.read();
		switch(process){
			case idle:
				if(byteIn == 0xAA){
					checksumInbox = 0xAA;
					timeoutTime = millis();
					process = startRecieved;
				}
				break;
			case startRecieved:
				instruction = byteIn;
				checksumInbox += byteIn;
				argumentBufferAt = 0;
				process = instructionRecieved;
				if(debugEnable == true){
					Serial.println("instruction recieved");
				}
				break;
			case instructionRecieved:
				switch(instruction){
					case readRequest:
						argumentBuffer[argumentBufferAt] = byteIn;
						argumentBufferAt++;
						if(argumentBufferAt >= 2){
							process = argumentsRecieved;
						}
						break;
					case writeInstruction:
						argumentBuffer[argumentBufferAt] = byteIn;
						argumentBufferAt++;
						if(argumentBufferAt >= 2 && argumentBufferAt >= argumentBuffer[1] + 2){
							process = argumentsRecieved;
						}
						break;
					case dataReturn:
						argumentBuffer[argumentBufferAt] = byteIn;
						argumentBufferAt++;
						if(argumentBufferAt >= 1 && argumentBufferAt >= argumentBuffer[0] + 2){
							process = argumentsRecieved;
						}
						break;
				}
				break;
			case argumentsRecieved:
				MSBchecksumIn = byteIn;
				if(debugEnable == true){
					Serial.println("LSB recieved");
				}
				process = MSBchecksumRecieved;
				break;
			case MSBchecksumRecieved:
				LSBchecksumIn = byteIn;
				if(debugEnable == true){
					Serial.println("MSB recieved");
				}
				process = LSBchecksumRecieved;
			case LSBchecksumRecieved:
				if((MSBchecksumIn << 8) | LSBchecksumIn == checksumInbox){
					process = executeInstruction;
					if(debugEnable == true){
						Serial.println("checksum ok. exucute instruction");
					}
				}
				else{
					process = idle;
					return -1;
				}
			case executeInstruction:
			switch(instruction){
				case readRequest:
					readCommand();
					process = idle;
					break;
				case writeInstruction:
					writeCommand();
					process = idle;
					break;
				default:
					process = idle;
			}
			break;
			default:
			process = idle;
		}
	}
	if(millis() - timeoutTime > 150){
		process = idle;
		return -1;
	}
	return 0;
}

void FlowSerial::readCommand(){
	Serial.write(0xAA);
	Serial.write(readRequest);
	Serial.write(argumentBuffer[1]);
	int checksumOut = 0xAA + readRequest + argumentBuffer[1];
	for(int i = 0;i < argumentBuffer[1]; i++){
		Serial.write(serialReg[i + argumentBuffer[0]]);
		checksumOut += serialReg[i + argumentBuffer[0]];
	}
	Serial.write(char(checksumOut >> 8));
	Serial.write(char(checksumOut));
}

void FlowSerial::writeCommand(){
	for(int i = 0;i < argumentBuffer[1]; i++){
		serialReg[i + argumentBuffer[0]] = argumentBuffer[i + 2];
	}
}

void FlowSerial::recieveData(){
	for(int i = 0; i < argumentBuffer[1]; i++){
		inboxBuffer[i + inboxRegisterAt] = argumentBuffer[i + 2];
		inboxAvailable++;
		inboxRegisterAt++;
	}
}

char FlowSerial::read(){
	char charOut = inboxBuffer[inboxRegisterAt - inboxAvailable];
	inboxAvailable--;
	return charOut;
}

int FlowSerial::available(){
	return inboxAvailable;
}

void FlowSerial::write(byte address, byte out[], int quantity){
	Serial.write(0xAA);
	Serial.write(writeInstruction);
	Serial.write(address);
	Serial.write(quantity);
	int serialSum = 0xAA + writeInstruction + address + quantity;
	for(int i = 0; i < quantity; i++){
		Serial.write(out[i]);
		serialSum += out[i];
	}
	Serial.write(char(serialSum >> 8));
	Serial.write(char(serialSum));
}

void FlowSerial::write(byte address, byte out){
	write(address, &out, 1);
}

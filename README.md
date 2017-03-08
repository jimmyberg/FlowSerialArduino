# FlowSerialArduino
FlowSerial implementation written for Arduino

## What is FlowSerial
FlowSerial is a communation layer that can run over the Serial communication. 
It is a protocole with error checking and so it can be used even with some data loss with minimal unexpected behaviour.

## What can it be used for
The library can be used to share a `uint8_t` or `unsigned char` array with another party on the serial interface. 
If both parties have FlowSerial available and assigned an array then both parties can read and write EASYLY to each others array.
This simplifies communaction a lot. 

## When should I use it
You can use this library if you don't want to bother to make your own protocole to communicate. 

## How to use is
Just make a array and assign it the FlowSerial like this:
```
uint8_t array[100];
FlowSerial flowserial(baudRate, array, 100);
//                                      ^ size of array
```
After that you must call flowSerial.update() periodically. This will handle incomming data from the serial interface.
To read to the other party you use `flowSerial.sendReadRequest(address, numberOfBytesYouWantFromThere)` to ask for the data and just wait for `avialable()` to give a value(number of bytes recieved).
After that use `flowSerial.read()` to get the byte asked for one at a time.

For writing you can just use `flowSerial.write(address, data)` or even better for arrays of data use `flowSerial.write(startAddressToWrite, pointerToArray, numberOfBytes)`

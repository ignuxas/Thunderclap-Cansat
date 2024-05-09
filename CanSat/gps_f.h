#ifndef GPS_F_H
#define GPS_F_H
#include <Arduino.h>
#include <HardwareSerial.h>

void configureUblox(byte *settingsArrayPointer);
void calcChecksum(byte *checksumPayload, byte payloadSize);
void sendUBX(byte *UBXmsg, byte msgLength);
byte getUBX_ACK(byte *msgID);
void printHex(uint8_t *data, uint8_t length);
void setBaud(byte baudSetting);
float extractAltitude(String nmea);

#endif
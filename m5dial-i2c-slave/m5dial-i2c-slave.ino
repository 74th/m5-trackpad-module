#include <Wire.h>

byte byteSlaveADR = 0x24; // 7-bit Slave Address
byte byteADR;
byte byteDAT[32];
byte byteREG[] = {0x00, 0x10, 0x02, 0x30, 0x04, 0x50, 0x06, 0x70, 0x08, 0x90, 0x0a, 0xb0, 0x0c, 0xd0, 0x0e, 0xf0};
bool trig;

void setup()
{
    // Wire.setPins(G13, G15);
    // Wire.begin(byteSlaveADR, G13, G15, 400000);
    Wire.begin(byteSlaveADR);
    Wire.onReceive(receiveEvent);
    Wire.onRequest(sendEvent);
}

void loop()
{
}

void receiveEvent(int numBytes)
{
    if (numBytes > 0)
    {
        byteADR = Wire.read();
        for (int i = 0; i < numBytes - 1; i++)
        {
            byteDAT[i] = Wire.read();
        }
        trig = true;
    }
}

void sendEvent()
{
    int i;

    for (i = byteADR; i < 16; i++)
    {
        Wire.write(byteREG[i]);
    }
}
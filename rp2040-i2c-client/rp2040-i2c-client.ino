#include <Wire.h>

byte byteREG[] = {0x00, 0x10, 0x02, 0x30, 0x04, 0x50, 0x06, 0x70, 0x08, 0x90, 0x0a, 0xb0, 0x0c, 0xd0, 0x0e, 0xf0};

void setup()
{
    Serial.begin(115200);
    Wire.begin();
}

int t = 0;

void loop()
{
    int err = 0;
    Serial.print("#");
    Serial.print(t++);
    Serial.print("\r\n");

    Serial.print("write: ");
    Wire.beginTransmission(0x24);
    err = Wire.getWriteError();
    Serial.print(err);
    Wire.write(byteREG, 4);
    err = Wire.getWriteError();
    Serial.print(err);
    Wire.endTransmission();
    err = Wire.getWriteError();
    Serial.print(err);

    delay(100);
    Serial.print("\r\n");
    Serial.print("read: ");

    Wire.requestFrom(0x24, 16);
    while (Wire.available())
    {
        Serial.print(Wire.read(), HEX);
        Serial.print(" ");
    }

    Serial.print("\r\n");
    delay(1000);
}

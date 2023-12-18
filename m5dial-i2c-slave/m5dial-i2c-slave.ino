#include <M5Dial.h>
#include <Wire.h>

byte byteSlaveADR = 0x24; // 7-bit Slave Address
byte byteADR;
byte byteDAT[32];
byte byteREG[] = {0x00, 0x10, 0x02, 0x30, 0x04, 0x50, 0x06, 0x70, 0x08, 0x90, 0x0a, 0xb0, 0x0c, 0xd0, 0x0e, 0xf0};
bool trig;

void setup()
{
    Serial.begin(115200);
    Wire.begin(byteSlaveADR);
    Wire.onReceive(receiveEvent);
    Wire.onRequest(sendEvent);

    auto cfg = M5.config();
    M5Dial.begin(cfg, true, false);
}

long oldPosition = -999;

int prev_x = -1;
int prev_y = -1;
static m5::touch_state_t prev_state;

int alpha_count = 0;

int t = 0;

void loop()
{
    M5Dial.update();
    auto t = M5Dial.Touch.getDetail();

    if (prev_state != t.state)
    {
        prev_state = t.state;
        static constexpr const char *state_name[16] = {
            "none", "touch", "touch_end", "touch_begin",
            "___", "hold", "hold_end", "hold_begin",
            "___", "flick", "flick_end", "flick_begin",
            "___", "drag", "drag_end", "drag_begin"};
        Serial.println(state_name[t.state]);
        if (t.state == m5::touch_state_t::none)
        {
            M5Dial.Display.fillRect(0, 0, 240, 240, BLACK);
        }
        if (t.state == m5::touch_state_t::touch)
        {
            prev_x = t.x;
            prev_y = t.y;
        }
    }
    if (prev_x != t.x || prev_y != t.y)
    {
        Serial.println("X:" + String(t.x) + " / " + "Y:" + String(t.y));
        prev_x = t.x;
        prev_y = t.y;
        M5Dial.Display.drawCircle(t.x, t.y, 5, RED);
    }

    long newPosition = M5Dial.Encoder.read();
    if (newPosition != oldPosition)
    {
        oldPosition = newPosition;
        Serial.println(newPosition);
    }
}

void receiveEvent(int numBytes)
{
    Serial.printf("read: ");
    if (numBytes > 0)
    {
        for (int i = 0; i < numBytes; i++)
        {
            byteDAT[i] = Wire.read();
            Serial.print(byteDAT[i], HEX);
            Serial.print(" ");
        }
        trig = true;
    }

    Serial.print("\r\n");
}

void sendEvent()
{
    Serial.printf("write: ");
    int i;

    for (i = byteADR; i < 16; i++)
    {
        Serial.print(byteREG[i], HEX);
        Serial.print(" ");
        Wire.write(byteREG[i]);
    }

    Serial.print("\r\n");
}
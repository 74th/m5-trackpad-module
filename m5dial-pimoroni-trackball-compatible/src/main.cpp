#include <Arduino.h>
#include <M5Dial.h>
#include <Wire.h>

#define I2C_SLAVE_ADDRESS 0x0A
#define I2C_BUF_SIZE 5

#define TESTING 0

#define MULTIPLE 2

void receiveEvent(int numBytes);
void sendEvent();

volatile uint8_t i2c_buf[I2C_BUF_SIZE] = {0, 0, 0, 0, 0};

void set_move_size(uint16_t step_dx, uint16_t step_dy)
{
    step_dx *= MULTIPLE;
    step_dy *= MULTIPLE;

    int16_t total_dx = (int16_t)i2c_buf[0] - (int16_t)i2c_buf[1] + (step_dx);
    int16_t total_dy = (int16_t)i2c_buf[2] - (int16_t)i2c_buf[3] + (step_dy);

#if TESTING > 0
    Serial.println("total: " + String(total_dx) + "," + String(total_dy));
#endif

    if (total_dx <= -0xff)
    {
        i2c_buf[0] = 0;
        i2c_buf[1] = 0xff;
    }
    else if (total_dx <= 0)
    {
        i2c_buf[0] = 0;
        i2c_buf[1] = -total_dx;
    }
    else if (total_dx <= 0xff)
    {
        i2c_buf[0] = total_dx;
        i2c_buf[1] = 0;
    }
    else
    {
        i2c_buf[0] = 0xff;
        i2c_buf[1] = 0;
    }

    if (total_dy <= -0xff)
    {
        i2c_buf[2] = 0;
        i2c_buf[3] = 0xff;
    }
    else if (total_dy <= 0)
    {
        i2c_buf[2] = 0;
        i2c_buf[3] = -total_dy;
    }
    else if (total_dy <= 0xff)
    {
        i2c_buf[2] = total_dy;
        i2c_buf[3] = 0;
    }
    else
    {
        i2c_buf[2] = 0xff;
        i2c_buf[3] = 0;
    }
}

void setup()
{
    auto cfg = M5.config();
    M5Dial.begin(cfg, true, false);

#if TESTING > 0
    Serial.begin(115200);
    Serial.write("M5Dial I2C Slave\n");
#else
    Wire.begin(I2C_SLAVE_ADDRESS, G13, G15, 400000);
    Wire.onReceive(receiveEvent);
    Wire.onRequest(sendEvent);
#endif
}

long oldPosition = -999;

int prev_x = -1;
int prev_y = -1;
static m5::touch_state_t prev_state;

int alpha_count = 0;

bool touched = false;
bool first_move = false;

int t = 0;
int loop_num = 0;

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
#if TESTING > 0
        Serial.println(state_name[t.state]);
#endif
        if (t.state == m5::touch_state_t::none)
        {
            M5Dial.Display.fillRect(0, 0, 240, 240, BLACK);
        }
        if (t.state == m5::touch_state_t::touch_begin)
        {
            touched = true;
            first_move = true;
            prev_x = t.x;
            prev_y = t.y;
        }
    }
    if (touched && (prev_x != t.x || prev_y != t.y))
    {
        if (first_move)
        {
            first_move = false;
            prev_x = t.x;
            prev_y = t.y;
        }
        else
        {
            int16_t dx = t.x - prev_x;
            int16_t dy = t.y - prev_y;
            set_move_size(dx, dy);

            prev_x = t.x;
            prev_y = t.y;
            M5Dial.Display.drawCircle(t.x, t.y, 5, RED);
        }
    }

    long newPosition = M5Dial.Encoder.read();
    if (newPosition != oldPosition)
    {
        oldPosition = newPosition;
    }

    if (M5Dial.BtnA.isPressed())
    {
        i2c_buf[4] = 0x80;
    }
    else
    {
        i2c_buf[4] = 0x00;
    }
}

void receiveEvent(int numBytes)
{
    if (numBytes > 0)
    {
        for (int i = 0; i < numBytes; i++)
        {
            uint8_t b = Wire.read();
        }
    }
}

void sendEvent()
{
    int n = 0;
    int i;

    for (i = 0; i < 4; i++)
    {
        if (i2c_buf[i] > 10)
        {
            Wire.write(2);
            i2c_buf[i] -= 2;
        }
        else if (i2c_buf[i] > 0)
        {
            Wire.write(1);
            i2c_buf[i]--;
            n++;
        }
        else
        {
            Wire.write(0);
        }
    }
    Wire.write(i2c_buf[4]);
}
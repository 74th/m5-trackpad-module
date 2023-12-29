#include <M5Dial.h>
#include <Wire.h>

#define I2C_SLAVE_ADDRESS 0x0A

#define I2C_BUF_SIZE 5

volatile uint8_t i2c_buf[I2C_BUF_SIZE];

void clear_i2c_buf()
{
    i2c_buf[0] = 0;
    i2c_buf[1] = 0;
    i2c_buf[2] = 0;
    i2c_buf[3] = 0;
}

void set_move_size(uint16_t step_dx, uint16_t step_dy)
{
    int16_t total_dx = (int16_t)i2c_buf[0] - (int16_t)i2c_buf[1] + (step_dx);
    int16_t total_dy = (int16_t)i2c_buf[2] - (int16_t)i2c_buf[3] + (step_dy);

    Serial.println("total: " + String(total_dx) + "," + String(total_dy));

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
    Serial.begin(115200);
    Wire.begin(I2C_SLAVE_ADDRESS, G13, G15, 400000);
    Wire.onReceive(receiveEvent);
    Wire.onRequest(sendEvent);

    clear_i2c_buf();

    auto cfg = M5.config();
    M5Dial.begin(cfg, true, false);
}

long oldPosition = -999;

int prev_x = -1;
int prev_y = -1;
static m5::touch_state_t prev_state;

int alpha_count = 0;

bool touched = false;
bool first_move = false;

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
            Serial.println("FIRST MOVE");
            prev_x = t.x;
            prev_y = t.y;
        }
        else
        {
            // Serial.println("C:" + String(t.x) + "," + String(t.y) + " / D: " + String(t.x - prev_x) + "," + String(t.y - prev_y) + " P:" + String(prev_x) + "," + String(prev_y));
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
        Serial.println(newPosition);
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

    for (i = 0; i < 5; i++)
    {
        Wire.write(i2c_buf[i]);
        n += i2c_buf[i];
    }
    if (n > 0)
    {
        Serial.printf("write: ");
        for (i = 0; i < 5; i++)
        {
            Serial.print(i2c_buf[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
    }
    clear_i2c_buf();
}
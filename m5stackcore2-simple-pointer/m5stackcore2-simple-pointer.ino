#include <M5Core2.h>
#include <Wire.h>

#define I2C_SLAVE_ADDRESS 0x0B
#define I2C_BUF_SIZE 5

#define TESTING 0

typedef struct
{
    uint8_t click;
    int8_t pointer_x;
    int8_t pointer_y;
    int8_t wheel_h;
    int8_t wheel_v;
} simple_pointer_data_t;

#define LEFT_CLICK 0x01
#define RIGHT_CLICK 0x02
#define MIDDLE_CLICK 0x03

#define CMD_POINTER 0x04

volatile simple_pointer_data_t i2c_buf = {0, 0, 0, 0, 0};

unsigned long latest_i2c_connection_time = 0;
uint8_t latest_i2c_command = CMD_POINTER;

char *message = (char *)malloc(32);
unsigned long show_message_limit = 0;

void set_move_size(int16_t step_dx, int16_t step_dy)
{
    int16_t total_dx = (int16_t)i2c_buf.pointer_x + step_dx;
    int16_t total_dy = (int16_t)i2c_buf.pointer_y + step_dy;

#if TESTING > 0
    Serial.println("total: " + String(total_dx) + "," + String(total_dy) + " step: " + String(step_dx) + "," + String(step_dy));
#endif

    if (total_dx <= -128)
    {
        i2c_buf.pointer_x = -128;
    }
    else if (total_dx > 127)
    {
        i2c_buf.pointer_x = 127;
    }
    else
    {
        i2c_buf.pointer_x = total_dx;
    }

    if (total_dy <= -128)
    {
        i2c_buf.pointer_y = -128;
    }
    else if (total_dy > 127)
    {
        i2c_buf.pointer_y = 127;
    }
    else
    {
        i2c_buf.pointer_y = total_dy;
    }
}

long oldPosition = -999;

int prev_x = -1;
int prev_y = -1;

int alpha_count = 0;

bool prev_touched = false;
bool first_touch = false;
bool first_move = false;

int t = 0;
int loop_num = 0;

void show_message()
{
    unsigned long now = millis();
    if (latest_i2c_connection_time + 1000 < now)
    {
        M5.Lcd.setTextDatum(MC_DATUM);
        M5.Lcd.setTextSize(0.5);
        M5.Lcd.drawString("no I2C Connection", M5.Lcd.width() / 2, M5.Lcd.height() / 2);
        return;
    }

    if (now < show_message_limit)
    {
        M5.Lcd.setTextDatum(MC_DATUM);
        M5.Lcd.setTextSize(0.5);
        M5.Lcd.drawString(message, M5.Lcd.width() / 2, M5.Lcd.height() / 2);
    }
}

void i2c_receive_event(int numBytes)
{
    if (numBytes == 0)
    {
        return;
    }

    uint8_t cmd = Wire.read();
    int i = 1;

    latest_i2c_command = cmd;

    switch (cmd)
    {
    case CMD_POINTER:
        break;
    default:
        sprintf(message, "r: n:%d c:%x", numBytes, cmd);
        show_message_limit = millis() + 1000;
        break;
    }

    for (int i = 1; i < numBytes; i++)
    {
        uint8_t b = Wire.read();
    }
}

void send_pointer()
{
    int n = 0;
    int i;

    uint8_t *raw_buf = (uint8_t *)&i2c_buf;

    Wire.write(raw_buf[0]);
    for (i = 1; i < 5; i++)
    {
        Wire.write(raw_buf[i]);
        raw_buf[i] = 0;
    }
}

void i2c_send_event()
{
    switch (latest_i2c_command)
    {
    case CMD_POINTER:
        send_pointer();
        break;
    default:
        Wire.write(0);
        break;
    }
    latest_i2c_connection_time = millis();
}

void setup()
{

#if TESTING > 0
    Serial.begin(115200);
    Serial.write("M5Dial I2C Slave\n");
#else
    Wire.begin(I2C_SLAVE_ADDRESS, G32, G33, 400000);
    Wire.onReceive(i2c_receive_event);
    Wire.onRequest(i2c_send_event);
#endif

    M5.begin();
    M5.Lcd.fillScreen(BLACK);
    sprintf(message, "");
}

void loop()
{
    M5.update();
    auto touched = M5.Touch.ispressed();

    static constexpr const char *state_name[16] = {
        "none", "touch", "touch_end", "touch_begin",
        "___", "hold", "hold_end", "hold_begin",
        "___", "flick", "flick_end", "flick_begin",
        "___", "drag", "drag_end", "drag_begin"};

    if (prev_touched != touched)
    {
        prev_touched = touched;
#if TESTING > 0
        Serial.println(state_name[t.state]);
#endif
        if (!touched)
        {
            M5.Lcd.fillRect(0, 0, M5.Lcd.width(), M5.Lcd.height(), BLACK);
            first_touch = true;
        }
    }
    if (touched)
    {
        TouchPoint_t p = M5.Touch.getPressPoint();
        if (first_touch)
        {
            first_touch = false;
            touched = true;
            first_move = true;
            prev_x = p.x;
            prev_y = p.y;
        }

        if (prev_x != p.x || prev_y != p.y)
        {
            if (first_move)
            {
                first_move = false;
                prev_x = p.x;
                prev_y = p.y;
            }
            else
            {
                int16_t dx = p.x - prev_x;
                int16_t dy = p.y - prev_y;
                if (-30 < dx && dx < 30 && -30 < dy && dy < 30)
                {
                    M5.Lcd.drawCircle(p.x, p.y, 5, RED);
                    set_move_size(dx, dy);
                    sprintf(message, "d:%d,%d", dx, dy);
                    show_message_limit = millis() + 1000;
                }

                prev_x = p.x;
                prev_y = p.y;
            }
        }
    }

    show_message();
}

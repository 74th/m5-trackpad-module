#include <Arduino.h>
#include <M5Dial.h>
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

volatile simple_pointer_data_t i2c_buf = {0};

unsigned long latest_i2c_connection_time = 0;
uint8_t latest_i2c_command = CMD_POINTER;

char *message = (char *)malloc(32);
unsigned long show_message_limit = 0;
unsigned long latest_message_limit = 0;
unsigned long tap_limit = 0;

void mprintf(const char *__restrict format, ...)
{
    va_list args;
    va_start(args, format);
    vsprintf(message, format, args);
    va_end(args);

    show_message_limit = millis() + 1000;
}

void draw_message()
{
    if (show_message_limit == 0)
    {
        return;
    }

    auto now = millis();

    if (latest_message_limit != show_message_limit)
    {
        M5Dial.Display.fillRect(0, M5Dial.Display.height() / 2 - 10, M5Dial.Display.width(), 20, BLACK);
        M5Dial.Display.setTextDatum(middle_center);
        M5Dial.Display.setTextFont(&fonts::Orbitron_Light_32);
        M5Dial.Display.setTextSize(0.5);
        M5Dial.Display.setTextColor(ORANGE);
        M5Dial.Display.drawString(message, M5Dial.Display.width() / 2, M5Dial.Display.height() / 2);
        latest_message_limit = show_message_limit;
    }
    else if (now > show_message_limit)
    {
        M5Dial.Display.fillRect(0, M5Dial.Display.height() / 2 - 10, M5Dial.Display.width(), 20, BLACK);
        show_message_limit = 0;
    }
}

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

void set_tap()
{
    i2c_buf.click |= LEFT_CLICK;
    tap_limit = millis() + 100;
}

void handle_tap()
{
    if (tap_limit > 0 && tap_limit < millis())
    {
        i2c_buf.click &= ~LEFT_CLICK;
        tap_limit = 0;
    }
}

int t = 0;

void check_latest_i2c_connection()
{
    auto now = millis();
    if (latest_i2c_connection_time + 1000 < now && show_message_limit < now)
    {
        mprintf("no I2C Connection");
#if TESTING > 0
        Serial.printf("no I2C Connection");
#endif
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
        mprintf("r: n:%d c:%x", numBytes, cmd);
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

#define TOUCH_SENSITIVITY_MS 200

bool touched = false;
uint32_t touch_started_at = 0;
bool moving = false;
m5::touch_state_t prev_state = m5::none;
int16_t prev_x = -1;
int16_t prev_y = -1;
bool first_move = false;

void handle_touch()
{
    auto e = M5Dial.Touch.getDetail();

    if (e.state == m5::none)
    {
        return;
    }

    if (prev_state == e.state && e.x == prev_x && e.y == prev_y)
    {
        return;
    }

    unsigned long now = millis();

#if TESTING > 0
    Serial.printf("t:%12d s:%4x x:%4d y:%4d\n", now, e.state, e.x, e.y);
#endif

    if (e.state == m5::touch_begin)
    {
        touched = true;
        moving = false;
        first_move = true;
        touch_started_at = now;
        prev_x = e.x;
        prev_y = e.y;
        M5.Lcd.fillRect(0, 10, M5.Lcd.width(), M5.Lcd.height() - 10, BLACK);
    }
    else if (e.state == m5::touch_end)
    {
        if (!moving && now < touch_started_at + TOUCH_SENSITIVITY_MS)
        {
            set_tap();
            mprintf("tap");
#if TESTING > 0
            Serial.printf("left click\n");
#endif
        }

        touched = false;
        moving = false;
    }
    if (touched && (e.prev_x != e.x || e.prev_y != e.y))
    {
        moving = true;
    }

    if (moving)
    {
        if (first_move)
        {
            first_move = false;
        }
        else
        {
            int16_t dx = e.x - prev_x;
            int16_t dy = e.y - prev_y;
            set_move_size(dx, dy);
            mprintf("move x:%3d y:%3d", dx, dy);
            M5Dial.Display.drawCircle(e.x, e.y, 5, ORANGE);
        }
    }

    prev_state = e.state;
    prev_x = e.x;
    prev_y = e.y;
}

long oldPosition = -999;

void handle_encoder()
{
    long newPosition = M5Dial.Encoder.read();
    if (newPosition != oldPosition)
    {
        int8_t d = newPosition - oldPosition;
        i2c_buf.wheel_v = d;
        oldPosition = newPosition;
        mprintf("encoder d:%d", d);
    }
}

bool prev_btna_pressed = false;

void handle_button()
{
    if (M5Dial.BtnA.isPressed())
    {
        i2c_buf.click |= RIGHT_CLICK;
        if (!prev_btna_pressed)
        {
            mprintf("btn a pressed");
            prev_btna_pressed = true;
        }
    }
    else
    {
        i2c_buf.click &= ~RIGHT_CLICK;
        prev_btna_pressed = false;
    }
}

void setup()
{

#if TESTING > 0
    Serial.begin(115200);
    Serial.write("M5Dial I2C Slave\n");
#endif

    auto cfg = M5.config();
    // cfg.serial_baudrate = 115200;
    M5Dial.begin(cfg, true, false);
    mprintf("");

    Wire.begin(I2C_SLAVE_ADDRESS, G13, G15, 400000);
    Wire.onReceive(i2c_receive_event);
    Wire.onRequest(i2c_send_event);
}

void loop()
{
    M5Dial.update();

    handle_touch();
    handle_encoder();
    handle_button();
    handle_tap();

#if TESTING == 0
    check_latest_i2c_connection();
#endif

    draw_message();
}
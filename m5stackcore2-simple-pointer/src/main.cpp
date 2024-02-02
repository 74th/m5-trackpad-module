#include <Arduino.h>
#include <M5Core2.h>
#include <Wire.h>

#define I2C_SLAVE_ADDRESS 0x0B
#define I2C_BUF_SIZE 5

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
        M5.Lcd.fillRect(0, 0, M5.Lcd.width(), 20, BLACK);
        M5.Lcd.setTextDatum(MC_DATUM);
        M5.Lcd.setTextSize(2);
        M5.Lcd.drawString(message, M5.Lcd.width() / 2, 0);
        latest_message_limit = show_message_limit;
    }
    else if (now > show_message_limit)
    {
        M5.Lcd.fillRect(0, 0, M5.Lcd.width(), 20, BLACK);
        show_message_limit = 0;
    }
}

void set_move_size(int16_t step_dx, int16_t step_dy)
{
    int16_t total_dx = (int16_t)i2c_buf.pointer_x + step_dx;
    int16_t total_dy = (int16_t)i2c_buf.pointer_y + step_dy;

    // Serial.println("total: " + String(total_dx) + "," + String(total_dy) + " step: " + String(step_dx) + "," + String(step_dy));

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

void set_wheel_size(int16_t step_dy)
{
    int16_t total_dy = (int16_t)i2c_buf.wheel_v + step_dy;

    // Serial.println("total: " + String(total_dx) + "," + String(total_dy) + " step: " + String(step_dx) + "," + String(step_dy));

    if (total_dy <= -128)
    {
        i2c_buf.wheel_v = -128;
    }
    else if (total_dy > 127)
    {
        i2c_buf.wheel_v = 127;
    }
    else
    {
        i2c_buf.wheel_v = total_dy;
    }
}

void set_tap(uint8_t click)
{
    i2c_buf.click = click;
    tap_limit = millis() + 100;
}

void check_latest_i2c_connection()
{
    auto now = millis();
    if (latest_i2c_connection_time + 1000 < now && show_message_limit < now)
    {
        mprintf("no I2C Connection");
        Serial.printf("no I2C Connection");
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

bool second_finger_touch = false;

#define TOUCH_SENSITIVITY_MS 200
#define BUTTON_SIZE 80

bool touched = false;
bool second_touched = false;
uint32_t touch_started_at = 0;
bool moving = false;
int16_t prev_x = -1;
int16_t prev_y = -1;
int8_t button_pushing_finger = -1;

void clear_display()
{
    M5.Lcd.fillRect(0, 10, M5.Lcd.width(), M5.Lcd.height() - 10, BLACK);
}

void draw_button()
{
    bool touched = button_pushing_finger != -1;
    M5.Lcd.fillRect(-40, 320 - BUTTON_SIZE, BUTTON_SIZE, BUTTON_SIZE, touched ? DARKGREY : BLACK);
}

void handle_first_touch(Event &e, unsigned long now)
{
    if (e.type == E_TOUCH)
    {
        touched = true;
        touch_started_at = now;
        clear_display();
        draw_button();
    }
    if (e.type == E_MOVE)
    {
        moving = true;
        int16_t dx = e.to.x - prev_x;
        int16_t dy = e.to.y - prev_y;
        if (second_touched)
        {
            set_wheel_size(dy);
            mprintf("wheel w:%3d h:%3d", dx, dy);
            M5.Lcd.drawCircle(e.to.x, e.to.y, 5, YELLOW);
        }
        else
        {
            set_move_size(dx, dy);
            if (button_pushing_finger == -1)
            {
                mprintf("move x:%3d y:%3d", dx, dy);
                M5.Lcd.drawCircle(e.to.x, e.to.y, 5, WHITE);
            }
            else
            {
                mprintf("drag x:%3d y:%3d", dx, dy);
                M5.Lcd.drawCircle(e.to.x, e.to.y, 5, BLUE);
            }
        }
    }

    if (e.type == E_RELEASE)
    {
        if (!moving && button_pushing_finger != -1 && now < touch_started_at + TOUCH_SENSITIVITY_MS)
        {
            if (second_touched)
            {
                set_tap(RIGHT_CLICK);
                mprintf("right click");
                Serial.printf("right click\n");
                show_message_limit = now + 500;
            }
            else
            {
                set_tap(LEFT_CLICK);
                mprintf("left click");
                Serial.printf("left click\n");
                show_message_limit = now + 500;
            }
        }

        touched = false;
        moving = false;
        second_touched = false;
    }

    prev_x = e.to.x;
    prev_y = e.to.y;
}

void handle_second_touch(Event &e, unsigned long now)
{
    if (e.type == E_TOUCH)
    {
        second_touched = true;
    }
    if (e.type == E_RELEASE)
    {
        second_touched = false;
    }
}

bool in_left_bottom_zone(int16_t x, int16_t y)
{
    return x < BUTTON_SIZE - 40 && y > 320 - BUTTON_SIZE;
}

bool handle_left_bottom_zone(Event &e)
{
    if (button_pushing_finger == -1)
    {
        if (e.type != E_TOUCH)
        {
            return false;
        }
        if (!in_left_bottom_zone(e.to.x, e.to.y))
        {
            return false;
        }

        button_pushing_finger = e.finger;
        i2c_buf.click |= LEFT_CLICK;
        mprintf("left pushed");
        draw_button();

        return true;
    }

    if (button_pushing_finger != e.finger)
    {
        return false;
    }

    if (e.type == E_RELEASE)
    {
        button_pushing_finger = -1;
        i2c_buf.click &= ~LEFT_CLICK;
        mprintf("left release");
        draw_button();

        return true;
    }

    return true;
}

void touch_handler(Event &e)
{
    Button *b = e.button;

    unsigned long now = millis();
    Serial.printf("t:%12d f:%d e:%4x x:%4d y:%4d\n", now, e.finger, e.type, e.to.x, e.to.y);

    if (handle_left_bottom_zone(e))
    {
        return;
    }

    if (e.finger == 0 || button_pushing_finger != -1)
    {
        handle_first_touch(e, now);
    }
    else
    {
        handle_second_touch(e, now);
    }
}

void setup()
{

    Wire.begin(I2C_SLAVE_ADDRESS, G32, G33, 400000);
    Wire.onReceive(i2c_receive_event);
    Wire.onRequest(i2c_send_event);

    M5.begin();
    M5.Lcd.setRotation(0);
    clear_display();
    draw_button();
    M5.Buttons.addHandler(touch_handler, E_ALL);
    mprintf("");
    Serial.write("M5Dial I2C Slave\n");
}

void loop()
{
    M5.update();

    check_latest_i2c_connection();
    draw_message();

    if (tap_limit > 0 && tap_limit < millis())
    {
        i2c_buf.click = 0;
        tap_limit = 0;
    }
}

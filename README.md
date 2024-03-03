# Firmware to Use M5Stack as a Trackpad Module

- M5Dial for PIMORONI Trackball I2C Protocol: [./m5dial-pimoroni-trackball-compatible (Arduino)](./m5dial-pimoroni-trackball-compatible)
- M5Dial for Simple Pointer I2C Protocol: [./m5dial-simple-pointer (Arduino/PlatformIO)](./m5dial-simple-pointer/)
- M5StackCore2 for Simple Pointer I2C Protocol: [./m5stackcore2-simple-pointer (Arduino/PlatformIO)](./m5stackcore2-simple-pointer)

## Download

- https://github.com/74th/m5-trackpad-module/releases

## Simple Pointer

Use PortA as I2C Slave.

### Protocol

Send below data.

```c
typedef struct
{
    uint8_t click;
    int8_t pointer_x;
    int8_t pointer_y;
    int8_t wheel_h;
    int8_t wheel_v;
} p74_data_t;
```

### QMK Firmware Sample Driver

```
# rules.mk
POINTING_DEVICE_DRIVER = custom
```

```c
// <keyboard_name>.h
#include "pointing_device.h"

void           pointing_device_driver_init(void);
report_mouse_t pointing_device_driver_get_report(report_mouse_t mouse_report);
uint16_t       pointing_device_driver_get_cpi(void);
void           pointing_device_driver_set_cpi(uint16_t cpi);
```

```c
// <keyboard_name>.c
#include "pointing_device.h"

#define SIMPLE_POINTER_TIMEOUT 100
#define SIMPLE_POINTER_SCALE 5
#define SIMPLE_POINTER_ADDRESS 0x0B
#define SIMPLE_POINTER_REG_POINTER  0x04
#define SIMPLE_POINTER_LEFT_CLICK   1
#define SIMPLE_POINTER_RIGHT_CLICK  1 << 1
#define SIMPLE_POINTER_MIDDLE_CLICK 1 << 2

typedef struct {
    uint8_t click;
    int8_t pointer_x;
    int8_t pointer_y;
    int8_t wheel_h;
    int8_t wheel_v;
} simple_pointer_data_t;

void           pointing_device_driver_init(void);
report_mouse_t pointing_device_driver_get_report(report_mouse_t mouse_report);
uint16_t       pointing_device_driver_get_cpi(void);
void           pointing_device_driver_set_cpi(uint16_t cpi);

void pointing_device_driver_init(void) {
    i2c_init();
}

report_mouse_t pointing_device_driver_get_report(report_mouse_t mouse_report) {
    simple_pointer_data_t data = {0};
    i2c_status_t status = i2c_readReg(SIMPLE_POINTER_ADDRESS << 1, SIMPLE_POINTER_REG_POINTER, (uint8_t*)&data, sizeof(data), SIMPLE_POINTER_TIMEOUT);

    if (status == I2C_STATUS_SUCCESS) {
        mouse_report.buttons = pointing_device_handle_buttons(mouse_report.buttons, data.click & SIMPLE_POINTER_LEFT_CLICK, POINTING_DEVICE_BUTTON1);
        mouse_report.buttons = pointing_device_handle_buttons(mouse_report.buttons, data.click & SIMPLE_POINTER_RIGHT_CLICK, POINTING_DEVICE_BUTTON2);
        mouse_report.buttons = pointing_device_handle_buttons(mouse_report.buttons, data.click & SIMPLE_POINTER_MIDDLE_CLICK, POINTING_DEVICE_BUTTON3);

        mouse_report.x = data.pointer_x * SIMPLE_POINTER_SCALE;
        mouse_report.y = data.pointer_y * SIMPLE_POINTER_SCALE;
        mouse_report.v = data.wheel_v;
        mouse_report.h = data.wheel_h;
    }
    return mouse_report;
}

uint16_t pointing_device_driver_get_cpi(void) {
    return 0;
}

void pointing_device_driver_set_cpi(uint16_t cpi) {}
```

## how to build

Please use PlatformIO.

## License

MIT License

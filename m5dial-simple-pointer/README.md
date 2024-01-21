# M5Dial for Simple Pointer

Use PortA as I2C Slave.

## Protocol

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

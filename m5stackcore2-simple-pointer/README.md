# M5StackCore2 for Simple Pointer

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

## Note

- M5StackCore2 is multi-touch compatible, but we gave up using it because we could not get two-finger operation as expected.
  - 🇯🇵 M5StackCore2 はマルチタッチ対応ですが、二本指での操作は、あまり期待通り取得できなかったため、利用を諦めました。

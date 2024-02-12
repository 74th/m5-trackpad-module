#!/bin/bash
esptool.py --chip esp32s3 merge_bin \
  -o m5stackcore2-simple-pointer-merged.bin \
  --flash_mode dio \
  --flash_freq 40m \
  --flash_size 16MB \
  0x1000 .pio/build/m5stack-core2/bootloader.bin \
  0x8000 .pio/build/m5stack-core2/partitions.bin \
  0xe000 ~/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin \
  0x10000 .pio/build/m5stack-core2/firmware.bin

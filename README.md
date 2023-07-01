# Lautstarkofant

A project for Arduino Nano 33 BLE for a bluetooth volume controller.
Uses a fork of [mbed-ble-hid](https://github.com/SpiritCroc/mbed-ble-hid) that supports media keys.
Note that due to some pairing bug in the latest Arduino firmware, you need to downgrade in the Boardmanager the Arduino Mbed OS Nano Boards to something like 2.1.0 (see https://github.com/tcoppex/mbed-ble-hid/issues/11).

## Deprecation Notice

Due to some long-lasting issues related to the mbed BLE firmware, I rewrote this project with Zephyr-OS as base, to be found [here](https://github.com/SpiritCroc/lautstarkofant-zephyr).  
In particular, it should address following issues:
- No need to downgrade firmware to get it working at all
- Allow for pairing to persist across Lautstarkofant restarts
- Do away with bugs in which after reconnecting the device, no more key presses would be sent until reconnecting again together with a power reset

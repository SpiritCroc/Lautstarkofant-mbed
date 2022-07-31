# Lautstarkofant

A project for Arduino Nano 33 BLE for a bluetooth volume controller.
Uses a fork of [mbed-ble-hid](https://github.com/SpiritCroc/mbed-ble-hid) that supports media keys.
Note that due to some pairing bug in the latest Arduino firmware, you need to downgrade in the Boardmanager the Arduino Mbed OS Nano Boards to something like 2.1.0 (see https://github.com/tcoppex/mbed-ble-hid/issues/11).

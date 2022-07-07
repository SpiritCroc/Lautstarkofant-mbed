/**
 * A volume controlling BT HID device.
 * Some inspiration to get a BT keyboard HID from
 * https://github.com/tcoppex/mbed-ble-hid/blob/master/examples/ble_shining_kb/ble_shining_kb.ino
 */

#include "Nano33BleHID.h"
#include "signal_utils.h"
#include "services/keylayouts.h"

Nano33BleKeyboard keyboard("Lautstarkofant");

// Builtin LED animation delays when disconnect.
static const int kLedBeaconDelayMilliseconds = 1185;
static const int kLedErrorDelayMilliseconds = kLedBeaconDelayMilliseconds / 10;

// Builtin LED intensity when connected.
static const int kLedConnectedIntensity = 30;

// setup() is run once
void setup() {
  // General setup.
  pinMode(LED_BUILTIN, OUTPUT);

  // Initialize both BLE and the HID.
  keyboard.initialize();

  // Launch the event queue that will manage both BLE events and the loop.
  // After this call the main thread will be halted.
  MbedBleHID_RunEventThread();
}

// loop() is run repeatedly
void loop() {
  // TODO only for testing purposes send something every second
  // https://forum.arduino.cc/t/arduino-perform-task-only-once-every-second/240495/6
  static const unsigned long REFRESH_INTERVAL = 100; // cycles
  static unsigned long lastRefreshTime = 0;
  static bool up = false;
  if (lastRefreshTime < REFRESH_INTERVAL) {
    lastRefreshTime++;
    return;
  }
  lastRefreshTime = 0;
  // When disconnected, we animate the builtin LED to indicate the device state.
  if (!keyboard.connected()) {
    animateLED(LED_BUILTIN, (keyboard.has_error()) ? kLedErrorDelayMilliseconds
                                                   : kLedBeaconDelayMilliseconds);
    return;
  }
  analogWrite(LED_BUILTIN, kLedConnectedIntensity);

  auto *kb = keyboard.hid();
  if (up) {
    kb->media_keydown(MEDIA_KEY_VOLUME_UP);
  } else {
    kb->media_keydown(MEDIA_KEY_VOLUME_DOWN);
  }
  kb->SendReport();
  kb->keyup();
  kb->SendReport();
  up = !up;
}

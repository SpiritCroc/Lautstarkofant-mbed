/**
 * A volume controlling BT HID device.
 * Some inspiration to get a BT keyboard HID from
 * https://github.com/tcoppex/mbed-ble-hid/blob/master/examples/ble_shining_kb/ble_shining_kb.ino
 */

#include "Nano33BleHID.h"
#include "signal_utils.h"
/*
HIDConsumerControlService(BLE &_ble) :
  HIDService(_ble,
             HID_KEYBOARD,

             // report map
             hid_report_map,
             sizeof(hid_report_map) / sizeof(*hid_report_map),

             // input report
             (uint8_t*)&hid_input_report,
             sizeof(hid_input_report),
             input_report_ref_descs,

             // output report
             (uint8_t*)&hid_output_report,
             sizeof(hid_output_report),
             output_report_ref_descs,
             sizeof(output_report_ref_descs) / sizeof(*output_report_ref_descs))
{}

Nano33BleHID<HIDConsumerControlService>;
*/
Nano33BleKeyboard keyboard("Lautstarkofant");

// Builtin LED animation delays when disconnect.
static const int kLedBeaconDelayMilliseconds = 1185;
static const int kLedErrorDelayMilliseconds = kLedBeaconDelayMilliseconds / 10;

// Builtin LED intensity when connected.
static const int kLedConnectedIntensity = 30;

// Buttons
static KeySym_t vol_down = KeySym_t(114);
static KeySym_t vol_up = KeySym_t(115);

// setup() is run once
void setup() {
  // Overwrite SHIFT_MASK stuff from
  // https://github.com/tcoppex/mbed-ble-hid/blob/39dd7cfcbad03845135396e434ad3ac79b334f88/src/services/HIDKeyboardService.cpp#L203-L219
  vol_down.usage = 114;
  vol_down.modifiers = 0;
  vol_up.usage = 115;
  vol_up.modifiers = 0;

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
  static const unsigned long REFRESH_INTERVAL = 1000; // ms
  static unsigned long lastRefreshTime = 0;
  static bool up = false;
  if (millis() - lastRefreshTime < REFRESH_INTERVAL) {
    return;
  }
  // When disconnected, we animate the builtin LED to indicate the device state.
  if (!keyboard.connected()) {
    animateLED(LED_BUILTIN, (keyboard.has_error()) ? kLedErrorDelayMilliseconds
                                                   : kLedBeaconDelayMilliseconds);
    return;
  }
  // When connected, we slightly dim the builtin LED.
  analogWrite(LED_BUILTIN, kLedConnectedIntensity);

  auto *kb = keyboard.hid();
  if (up) {
    kb->keydown(vol_up);
  } else {
    kb->keydown(vol_down);
  }
  kb->SendReport();
  kb->keyup();
  kb->SendReport();
  up = !up;
}

/**
 * A volume controlling BT HID device.
 * Some inspiration to get a BT keyboard HID from
 * https://github.com/tcoppex/mbed-ble-hid/blob/master/examples/ble_shining_kb/ble_shining_kb.ino
 */

#include "Nano33BleHID.h"
#include "signal_utils.h"
#include "services/keylayouts.h"

// LEDs
#define LED_STATUS 9
#define LED_ACTION 10
#define LED_STATUS_BRIGHTNESS 255
#define LED_ACTION_BRIGHTNESS 127
// BUTTONs
#define BUTTON_PRESSED_STATE LOW
#define BUTTON_RELEASED_STATE HIGH
#define BUTTON_THROTTLE_MILLIS 300L
// maintenance button on back
#define BUTTON_MT 12
// left/right top/bottom buttons
#define BUTTON_LB 3
#define BUTTON_LT 4
#define BUTTON_RB 5
#define BUTTON_RT 6

#define ACTION_NONE 0
#define ACTION_INVALID -1
#define ACTION_RESET 1

struct BUTTON_STATE {
  const int pin;
  const int action;
};

struct BUTTON_STATE buttonStates[] = {
  {BUTTON_LB, MEDIA_KEY_VOLUME_DOWN},
  {BUTTON_RB, MEDIA_KEY_VOLUME_UP},
  {BUTTON_LT, KEYCODE_PAGE_DOWN},
  {BUTTON_RT, KEYCODE_PAGE_UP},
  {BUTTON_MT, ACTION_RESET},
};

// Debugging/test mode without buttons
//#define NOBUTTON_TEST 1

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
  pinMode(LED_STATUS, OUTPUT);
  pinMode(LED_ACTION, OUTPUT);
  for (int i = 0; i < sizeof(buttonStates) / sizeof(struct BUTTON_STATE); i++) {
    pinMode(buttonStates[i].pin, INPUT_PULLUP);
  }

  // Initialize both BLE and the HID.
  keyboard.initialize();

  // Launch the event queue that will manage both BLE events and the loop.
  // After this call the main thread will be halted.
  MbedBleHID_RunEventThread();
}

// loop() is run repeatedly
void loop() {
  unsigned long now = millis();
  static unsigned long lastAction = 0;
  static bool lastPressed = false;

#ifdef NOBUTTON_TEST
  // https://forum.arduino.cc/t/arduino-perform-task-only-once-every-second/240495/6
  static const unsigned long REFRESH_INTERVAL = 100; // cycles
  static unsigned long lastRefreshTime = 0;
  static bool up = false;
  if (lastRefreshTime < REFRESH_INTERVAL) {
    lastRefreshTime++;
    return;
  }
  lastRefreshTime = 0;
#endif

  // When disconnected, we animate the builtin LED to indicate the device state.
  if (!keyboard.connected()) {
    animateLED(LED_BUILTIN, (keyboard.has_error()) ? kLedErrorDelayMilliseconds
                                                   : kLedBeaconDelayMilliseconds);
                                                   
    animateLED(LED_STATUS, (keyboard.has_error()) ? kLedErrorDelayMilliseconds
                                                   : kLedBeaconDelayMilliseconds);
                                                   
    animateLED(LED_ACTION, (keyboard.has_error()) ? kLedErrorDelayMilliseconds
                                                   : kLedBeaconDelayMilliseconds);
    return;
  }
  analogWrite(LED_BUILTIN, kLedConnectedIntensity);
  analogWrite(LED_STATUS, LED_STATUS_BRIGHTNESS);

  int action = ACTION_NONE;
  bool anyPressed = false;
  
  for (int i = 0; i < sizeof(buttonStates) / sizeof(struct BUTTON_STATE); i++) {
    struct BUTTON_STATE buttonState = buttonStates[i];
    int currentState = digitalRead(buttonState.pin);

    if (currentState == BUTTON_PRESSED_STATE) {
      // Light LED while any is pressed
      anyPressed = true;
      // Immediately execute action on first press, or require long press to repeat
      if (!lastPressed || (now - lastAction) > BUTTON_THROTTLE_MILLIS) {
        // Do action
        if (action == ACTION_NONE) {
          action = buttonState.action;
        } else {
          action = ACTION_INVALID;
        }
        lastAction = now;
      }
    }
  }

  analogWrite(LED_ACTION, anyPressed ? LED_ACTION_BRIGHTNESS : 0);
  lastPressed = anyPressed;

  auto *kb = keyboard.hid();

#ifndef NOBUTTON_TEST

  if (action == ACTION_NONE || action == ACTION_INVALID) {
    return;
  }

  if (action == MEDIA_KEY_VOLUME_UP || action == MEDIA_KEY_VOLUME_DOWN) {
    kb->media_keydown(action);
    kb->SendReport();
    kb->keyup();
    kb->SendReport();
  } else if (action == ACTION_RESET) {
    // TODO implement me
  } else {
    // TODO fix me
    kb->keydown(KeySym_t(action));
    kb->SendReport();
    kb->keyup();
    kb->SendReport();
  }

#else
  if (up) {
    kb->media_keydown(MEDIA_KEY_VOLUME_UP);
  } else {
    kb->media_keydown(MEDIA_KEY_VOLUME_DOWN);
  }
  kb->SendReport();
  kb->keyup();
  kb->SendReport();
  up = !up;
#endif
}

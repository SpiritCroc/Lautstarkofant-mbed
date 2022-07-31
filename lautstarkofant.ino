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
#define BUTTON_LT 6
#define BUTTON_RB 5
#define BUTTON_RT 4

enum ACTION {
  NONE,
  INVALID,
  REPEAT_SINGLE_PRESS,
  RESET,
  VOLUME_UP,
  VOLUME_DOWN,
  PAUSE,
  PAGE_DOWN,
  PAGE_UP,
  DOUBLE_PAGE_UP
};

struct BUTTON_STATE {
  const int pin;
  const int action;
  const int longPressAction = NONE;
  const bool canRepeat = false;
  const int repeatThrottle = BUTTON_THROTTLE_MILLIS;
};

BUTTON_STATE buttonStates[] = {
  {BUTTON_LB, VOLUME_DOWN, REPEAT_SINGLE_PRESS},
  {BUTTON_RB, VOLUME_UP, REPEAT_SINGLE_PRESS},
  //{BUTTON_LT, PAGE_UP},
  //{BUTTON_RT, PAGE_DOWN},
  {BUTTON_LT, PAUSE},
  {BUTTON_RT, PAGE_DOWN, PAGE_UP, true, 1000},
  {BUTTON_MT, INVALID, RESET}
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
  for (unsigned int i = 0; i < sizeof(buttonStates) / sizeof(struct BUTTON_STATE); i++) {
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
  static bool lastLongPressed = false;

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

  int action = NONE;
  bool anyPressed = false;
  
  for (unsigned int i = 0; i < sizeof(buttonStates) / sizeof(struct BUTTON_STATE); i++) {
    struct BUTTON_STATE buttonState = buttonStates[i];
    int currentState = digitalRead(buttonState.pin);

    if (currentState == BUTTON_PRESSED_STATE) {
      // Light LED while any is pressed
      anyPressed = true;
      int thisAction = NONE;
      if (!lastPressed) {
        // Immediately execute action on first press
        thisAction = buttonState.action;
        lastLongPressed = false;
      } else if ((now - lastAction) > buttonState.repeatThrottle) {
        // Long press action
        if (buttonState.longPressAction == REPEAT_SINGLE_PRESS) {
          thisAction = buttonState.action;
        } else if (!lastLongPressed && buttonState.longPressAction != NONE) {
          thisAction = buttonState.longPressAction;
          // We only want to do long press action once
          if (!buttonState.canRepeat) {
            lastLongPressed = true;
          }
        }
      }
      if (thisAction != NONE) {
        // Ensure we don't do any action when pressing multiple buttons
        if (action == NONE) {
          action = thisAction;
        } else {
          action = INVALID;
        }
        lastAction = now;
      }
    }
  }

  analogWrite(LED_ACTION, anyPressed ? LED_ACTION_BRIGHTNESS : 0);
  lastPressed = anyPressed;

  auto *kb = keyboard.hid();

#ifndef NOBUTTON_TEST

  int media_key = 0;
  int key = 0;
  int keyCount = 1;
  switch (action) {
    case VOLUME_DOWN:
      media_key = MEDIA_KEY_VOLUME_DOWN;
      break;
    case VOLUME_UP:
      media_key = MEDIA_KEY_VOLUME_UP;
      break;
    case PAUSE:
      media_key = MEDIA_KEY_PLAY_PAUSE;
      break;
    case PAGE_DOWN:
      key = KEYCODE_PAGE_DOWN;
      break;
    case PAGE_UP:
      key = KEYCODE_PAGE_UP;
      break;
    case DOUBLE_PAGE_UP:
      key = KEYCODE_PAGE_UP;
      keyCount = 2;
      break;
    case RESET:
      NVIC_SystemReset();
      break;
  }
  if (media_key || key) {
    while (keyCount > 0) {
      if (media_key) {
        kb->media_keydown(media_key);
      }
      if (key) {
        KeySym_t keySym = KeySym_t(0);
        keySym.usage = key;
        kb->keydown(keySym);
      }
      kb->SendReport();
      kb->keyup();
      kb->SendReport();
      keyCount--;
    }
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

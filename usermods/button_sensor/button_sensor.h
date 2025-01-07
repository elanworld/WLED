#include "wled.h"

#ifndef BUTTON_SENSOR_ENABLE
#define BUTTON_SENSOR_ENABLE false
#endif

#ifndef BUTTON_SENSOR_PIN
#define BUTTON_SENSOR_PIN 25
#endif

#ifndef BUTTON_SENSOR_PULLUP
#define BUTTON_SENSOR_PULLUP true
#endif

#ifndef BUTTON_SENSOR_INTERVAL
#define BUTTON_SENSOR_INTERVAL 300
#endif
volatile bool sensorTriggered = false;

void IRAM_ATTR handleSensorInterrupt() { sensorTriggered = true; }

class ButtonSensorUsermod : public Usermod {
 private:
  bool enable = BUTTON_SENSOR_ENABLE;
  int sensorPin = BUTTON_SENSOR_PIN;
  bool pullUp = BUTTON_SENSOR_PULLUP;
  unsigned long interval = BUTTON_SENSOR_INTERVAL;
  unsigned long lastTriggerdTime = 0;

  // string that are used multiple time (this will save some flash memory)
  static const char _name[];
  static const char _enabled[];

 public:
  void setup() override {
    if (!enable) {
      return;
    }
    if (pinManager.allocatePin(sensorPin, false, PinOwner::UM_BUTTON_SENSOR)) {
      // 设置GPIO引脚的中断，任何引脚电平变化都会触发中断
      attachInterrupt(digitalPinToInterrupt(sensorPin), handleSensorInterrupt,
                      CHANGE);
      pinMode(sensorPin, pullUp ? INPUT_PULLUP : INPUT_PULLDOWN);
      DEBUG_PRINTLN("Button Sensor Usermod Initialized");
    } else {
      DEBUG_PRINTLN("Button Sensor Usermod allocate Pin fail");
    }
  }

  void loop() override {
    if (!enable) {
      return;
    }
    unsigned long currentMillis = millis();

    if (currentMillis - lastTriggerdTime >= interval) {
      lastTriggerdTime = currentMillis;
      if (sensorTriggered) {
        sensorTriggered = false;
        ++effectCurrent %= strip.getModeCount();
        stateChanged = true;
        colorUpdated(CALL_MODE_BUTTON);
      }
    }
  }
  // 在config中添加新选项
  virtual void addToConfig(JsonObject &root) override {
    JsonObject top = root.createNestedObject(FPSTR(_name));
    top["enable"] = enable;
    top["pin"] = sensorPin;
    top["pullUp"] = pullUp;
  }

  // 从config读取设置
  virtual bool readFromConfig(JsonObject &root) override {
    JsonObject top = root[FPSTR(_name)];
    bool configComplete = !top.isNull();
    configComplete &= getJsonValue(top["enable"], enable);
    int newSensorPin;
    configComplete &= getJsonValue(top["pin"], newSensorPin);
    bool newPull;
    configComplete &= getJsonValue(top["pullUp"], newPull);
    if (newSensorPin != sensorPin || newPull != pullUp) {
      pinManager.deallocatePin(sensorPin, PinOwner::UM_BUTTON_SENSOR);
      sensorPin = newSensorPin;
      pullUp = newPull;
      setup();
    }
    return configComplete;
  }

  /*
   * getId() allows you to optionally give your V2 usermod an unique ID (please
   * define it in const.h!). This could be used in the future for the system to
   * determine whether your usermod is installed.
   */
  uint16_t getId() { return USERMOD_ID_BUTTON_SENSOR; }
};

const char ButtonSensorUsermod::_name[] PROGMEM = "ButtonSensor";
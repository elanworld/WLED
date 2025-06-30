#include <esp_sleep.h>
#include <sys/time.h>

#include "wled.h"
#ifndef LOCALTIMER_ENABLE
#define LOCALTIMER_ENABLE true
#endif
#ifndef LOCALTIMER_SAVETIME
#define LOCALTIMER_SAVETIME 5
#endif
#ifndef LOCALTIMER_SAVETIME_MULTIPLIER
#define LOCALTIMER_SAVETIME_MULTIPLIER 10
#endif

// save time in rtc momory
RTC_DATA_ATTR struct timeval rtcTime = {0};
RTC_DATA_ATTR Toki::Time tokiTime = {0, 0};

class LocalTimerUsermod : public Usermod {
 private:
  bool enable = LOCALTIMER_ENABLE;
  int saveTime = LOCALTIMER_SAVETIME;
  int saveMultiplier = LOCALTIMER_SAVETIME_MULTIPLIER;
  unsigned long lastSaveTime = 0;
  int saveCounter = 0;
  // string that are used multiple time (this will save some flash memory)
  static const char _name[];
  static const char _enabled[];

 public:
  void setup() {
    if (!enable) return;
    if (!firstBoot()) {
      struct timeval tv_now;
      gettimeofday(&tv_now, NULL);

      int64_t deltaSec = tv_now.tv_sec - rtcTime.tv_sec;
      int64_t deltaMs = (tv_now.tv_usec - rtcTime.tv_usec) / 1000;

      if (deltaMs < 0) {
        deltaMs += 1000;
        deltaSec -= 1;
      }

      tokiTime.sec += deltaSec;
      tokiTime.ms += deltaMs;
      toki.setTime(tokiTime);
      DEBUG_PRINTF("sleep seconds: %d\n", deltaSec);
    } else {
      toki.setTime(tokiTime);
    }
  }

  void loop() {
    if (!enable) return;

    unsigned long currentMillis = millis();
    if (currentMillis - lastSaveTime >= saveTime * 1000) {
      lastSaveTime = currentMillis;
      gettimeofday(&rtcTime, NULL);
      tokiTime = toki.getTime();
      saveCounter++;
      if (saveCounter % saveMultiplier == 0) {
        serializeConfig();
        DEBUG_PRINTF("save rtc time: %d, toki time: %d\n", rtcTime.tv_sec,
                     tokiTime.sec);
      }
    }
  }

  virtual void addToConfig(JsonObject &root) override {
    JsonObject top = root.createNestedObject(FPSTR(_name));
    top[FPSTR(_enabled)] = enable;
    top["saveTime"] = saveTime;
    top["saveMultiplier"] = saveMultiplier;
    top["tokiTimeSec"] = tokiTime.sec;
    top["tokiTimeMs"] = tokiTime.ms;
  }

  virtual bool readFromConfig(JsonObject &root) override {
    JsonObject top = root[FPSTR(_name)];
    bool configComplete = !top.isNull();
    configComplete &= getJsonValue(top[FPSTR(_enabled)], enable);

    uint32_t savedSec;
    uint16_t savedMs;
    configComplete &= getJsonValue(top["saveTime"], saveTime);
    configComplete &= getJsonValue(top["saveMultiplier"], saveMultiplier);
    configComplete &= getJsonValue(top["tokiTimeSec"], savedSec);
    configComplete &= getJsonValue(top["tokiTimeMs"], savedMs);
    bool update = false;
    configComplete &= getJsonValue(top["update"], update);
    if (update)
    {
      toki.setTime(savedSec);
    }
    return configComplete;
  }
  bool firstBoot() {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    return wakeup_reason == ESP_SLEEP_WAKEUP_UNDEFINED;
  }
};

const char LocalTimerUsermod::_name[] PROGMEM = "LocalTimer";
const char LocalTimerUsermod::_enabled[] PROGMEM = "enable";
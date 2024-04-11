#ifndef SLEEP_MANAGER
#define SLEEP_MANAGER

#include "wled.h"
#include "esp_pm.h"
#include <map>

static int boot_ide_sec = 20; // 启动后等待时间
static int wait_user_sec = 20; // 可以睡眠后等待用户的时间

static std::map<int, int> config_sleep_time = {
    {60, 20},
    {300, 40},
    {600, 60},
    {3600, 300},
};

class SleepManager : public Usermod
{
public:
    virtual void setup()
    {
        if (!enableSleep)
        {
            return;
        }
        
        DEBUG_PRINTF("change_time %ds %ldus\n",int(change_time.tv_sec), change_time.tv_usec);

        esp_pm_config_esp32_t pmConfig;
        pmConfig.max_freq_mhz = 240;
        pmConfig.min_freq_mhz = 80;
        pmConfig.light_sleep_enable = true;
        ESP_ERROR_CHECK(esp_pm_configure(&pmConfig));
        DEBUG_PRINTF("esp_pm_configure\n");
    }

    virtual void loop()
    {
        if (!enableSleep)
        {
            return;
        }
        if (false && millis() > boot_ide_sec * 1000 && shouldEnterSleep() && getCurrentTimeInSeconds() > wait_user_sec && bri == 0)
        {
            DEBUG_PRINTF("change_time duration %ds\n", getCurrentTimeInSeconds());
            // Enable wakeup from deep sleep by rtc timer
            const int wakeup_time_sec = getNextSleepTime();
            DEBUG_PRINTF("Enabling timer wakeup, %ds\n", wakeup_time_sec);
            ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000));
            // enter deep sleep
            esp_deep_sleep_start();
        }
    }

    // 获取下一个睡眠时间
    int getNextSleepTime() const
    {
        int currentTime = getCurrentTimeInSeconds();
        for (const auto& entry : config_sleep_time)
        {
            if (currentTime < entry.first)
            {
                return entry.second;
            }
        }
        // 如果当前时间大于配置的最大时间，返回最后一个配置的睡眠时间
        return config_sleep_time.rbegin()->second;
    }

    // 获取当前时间和change_time的差异
    int getCurrentTimeInSeconds() const
    {
        struct timeval currentTime;
        gettimeofday(&currentTime, NULL);
        return static_cast<int>(currentTime.tv_sec - change_time.tv_sec);
    }

    // 检查所有模块是否都不需要唤醒
    bool shouldEnterSleep() const
    {
        for (const auto& pair : modules)
        {
            if (pair.second)
            {
                // 如果有一个模块需要唤醒，返回false
                return false;
            }
        }
        // 所有模块都不需要唤醒
        return true;
    }

    virtual void addToConfig(JsonObject &root)
    {
      JsonObject top = root.createNestedObject("SleepModule");
      top["enableSleep"] = enableSleep;
    }
    virtual bool readFromConfig(JsonObject &root)
    {
      JsonObject top = root["SleepModule"];
      bool configComplete = !top.isNull();
      configComplete &= getJsonValue(top["enableSleep"], enableSleep);
      return configComplete;
    }
};

#endif

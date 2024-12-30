#ifndef SLEEP_MANAGER_H
#define SLEEP_MANAGER_H

#include "wled.h"
#include "esp_pm.h"
#include <map>
#include "esp_system.h"
#include <driver/touch_pad.h>
#include <string>
#ifndef WAKEUP_TOUCH_PIN
#ifdef CONFIG_IDF_TARGET_ESP32S3 // ESP32S3
#define WAKEUP_TOUCH_PIN 5
#else
#define WAKEUP_TOUCH_PIN 15
#endif
#endif //WAKEUP_TOUCH_PIN

class SleepManager : public Usermod
{
private:
    boolean sleepOnIdle = false;
    int voltagePin = 34;
    unsigned long voltageCheckInterval = 5;
    unsigned long lastVoltageCheckTime = 0;
    float voltage;
    unsigned long lastLedOffTime = -1;
    unsigned long idleWaitSeconds = 60;
    bool isTestingBattery = false;
    bool sleepNextLoop = false;
    float minVoltage = 3.0;      // 最低电压（例如：3.0V）
    float maxVoltage = 4.2;      // 最高电压（例如：4.2V）
    float voltageDivRatio = 2.0; // 分压比，用于电池电压检测
    // 当前电压值
    float currentVoltage = 0.0;
    bool presetWakeup = true;

public:
    virtual void setup()
    {
        int gpioPinsDown[] = {25, 26};
        pull_up_down(gpioPinsDown, sizeof(gpioPinsDown) / sizeof(gpioPinsDown[0]), false, true);
        int gpioPinsup[] = {27};
        pull_up_down(gpioPinsup, sizeof(gpioPinsup) / sizeof(gpioPinsup[0]), true, false);
        pinManager.allocatePin(voltagePin, false, PinOwner::Button);
    }

    virtual void loop()
    {
        if (!enableSleep)
        {
            return;
        }
        unsigned long currentMillis = millis();

        // 每隔 voltageCheckInterval 时间检测一次电压
        if (currentMillis - lastVoltageCheckTime >= voltageCheckInterval * 1000)
        {
            lastVoltageCheckTime = currentMillis;
            if (sleepNextLoop)
            {
                startDeepSeelp(true);
            }

            if (sleepOnIdle && lastLedOffTime != -1 && currentMillis - lastLedOffTime > idleWaitSeconds * 1000 && !haveWakeupLock())
            {
                DEBUG_PRINTLN("sleep on idle...");
                startDeepSeelp(false);
            }

            // 读取当前电压值
            float voltage = readVoltage() * voltageDivRatio;

            // 打印当前电压
            DEBUG_PRINTF("Current voltage on IO%d: %.3f\n", voltagePin, voltage);
            if (isTestingBattery)
            {
                minVoltage = voltage;
                serializeConfig();
            }

            // 检查电压是否低于阈值
            if (voltage < minVoltage)
            {
                if (voltage != 0)
                {
                    DEBUG_PRINTLN("Voltage is below threshold. Entering deep sleep...");
                    startDeepSeelp(false);
                }
            }
        }
    }

    virtual void onStateChange(uint8_t mode)
    {
        DEBUG_PRINTF("current bri value: %d,effect: %d\n", bri, effectCurrent);
        if (bri == 0)
        {
            lastLedOffTime = millis();
        }
        else
        {
            lastLedOffTime = -1;
        }
    }

    void startDeepSeelp(bool immediate)
    {
        if (immediate)
        {
            DEBUG_PRINTLN("Entering deep sleep...");
            if (presetWakeup)
            {
                int nextWakeupMin = findNextTimerInterval() - 1;
                if (nextWakeupMin > 0)
                {
                    esp_sleep_enable_timer_wakeup(nextWakeupMin * 60ULL * 1000000ULL); // wakeup for preset
                    DEBUG_PRINTF("wakeup after %d minites", nextWakeupMin);
                    DEBUG_PRINTLN("");
                }
            }
            int gpioPinsDown[] = {4, 27};
            pull_up_down(gpioPinsDown, sizeof(gpioPinsDown) / sizeof(gpioPinsDown[0]), false, true);
            int gpioPinsup[] = {0, 26, 25};
            pull_up_down(gpioPinsup, sizeof(gpioPinsup) / sizeof(gpioPinsup[0]), true, false);
            WiFi.disconnect();
            WiFi.mode(WIFI_OFF);
            // 初始化触摸传感器
            touchSleepWakeUpEnable(WAKEUP_TOUCH_PIN, touchThreshold);
            ESP_ERROR_CHECK(esp_sleep_enable_ext0_wakeup(GPIO_NUM_26, 0));
            ESP_ERROR_CHECK(esp_sleep_enable_ext1_wakeup(1ULL << GPIO_NUM_0, ESP_EXT1_WAKEUP_ALL_LOW));
            delay(2000); // wati gpio level restore ...
#ifndef ARDUINO_ARCH_ESP32C3
#endif
            esp_deep_sleep_start();
        }
        else
        {
            sleepNextLoop = true;
            briLast = bri;
            bri = 0;
            stateUpdated(CALL_MODE_DIRECT_CHANGE);
        }
    }

    void pull_up_down(int gpioPins[], int numPins, bool up, bool down)
    {
        for (int i = 0; i < numPins; i++)
        {
            gpio_set_direction((gpio_num_t)gpioPins[i], GPIO_MODE_INPUT); // 设置为输入模式（高阻态）
            gpio_pullup_dis((gpio_num_t)gpioPins[i]);                     // 禁用上拉电阻
            gpio_pulldown_dis((gpio_num_t)gpioPins[i]);                   // 禁用下拉电阻
            if (up)
            {
                ESP_ERROR_CHECK(gpio_pullup_en((gpio_num_t)gpioPins[i]));
            }
            if (down)
            {
                ESP_ERROR_CHECK(gpio_pulldown_en((gpio_num_t)gpioPins[i]));
            }
        }
    }

    // 电压检测函数
    float readVoltage()
    {
        int adcValue = analogRead(voltagePin);
        float voltageOut = (adcValue / float(4095)) * 3.3;
        return voltageOut;
    }

    // 检查所有模块是否都不需要唤醒
    bool haveWakeupLock() const
    {
        for (const auto &pair : modules)
        {
            if (pair.second)
            {
                // 如果有一个模块需要唤醒，返回false
                return true;
            }
        }
        // 所有模块都不需要唤醒
        return false;
    }

    // 展示电压信息的函数
    void addToJsonInfo(JsonObject &root)
    {
        JsonObject user = root["u"];
        if (user.isNull())
            user = root.createNestedObject("u");

        // 获取电压值
        currentVoltage = readVoltage();

        // 计算电压百分比
        float batteryPercentage = calculateBatteryPercentage(currentVoltage * voltageDivRatio);

        // 将电压信息添加到 JSON 对象
        JsonArray percentage = user.createNestedArray(F("Battery Percentage"));
        JsonArray voltage = user.createNestedArray(F("Current Voltage"));
        JsonArray boot = user.createNestedArray(F("boot type"));
        percentage.add(batteryPercentage);
        percentage.add(F(" %"));
        voltage.add(round(currentVoltage * voltageDivRatio * 100.0) / 100.0);
        voltage.add(F(" V"));
        // 检查唤醒原因
        boot.add(F(phase_wakeup_reason()));
    }

    // 在配置中添加电压相关配置
    void addToConfig(JsonObject &root)
    {
        DEBUG_PRINTLN("sleep module addToConfig");
        JsonObject top = root.createNestedObject("Sleep Module");

        // 添加电压相关配置项
        top["enable Sleep"] = enableSleep;
        top["voltage Check Interval"] = voltageCheckInterval; // 电压检测间隔
        top["voltage Pin"] = voltagePin;                      // 电压检测引脚
        top["Min Voltage"] = minVoltage;
        top["Max Voltage"] = maxVoltage;
        top["Voltage Div Ratio"] = voltageDivRatio;
        top["sleep On Idle"] = sleepOnIdle;
        top["idle Wait Seconds"] = idleWaitSeconds;
        top["battery Test Enabled"] = false;
        top["preset Wakeup"] = presetWakeup;
    }

    // 从配置中读取电压相关配置
    bool readFromConfig(JsonObject &root)
    {
        DEBUG_PRINTLN("sleep module readFromConfig");
        JsonObject top = root["Sleep Module"];
        bool configComplete = !top.isNull();

        // 读取电压相关配置项
        configComplete &= getJsonValue(top["enable Sleep"], enableSleep);
        configComplete &= getJsonValue(top["voltage Check Interval"], voltageCheckInterval);
        configComplete &= getJsonValue(top["voltage Pin"], voltagePin);
        configComplete &= getJsonValue(top["Min Voltage"], minVoltage);
        configComplete &= getJsonValue(top["Max Voltage"], maxVoltage);
        configComplete &= getJsonValue(top["Voltage Div Ratio"], voltageDivRatio);
        configComplete &= getJsonValue(top["sleep On Idle"], sleepOnIdle);
        configComplete &= getJsonValue(top["idle Wait Seconds"], idleWaitSeconds);
        configComplete &= getJsonValue(top["battery Test Enabled"], isTestingBattery);
        configComplete &= getJsonValue(top["preset Wakeup"], presetWakeup);

        return configComplete;
    }
    // 计算电压百分比
    int calculateBatteryPercentage(float voltage)
    {
        int percent = (voltage - minVoltage) / (maxVoltage - minVoltage) * 100.0;
        if (percent < 0)
        {
            percent = 0;
        }
        if (percent > 100)
        {
            percent = 100;
        }
        return percent;
    }

    // 辅助函数：计算两个时间的分钟差
    int calculateTimeDifference(int hour1, int minute1, int hour2, int minute2)
    {
        int totalMinutes1 = hour1 * 60 + minute1;
        int totalMinutes2 = hour2 * 60 + minute2;
        if (totalMinutes2 < totalMinutes1)
        {
            // 如果目标时间比当前时间早，说明跨天了
            totalMinutes2 += 24 * 60;
        }
        return totalMinutes2 - totalMinutes1;
    }

    // 查找下一个最近的启用时间
    int findNextTimerInterval()
    {
        // 获取当前时间的小时、分钟、星期几
        int currentHour = hour(localTime), currentMinute = minute(localTime), currentWeekday = weekdayMondayFirst();
        int minDifference = INT_MAX; // 初始化为最大值

        for (uint8_t i = 0; i < 8; i++)
        {
            if (!(timerMacro[i] != 0 && (timerWeekday[i] & 0x01)))
            {
                continue; // 跳过未启用的定时器
            }

            for (int dayOffset = 0; dayOffset < 7; dayOffset++)
            {
                int checkWeekday = ((currentWeekday + dayOffset) % 7); // 计算未来的星期几 1-7
                if (checkWeekday == 0)
                {
                    checkWeekday = 7;
                }

                if ((timerWeekday[i] >> (checkWeekday)) & 0x01)
                {
                    // 如果是今天，检查时间是否已经过去
                    if (dayOffset == 0 &&
                        (timerHours[i] < currentHour ||
                         (timerHours[i] == currentHour && timerMinutes[i] <= currentMinute)))
                    {
                        continue;
                    }

                    // 计算时间差
                    int targetHour = timerHours[i];
                    int targetMinute = timerMinutes[i];
                    int timeDifference = calculateTimeDifference(
                        currentHour, currentMinute,
                        targetHour + (dayOffset * 24), targetMinute);

                    if (timeDifference < minDifference)
                    {
                        minDifference = timeDifference;
                    }
                }
            }
        }
        return minDifference; // 返回分钟差
    }
    const char *phase_wakeup_reason()
    {
        static char reson[20]; // 使用 static 保证数组在函数外部依然有效
        esp_sleep_wakeup_cause_t wakeup_reason;

        wakeup_reason = esp_sleep_get_wakeup_cause();

        switch (wakeup_reason)
        {
        case ESP_SLEEP_WAKEUP_EXT0:
            Serial.println("Wakeup caused by external signal using RTC_IO");
            strcpy(reson, "RTC_IO");
            break;
        case ESP_SLEEP_WAKEUP_EXT1:
            Serial.println("Wakeup caused by external signal using RTC_CNTL");
            strcpy(reson, "RTC_CNTL");
            break;
        case ESP_SLEEP_WAKEUP_TIMER:
            Serial.println("Wakeup caused by timer");
            strcpy(reson, "timer");
            break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD:
            Serial.println("Wakeup caused by touchpad");
            strcpy(reson, "touchpad");
            break;
        case ESP_SLEEP_WAKEUP_ULP:
            Serial.println("Wakeup caused by ULP program");
            strcpy(reson, "ULP");
            break;
        default:
            Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
            snprintf(reson, sizeof(reson), "other %d", wakeup_reason); // 正确格式化
            break;
        }
        return reson;
    }
};
#endif

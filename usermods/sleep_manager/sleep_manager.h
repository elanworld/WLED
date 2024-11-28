#ifndef SLEEP_MANAGER
#define SLEEP_MANAGER

#include "wled.h"
#include "esp_pm.h"
#include <map>

boolean sleepOnIdle = false;
int voltagePin = 34;
float voltageThreshold = 0.2;
unsigned long voltageCheckInterval = 5000;
unsigned long lastVoltageCheckTime = 0;
float voltage;
unsigned long lastLedOffTime = 0;
unsigned long idleWaitSeconds = 60;

class SleepManager : public Usermod
{
public:
    virtual void setup()
    {
        if (!enableSleep)
        {
            return;
        }
    }

    virtual void loop()
    {
        if (!enableSleep)
        {
            return;
        }
        unsigned long currentMillis = millis();

        // 每隔 voltageCheckInterval 时间检测一次电压
        if (currentMillis - lastVoltageCheckTime >= voltageCheckInterval)
        {
            if (sleepOnIdle && lastLedOffTime != 0 && currentMillis - lastLedOffTime > idleWaitSeconds * 1000)
            {
                DEBUG_PRINTLN("sleep on idle...");
                startDeepSeelp();
            }
            lastVoltageCheckTime = currentMillis;

            // 读取当前电压值
            float voltage = readVoltage();

            // 打印当前电压
            DEBUG_PRINTF("Current voltage on IO%d: %.3f\n", voltagePin, voltage);

            // 检查电压是否低于阈值
            if (voltage < voltageThreshold)
            {
                if (voltage == 0)
                {
                    DEBUG_PRINTLN("Voltage is zoro");
                }
                else
                {
                    DEBUG_PRINTLN("Voltage is below threshold. Entering deep sleep...");
                    startDeepSeelp();
                }
            }
        }
    }

    virtual void onStateChange(uint8_t mode)
    {
        DEBUG_PRINTF("current bri value: %d\n", bri);
        if (bri == 0)
        {
            lastLedOffTime = millis();
        }
        else
        {
            lastLedOffTime = 0;
        }
    }

    void startDeepSeelp()
    {
        briLast = bri;
        bri = 0;
        stateUpdated(CALL_MODE_DIRECT_CHANGE);
        gpio_pulldown_dis(GPIO_NUM_0); // 确保没有内部上拉
        gpio_pullup_en(GPIO_NUM_0);
        delay(1000); // wati votage restore ...
        ESP_ERROR_CHECK(esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0));
        esp_deep_sleep_start();
    }

    // 电压检测函数
    float readVoltage()
    {
        int adcValue = analogRead(voltagePin);
        float voltageOut = (adcValue / float(4095)) * 3.3;
        return voltageOut;
    }

    // 检查所有模块是否都不需要唤醒
    bool checkWakeupLock() const
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

    // 在config中添加新选项
    virtual void addToConfig(JsonObject &root)
    {
        JsonObject top = root.createNestedObject("Sleep Module");
        top["enable Sleep"] = enableSleep;
        top["voltage Pin"] = voltagePin;                      // 电压检测引脚
        top["voltage Threshold"] = voltageThreshold;          // 电压阈值
        top["voltage Check Interval"] = voltageCheckInterval; // 电压检测间隔
        top["voltage Current"] = readVoltage();
        top["sleep On Idle"] = sleepOnIdle;
        top["idle Wait Seconds"] = idleWaitSeconds;
    }

    // 从config读取设置
    virtual bool readFromConfig(JsonObject &root)
    {
        JsonObject top = root["Sleep Module"];
        bool configComplete = !top.isNull();
        configComplete &= getJsonValue(top["enable Sleep"], enableSleep);
        configComplete &= getJsonValue(top["voltage Pin"], voltagePin);
        configComplete &= getJsonValue(top["voltage Threshold"], voltageThreshold);
        configComplete &= getJsonValue(top["voltage Check Interval"], voltageCheckInterval);
        configComplete &= getJsonValue(top["voltage Current"], voltage);
        configComplete &= getJsonValue(top["sleep On Idle"], sleepOnIdle);
        configComplete &= getJsonValue(top["idle Wait Seconds"], idleWaitSeconds);
        return configComplete;
    }
};

#endif

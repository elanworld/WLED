#ifndef SLEEP_MANAGER_H
#define SLEEP_MANAGER_H

#include "wled.h"
#include "esp_pm.h"
#include <map>
class SleepManager : public Usermod
{
private:
    boolean sleepOnIdle = false;
    int voltagePin = 34;
    float voltageThreshold = 0.2;
    unsigned long voltageCheckInterval = 5000;
    unsigned long lastVoltageCheckTime = 0;
    float voltage;
    unsigned long lastLedOffTime = 0;
    unsigned long idleWaitSeconds = 60;
    bool isTestingBattery = false;
    bool sleepNextLoop = false;

public:
    virtual void setup()
    {
            int gpioPinsDown[] = {25,26};
            pull_up_down(gpioPinsDown,sizeof(gpioPinsDown) / sizeof(gpioPinsDown[0]),false,true);
            int gpioPinsup[] = {27};
            pull_up_down(gpioPinsup,sizeof(gpioPinsup) / sizeof(gpioPinsup[0]),true,false);
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
            if (sleepNextLoop)
            {
                startDeepSeelp(true);
            }

            if (sleepOnIdle && lastLedOffTime != 0 && currentMillis - lastLedOffTime > idleWaitSeconds * 1000 && !haveWakeupLock())
            {
                DEBUG_PRINTLN("sleep on idle...");
                startDeepSeelp(false);
            }
            lastVoltageCheckTime = currentMillis;

            // 读取当前电压值
            float voltage = readVoltage();

            // 打印当前电压
            DEBUG_PRINTF("Current voltage on IO%d: %.3f\n", voltagePin, voltage);
            if (isTestingBattery)
            {
                voltageThreshold = voltage;
                serializeConfig();
            }

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
            lastLedOffTime = 0;
        }
    }

    void startDeepSeelp(bool immediate)
    {
        if (immediate)
        {
            DEBUG_PRINTLN("Entering deep sleep...");
            int gpioPins[] = {2, 4, 5, 12, 13, 14, 16, 17, 18};
            pull_up_down(gpioPins,sizeof(gpioPins) / sizeof(gpioPins[0]),false,false);
            int gpioPinsDown[] = {27};
            pull_up_down(gpioPinsDown,sizeof(gpioPinsDown) / sizeof(gpioPinsDown[0]),false,true);
            int gpioPinsup[] = {26,25};
            pull_up_down(gpioPinsup,sizeof(gpioPinsup) / sizeof(gpioPinsup[0]),true,false);
            WiFi.disconnect();
            WiFi.mode(WIFI_OFF);
            delay(2000); // wati gpio level restore ...
#ifndef ARDUINO_ARCH_ESP32C3
            ESP_ERROR_CHECK(esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0));
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

    // 在config中添加新选项
    virtual void addToConfig(JsonObject &root)
    {
        JsonObject top = root.createNestedObject("Sleep Module");
        top["enable Sleep"] = enableSleep;
        top["voltage Pin"] = voltagePin;                      // 电压检测引脚
        top["voltage Threshold"] = voltageThreshold;          // 电压阈值
        top["voltage Check Interval"] = voltageCheckInterval; // 电压检测间隔
        top["voltage"] = readVoltage();
        top["sleep On Idle"] = sleepOnIdle;
        top["idle Wait Seconds"] = idleWaitSeconds;
        top["battery Test Enabled"] = false;
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
        configComplete &= getJsonValue(top["voltage"], voltage);
        configComplete &= getJsonValue(top["sleep On Idle"], sleepOnIdle);
        configComplete &= getJsonValue(top["idle Wait Seconds"], idleWaitSeconds);
        configComplete &= getJsonValue(top["battery Test Enabled"], isTestingBattery);
        return configComplete;
    }
};

#endif

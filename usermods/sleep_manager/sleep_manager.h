#ifndef SLEEP_MANAGER_H
#define SLEEP_MANAGER_H

#include "wled.h"
#include "esp_pm.h"
#include <map>
#include "esp_partition.h"
#include "esp_system.h"
#include "esp_ota_ops.h"

class SleepManager : public Usermod
{
private:
    boolean sleepOnIdle = false;
    int voltagePin = 34;
    unsigned long voltageCheckInterval = 5;
    unsigned long lastVoltageCheckTime = 0;
    float voltage;
    unsigned long lastLedOffTime = 0;
    unsigned long idleWaitSeconds = 60;
    bool isTestingBattery = false;
    bool sleepNextLoop = false;
    float minVoltage = 3.0;      // 最低电压（例如：3.0V）
    float maxVoltage = 4.2;      // 最高电压（例如：4.2V）
    float voltageDivRatio = 2.0; // 分压比，用于电池电压检测
    bool nextBootSwap = false;   // 分压比，用于电池电压检测

    // 当前电压值
    float currentVoltage = 0.0;

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
        if (nextBootSwap)
        {
            switch_to_another_partition();
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

    void switch_to_another_partition()
    {
        // 获取当前正在运行的分区
        const esp_partition_t *running_partition = esp_ota_get_running_partition();
        const esp_partition_t *next_partition = NULL;

        // 判断当前是哪个应用分区，选择下一个分区
        if (strcmp(running_partition->label, "app0") == 0)
        {
            next_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, NULL);
        }
        else
        {
            next_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
        }

        if (next_partition == NULL)
        {
            DEBUG_PRINTLN("Next partition not found!");
            return;
        }

        // 获取 otadata 分区
        const esp_partition_t *otadata_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_OTA, NULL);
        if (otadata_partition == NULL)
        {
            DEBUG_PRINTLN("OTADATA partition not found!");
            return;
        }

        // 设置下次启动的分区
        esp_ota_set_boot_partition(next_partition);

        DEBUG_PRINTF("Switching to partition: %s\n", next_partition->label);
        WiFi.disconnect(); // 确保 Wi-Fi 处于断开状态
        esp_wifi_stop();   // 停止 Wi-Fi 模块
        delay(1000);
        // 重启设备使更改生效
        esp_wifi_stop();   // 停止 Wi-Fi 模块
        esp_wifi_deinit(); // 去初始化 Wi-Fi 堆栈
        esp_sleep_enable_timer_wakeup(1 * 1000000);
        esp_deep_sleep_start();
    }

    void startDeepSeelp(bool immediate)
    {
        if (immediate)
        {
            DEBUG_PRINTLN("Entering deep sleep...");
            int gpioPinsDown[] = {4, 27};
            pull_up_down(gpioPinsDown, sizeof(gpioPinsDown) / sizeof(gpioPinsDown[0]), false, true);
            int gpioPinsup[] = {26, 25};
            pull_up_down(gpioPinsup, sizeof(gpioPinsup) / sizeof(gpioPinsup[0]), true, false);
            WiFi.disconnect();
            WiFi.mode(WIFI_OFF);
            delay(2000); // wati gpio level restore ...
#ifndef ARDUINO_ARCH_ESP32C3
            ESP_ERROR_CHECK(esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0));
            // ESP_ERROR_CHECK(esp_sleep_enable_ext1_wakeup(1ULL << 0, ESP_EXT1_WAKEUP_ALL_LOW));
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
        esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
        boot.add(F(wakeup_reason == ESP_SLEEP_WAKEUP_UNDEFINED ? "reset" : wakeup_reason == ESP_SLEEP_WAKEUP_EXT0 ? "sleep"
                                                                                                                  : "other"));
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
        top["Switch Boot Partition"] = false;
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
        configComplete &= getJsonValue(top["Switch Boot Partition"], nextBootSwap);

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
};
#endif

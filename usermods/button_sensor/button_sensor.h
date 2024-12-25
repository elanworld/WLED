#include "wled.h"

volatile bool sensorTriggered = false; // 用于标记是否检测到震动

void IRAM_ATTR handleSensorInterrupt()
{
    sensorTriggered = true; // 设置传感器触发标志
}

class ButtonSensorUsermod : public Usermod
{
private:
    int sensorPin = 25; // 设置震动传感器连接的GPIO引脚
    unsigned long lastTriggerdTime = 0; 
    unsigned long interval = 300;   
public:
    void setup() override
    {
        pinMode(sensorPin, INPUT_PULLDOWN); 

        // 设置GPIO引脚的中断，任何引脚电平变化都会触发中断
        attachInterrupt(digitalPinToInterrupt(sensorPin), handleSensorInterrupt, CHANGE);
        DEBUG_PRINTLN("Button Sensor Usermod Initialized");
    }

    void loop() override
    {
        unsigned long currentMillis = millis();

        if (currentMillis - lastTriggerdTime >= interval)
        {
            lastTriggerdTime = currentMillis;
            if (sensorTriggered)
            {
                sensorTriggered = false;
                ++effectCurrent %= strip.getModeCount();
                stateChanged = true;
                colorUpdated(CALL_MODE_BUTTON);
            }
        }
    }
    // 可选：如果需要将模块配置添加到 WLED 配置中
    void addToConfig(JsonObject &obj) override
    {
    }

    bool readFromConfig(JsonObject &obj) override
    {
        return true;
    }
};
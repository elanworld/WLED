#include "wled.h"

volatile bool sensorTriggered = false; // 用于标记是否检测到震动

void IRAM_ATTR handleSensorInterrupt()
{
    sensorTriggered = true; // 设置传感器触发标志
}

class ButtonSensorUsermod : public Usermod
{
private:
    bool enable = false;
    int sensorPin = 25; // 设置震动传感器连接的GPIO引脚
    bool pullUp = true;  // 设置震动传感器连接的GPIO引脚
    unsigned long lastTriggerdTime = 0;
    unsigned long interval = 300;

public:
    void setup() override
    {
        if (!enable)
        {
            return;
        }
        if (pinManager.allocatePin(sensorPin, false, PinOwner::Button))
        {
            // 设置GPIO引脚的中断，任何引脚电平变化都会触发中断
            attachInterrupt(digitalPinToInterrupt(sensorPin), handleSensorInterrupt, CHANGE);
            DEBUG_PRINTLN("Button Sensor Usermod Initialized");
            pinMode(sensorPin, pullUp ? INPUT_PULLDOWN : INPUT_PULLUP);
        }
        else
        {
            DEBUG_PRINTLN("Button Sensor Usermod allocate Pin fail");
        }
    }

    void loop() override
    {
        if (!enable)
        {
            return;
        }
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
    // 在config中添加新选项
    virtual void addToConfig(JsonObject &root)
    {
        JsonObject top = root.createNestedObject("Shake Sensor");
        top["enable"] = enable;
        top["sensor pin"] = sensorPin;
        top["pull up"] = pullUp;
    }

    // 从config读取设置
    virtual bool readFromConfig(JsonObject &root)
    {
        JsonObject top = root["Shake Sensor"];
        bool configComplete = !top.isNull();
        configComplete &= getJsonValue(top["enable"], enable);
        configComplete &= getJsonValue(top["Sensor Pin"], sensorPin);
        configComplete &= getJsonValue(top["pull up"], pullUp);
        return configComplete;
    }
};
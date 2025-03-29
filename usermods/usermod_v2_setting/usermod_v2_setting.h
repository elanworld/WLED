#pragma once

#include "wled.h"

// update init values
class SettingUsermod : public Usermod
{
private:
    bool reinit = true;

public:
    void setup()
    {
        if (!reinit)
        {
            return;
        }

#ifdef NTPSERVERNAME
        strncpy(ntpServerName, NTPSERVERNAME, sizeof(ntpServerName) - 1);
        ntpServerName[sizeof(ntpServerName) - 1] = '\0'; // 确保字符串以 null 结尾
#endif
#ifdef TIMEZONE
        currentTimezone = TIMEZONE;
#endif
        reinit = false;
        serializeConfig();
    }

    void loop()
    {
    }

    // 在配置中添加电压相关配置
    void addToConfig(JsonObject &root)
    {
        JsonObject top = root.createNestedObject("Setting");
        top["reinit"] = reinit;
    }

    // 从配置中读取电压相关配置
    bool readFromConfig(JsonObject &root)
    {
        JsonObject top = root["Setting"];
        bool configComplete = !top.isNull();
        configComplete &= getJsonValue(top["reinit"], reinit);
        return configComplete;
    }
};

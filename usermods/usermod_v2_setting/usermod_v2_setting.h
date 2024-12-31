#pragma once

#include "wled.h"

// update init values
class SettingUsermod : public Usermod
{
private:
public:
    void setup()
    {
#ifdef NTPSERVERNAME
        if (strcmp(NTPSERVERNAME, "0.wled.pool.ntp.org") == 0)
        {
            strncpy(ntpServerName, NTPSERVERNAME, sizeof(ntpServerName) - 1);
            ntpServerName[sizeof(ntpServerName) - 1] = '\0'; // 确保字符串以 null 结尾
        }
#endif
#ifdef TIMEZONE
        if (currentTimezone = 0)
        {
            currentTimezone = TIMEZONE;
        }
#endif
    }

    void loop()
    {
    }
};

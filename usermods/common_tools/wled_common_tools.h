#ifndef WLED_COMMON_TOOLS
#define WLED_COMMON_TOOLS
#include "wled.h"

int findEffectIndex(String effect)
{
    char lineBuffer[128];
    for (int i = 0; i < strip.getModeCount(); i++)
    {
        strncpy_P(lineBuffer, strip.getModeData(i), 127);
        if (lineBuffer[0] != 0)
        {
            char *dataPtr = strchr(lineBuffer, '@');
            if (dataPtr)
                *dataPtr = 0; // terminate mode data after name
        }
        if (strcmp_P(effect.c_str(), lineBuffer) == 0)
        {
            DEBUG_PRINTLN("found effect " + effect + " at index " + String(i));
            return i;
        }
    }
    return -1;
}

void setRealtimeOverride(size_t status)
{
    realtimeOverride = status;
    if (realtimeOverride > 2)
        realtimeOverride = REALTIME_OVERRIDE_ALWAYS;
    if (realtimeMode && useMainSegmentOnly)
    {
        strip.getMainSegment().freeze = !realtimeOverride;
    }
}

String getState()
{
#ifdef WLED_USE_DYNAMIC_JSON
    DynamicJsonDocument doc(JSON_BUFFER_SIZE);
#else
    if (!requestJSONBufferLock(15))
        return "{}";
#endif
    if (bri == 0)
    {
        doc["state"] = "OFF";
    }
    else
    {
        doc["state"] = "ON";
    }
    doc["brightness"] = bri;
    doc["color_mode"] = "rgb";
    JsonObject color = doc.createNestedObject("color");
    color["r"] = col[0];
    color["g"] = col[1];
    color["b"] = col[2];
    char lineBuffer[128];
    strncpy_P(lineBuffer, strip.getModeData(effectCurrent), 127);
    if (lineBuffer[0] != 0)
    {
        char *dataPtr = strchr(lineBuffer, '@');
        if (dataPtr)
            *dataPtr = 0; // terminate mode data after name
        doc["effect"] = lineBuffer;
    }
    doc["bleOpen"] = bleOpen;
    String payload;
    serializeJson(doc, payload);
    releaseJSONBufferLock();
    return payload;
}

void setState(String payloadStr, const char *topic)
{
    bool updated = false;
    // homeassistant command
    if (strcmp_P(topic, PSTR("/command")) == 0)
    {
        if (payloadStr.c_str()[0] == '{')
        {
#ifdef WLED_USE_DYNAMIC_JSON
            DynamicJsonDocument doc(JSON_BUFFER_SIZE);
#else
            if (!requestJSONBufferLock(16))
                return;
#endif
            deserializeJson(doc, payloadStr);
            JsonObject commandJson = doc.as<JsonObject>();
            // update color
            if (commandJson.containsKey("color"))
            {
                if (commandJson["color"].containsKey("r") &&
                    commandJson["color"].containsKey("g") &&
                    commandJson["color"].containsKey("b"))
                {
                    col[0] = commandJson["color"]["r"];
                    col[1] = commandJson["color"]["g"];
                    col[2] = commandJson["color"]["b"];
                    setRealtimeOverride(REALTIME_OVERRIDE_ALWAYS);
                    colorUpdated(CALL_MODE_DIRECT_CHANGE);
                    updated = true;
                }
            }
            // update fx
            if (commandJson.containsKey("effect"))
            {
                String fx = commandJson["effect"].as<String>();
                int effectIndex = findEffectIndex(fx);
                if (effectIndex != -1)
                {
                    for (uint8_t i = 0; i < strip.getSegmentsNum(); i++)
                    {
                        strip.setMode(i, effectIndex);
                        stateChanged = true;
                    }
                    setRealtimeOverride(2);
                    updated = true;
                }
            }
            // update bright
            if (commandJson.containsKey("state") && strcmp_P(commandJson["state"], "OFF") == 0)
            {
                if (bri != 0)
                {
                    briLast = bri;
                    bri = 0;
                    updated = true;
                }
            }
            else if (commandJson.containsKey("state") && strcmp_P(commandJson["state"], "ON") == 0)
            {
                if (commandJson.containsKey("brightness") && commandJson["brightness"] > 0)
                {
                    briLast = bri;
                    bri = commandJson["brightness"];
                }
                else
                {
                    if (briLast != 0)
                    {
                        int temp = briLast;
                        briLast = bri;
                        bri = temp;
                    }
                    else
                    {
                        briLast = bri;
                        bri = 128;
                    }
                }
                updated = true;
            }
            else if (commandJson.containsKey("brightness"))
            {
                if (commandJson["brightness"] > 0)
                {
                    briLast = bri;
                    bri = commandJson["brightness"];
                    updated = true;
                }
            }
            // bleSetting
            if (commandJson.containsKey("bleOpen"))
            {
                bool bleOpenResut = commandJson["bleOpen"].as<boolean>();
                if (bleOpenResut != bleOpen)
                {
                    bleOpen = bleOpenResut;
                    doSerializeConfig = true;
                }
            }
            if (commandJson.containsKey("restart"))
            {
                bool restart = commandJson["restart"].as<boolean>();
                if (restart)
                {
                    ESP.restart();
                }
            }
            releaseJSONBufferLock();
        }
    }

    // homeassistant override switch command
    if (strcmp_P(topic, PSTR("_override/command")) == 0)
    {
        if (strcmp_P(payloadStr.c_str(), PSTR("ON")) == 0)
        {
            setRealtimeOverride(2);
            updated = true;
        }
        if (strcmp_P(payloadStr.c_str(), PSTR("OFF")) == 0)
        {
            setRealtimeOverride(0);
            updated = true;
        }
    }

    if (updated)
    {
        stateUpdated(CALL_MODE_DIRECT_CHANGE);
    }
    DEBUG_PRINTLN("update state done");
}

#endif
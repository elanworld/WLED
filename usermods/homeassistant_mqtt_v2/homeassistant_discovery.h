#include "wled.h"
#ifdef ESP8266
std::vector<int> indices = {0, 1, 2, 3, 5, 8, 10, 26, 29, 94};
#else
std::vector<int> indices = {};
#endif

void addFxsToJsonArray(JsonArray &jsonArray, const std::vector<int> &indices)
{
  jsonArray.clear();
  if (indices.empty())
  {
    for (int i = 0; i < strip.getModeCount(); i++)
    {
      jsonArray.add(String(strip.getModeData(i)));
    }
  }
  else
  {
    for (int i = 0; i < indices.size(); i++)
    {
      int index = indices[i];
  DEBUG_PRINTLN(i);
      if (index >= 0 && index < strip.getModeCount())
      {
  DEBUG_PRINTLN(i);
        jsonArray.add(String(strip.getModeData(index)));
  DEBUG_PRINTLN(String(strip.getModeData(index)));
      }
    }
  }
}

int findEffectIndex(String effect)
{
  for (int i = 0; i < strip.getModeCount(); i++)
  {
    DEBUG_PRINTLN("checking effect " + String(strip.getModeData(i)));
    if (strcmp_P(effect.c_str(), strip.getModeData(i)) == 0)
    {
      DEBUG_PRINTLN("found effect " + effect + " at index " + String(i));
      return i;
    }
  }
  return -1;
}

void setDeviceAttr(JsonObject &device)
{
  device["ids"] = escapedMac;
  if (strcmp_P(serverDescription, PSTR("WLED")) == 0)
  {
    char bufn[15];
    device["name"] = strcat(strcpy(bufn, "WLED "), deviceUni);
  }
  else
  {
    device["name"] = serverDescription;
  }
  device["mf"] = "WLED";
  device["sw"] = versionString;
#ifdef ARDUINO_ARCH_ESP32
  device["mdl"] = "esp32";
#else
  device["mdl"] = "esp8266";
#endif
}

void fakeApi(String api)
{
  String apireq = "win&";
  apireq += api;
  DEBUG_PRINTLN("fake api from json:" + apireq);
  handleSet(nullptr, apireq);
}

void sendState()
{
#ifdef WLED_USE_DYNAMIC_JSON
  DynamicJsonDocument doc(JSON_BUFFER_SIZE);
#else
  if (!requestJSONBufferLock(15))
    return;
#endif
  char subuf[38];
  strlcpy(subuf, mqttDeviceTopic, 33);
  strcat_P(subuf, PSTR("/state"));
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
  doc["effect"] = String(strip.getModeData(effectCurrent));
  String payload;
  serializeJson(doc, payload);
  DEBUG_PRINTLN(subuf);
  DEBUG_PRINTLN(payload);
  mqtt->publish(subuf, 0, false, payload.c_str()); // do not retain message
  // override switch
  payload.clear();
  if (realtimeOverride == REALTIME_OVERRIDE_NONE)
  {
    payload = "OFF";
  }
  else
  {
    payload = "ON";
  }
  memset(subuf, 0, sizeof subuf);
  strlcpy(subuf, mqttDeviceTopic, 33);
  strcat_P(subuf, PSTR("_override/state"));
  DEBUG_PRINTLN(subuf);
  DEBUG_PRINTLN(payload);
  mqtt->publish(subuf, 0, false, payload.c_str()); // do not retain message

  releaseJSONBufferLock();
}

void setState(String payloadStr, char *payload, char *topic)
{
  bool updated = false;

  // homeassistant command
  if (strcmp_P(topic, PSTR("/command")) == 0)
  {
    if (payload[0] == '{')
    {
#ifdef WLED_USE_DYNAMIC_JSON
      DynamicJsonDocument doc(JSON_BUFFER_SIZE);
#else
      if (!requestJSONBufferLock(15))
        return;
#endif
      deserializeJson(doc, payloadStr);
      JsonObject commandJson = doc.as<JsonObject>();
      // update bright
      if (commandJson.containsKey("state"))
      {
        if (commandJson.containsKey("color") || commandJson.containsKey("effect"))
        {
          if (realtimeOverride == 0)
          {
            // set realtimeOverride until reboot
            fakeApi("LO=2");
            updated = true;
          }
        }

        if (strcmp_P(commandJson["state"], "OFF") == 0)
        {
          if (bri != 0)
          {
            briLast = bri;
            bri = 0;
          }
          updated = true;
        }
        else if (strcmp_P(commandJson["state"], "ON") == 0)
        {
          if (commandJson.containsKey("brightness"))
          {
            if (commandJson["brightness"].is<int>())
            {
              if (commandJson["brightness"] > 0)
              {
                bri = commandJson["brightness"];
                updated = true;
              }
            }
          }
          else
          {
            bri = briLast;
            updated = true;
          }
        }
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
            String apireq = "FX=";
            apireq += effectIndex;
            fakeApi(apireq);
            updated = true;
          }
        }
      }
    }
  }

  // homeassistant override switch command
  if (strcmp_P(topic, PSTR("_override/command")) == 0)
  {
    DEBUG_PRINTLN("override command topic");
    if (strcmp_P(payloadStr.c_str(), PSTR("ON")) == 0)
    {
      DEBUG_PRINTLN("override command on");
      fakeApi("LO=2");
      publishMqtt();
    }
    if (strcmp_P(payloadStr.c_str(), PSTR("OFF")) == 0)
    {
      fakeApi("LO=0");
      publishMqtt();
    }
  }
  if (updated)
  {
    stateUpdated(CALL_MODE_DIRECT_CHANGE);
  }

  releaseJSONBufferLock();
  DEBUG_PRINTLN("update state done");
}

void sendHADiscoveryMQTT()
{
  DEBUG_PRINTLN(0);
#if ARDUINO_ARCH_ESP32 || LWIP_VERSION_MAJOR >= 1
  if (mqtt == nullptr || !mqtt->connected())
    return;
  char bufcom[45];
#ifdef WLED_USE_DYNAMIC_JSON
  DynamicJsonDocument doc(JSON_BUFFER_SIZE);
#else
  if (!requestJSONBufferLock(15))
    return;
#endif
  doc["schema"] = "json";
  doc["brightness"] = true;
  doc["color_mode"] = true;
  JsonArray modes = doc.createNestedArray("supported_color_modes");
  modes.add("rgb");
  doc["effect"] = true;
  memset(bufcom, 0, sizeof bufcom);
  if (strcmp_P(serverDescription, PSTR("WLED")) == 0)
  {
    doc["name"] = strcat(strcat(strcat(strcpy(bufcom, serverDescription), " "), deviceUni), " light");
  }
  else
  {
    doc["name"] = strcat(strcpy(bufcom, serverDescription), " light");
  }
  memset(bufcom, 0, sizeof bufcom);
  doc["avty_t"] = strcat(strcpy(bufcom, mqttDeviceTopic), "/status");
  memset(bufcom, 0, sizeof bufcom);
  doc["stat_t"] = strcat(strcpy(bufcom, mqttDeviceTopic), "/state");
  memset(bufcom, 0, sizeof bufcom);
  doc["cmd_t"] = strcat(strcpy(bufcom, mqttDeviceTopic), "/command");
  memset(bufcom, 0, sizeof bufcom);
  doc["uniq_id"] = strcat(strcpy(bufcom, "wled_light_"), escapedMac.c_str());
  JsonObject dev = doc.createNestedObject("dev");
  DEBUG_PRINTLN(1);
  setDeviceAttr(dev);
  DEBUG_PRINTLN(2);
  // add fx_list
  JsonArray fxs = doc.createNestedArray("fx_list");
  addFxsToJsonArray(fxs, indices);
  DEBUG_PRINTLN("HA Discovery Sending >>");
  char pubt[25 + 12 + 8];
  strcpy(pubt, "homeassistant/light/");
  strcat(pubt, mqttClientID);
  strcat(pubt, "/config");
  String payload;
  DEBUG_PRINTLN("sending discovery config");
  serializeJson(doc, payload);
  DEBUG_PRINTLN(payload);
  mqtt->publish(pubt, 0, true, payload.c_str());
  memset(bufcom, 0, sizeof bufcom);
  strlcpy(bufcom, mqttDeviceTopic, 33);
  strcat_P(bufcom, PSTR("/command"));
  mqtt->subscribe(bufcom, 0);

  // ip address
  doc.clear();
  memset(bufcom, 0, sizeof bufcom);
  doc["uniq_id"] = strcat(strcpy(bufcom, "wled_ip_"), escapedMac.c_str());
  memset(bufcom, 0, sizeof bufcom);
  if (strcmp_P(serverDescription, PSTR("WLED")) == 0)
  {
    doc["name"] = strcat(strcat(strcat(strcpy(bufcom, serverDescription), " "), deviceUni), " ip");
  }
  else
  {
    doc["name"] = strcat(strcpy(bufcom, serverDescription), " ip");
  }
  memset(bufcom, 0, sizeof bufcom);
  doc["stat_t"] = strcat(strcpy(bufcom, mqttDeviceTopic), "_ip/state");
  JsonObject device = doc.createNestedObject("dev");
  setDeviceAttr(device);
  payload.clear();
  serializeJson(doc, payload);
  memset(bufcom, 0, sizeof bufcom);
  mqtt->publish(
      strcat(strcat(strcpy(bufcom, "homeassistant/sensor/"), mqttClientID),
             "_ip/config"),
      0, true, payload.c_str());
  DEBUG_PRINTLN(payload);
  memset(bufcom, 0, sizeof bufcom);
  mqtt->publish(strcat(strcpy(bufcom, mqttDeviceTopic), "_ip/state"), 0, false,
                Network.localIP().toString().c_str());
  // override switch
  memset(bufcom, 0, sizeof bufcom);
  doc["uniq_id"] = strcat(strcpy(bufcom, "wled_override"), escapedMac.c_str());
  memset(bufcom, 0, sizeof bufcom);
  if (strcmp_P(serverDescription, PSTR("WLED")) == 0)
  {
    doc["name"] = strcat(strcat(strcat(strcpy(bufcom, serverDescription), " "), deviceUni), " override");
  }
  else
  {
    doc["name"] = strcat(strcpy(bufcom, serverDescription), " override");
  }
  memset(bufcom, 0, sizeof bufcom);
  doc["stat_t"] = strcat(strcpy(bufcom, mqttDeviceTopic), "_override/state");
  memset(bufcom, 0, sizeof bufcom);
  doc["cmd_t"] = strcat(strcpy(bufcom, mqttDeviceTopic), "_override/command");
  memset(bufcom, 0, sizeof bufcom);
  doc["avty_t"] = strcat(strcpy(bufcom, mqttDeviceTopic), "/status");
  payload.clear();
  serializeJson(doc, payload);
  DEBUG_PRINTLN(payload);
  memset(bufcom, 0, sizeof bufcom);
  mqtt->publish(
      strcat(strcat(strcpy(bufcom, "homeassistant/switch/"), mqttClientID),
             "_override/config"),
      0, true, payload.c_str());
  memset(bufcom, 0, sizeof bufcom);
  strcat(strcpy(bufcom, mqttDeviceTopic), "_override/command");
  // send online subscribe to command topic
  DEBUG_PRINTLN(doc["avty_t"].as<String>().c_str());
  mqtt->publish(doc["avty_t"].as<String>().c_str(), 0, true, "online"); // retain message for a LWT
  mqtt->subscribe(bufcom, 0);
  releaseJSONBufferLock();
#endif
}

class UsermodHomeAssistantDiscovery : public Usermod
{
public:
  virtual void setup()
  {
    // implementation here
  }
  virtual void loop()
  {
    // implementation here
  }

  inline void onMqttConnect(bool sessionPresent)
  {
    sendHADiscoveryMQTT();
  }
  inline bool onMqttMessage(char *topic, char *payload)
  {
    setState(payload, payload, topic);
    return true;
  }
  inline void publishMqtt()
  {
    sendState();
  }
};

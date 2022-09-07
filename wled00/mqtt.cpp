#include "wled.h"

/*
 * MQTT communication protocol for home automation
 */

#ifdef WLED_ENABLE_MQTT
#define MQTT_KEEP_ALIVE_TIME 60 // contact the MQTT broker every 60 seconds

JsonArray getFxs(DynamicJsonDocument doc)
{
  JsonArray fxs = doc.createNestedArray("fx_list");
  uint16_t jmnlen = strlen_P(JSON_mode_names);
  uint16_t nameStart = 0, nameEnd = 0;
  int i = 0;
  bool isNameStart = true;
  for (uint16_t j = 0; j < jmnlen; j++)
  {
    if (pgm_read_byte(JSON_mode_names + j) == '\"' || j == jmnlen - 1)
    {
      if (isNameStart)
      {
        nameStart = j + 1;
      }
      else
      {
        nameEnd = j;
        char mdn[56];
        uint16_t namelen = nameEnd - nameStart;
        strncpy_P(mdn, JSON_mode_names + nameStart, namelen);
        mdn[namelen] = 0;
        fxs.add(mdn);
        i++;
      }
      isNameStart = !isNameStart;
    }
  }
  return fxs;
}
JsonObject getDevice(DynamicJsonDocument doc)
{
  JsonObject device = doc.createNestedObject("dev");
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
  return device;
}

void fakeApi(String api)
{
  String apireq = "win&";
  apireq += api;
  DEBUG_PRINTLN("fake api from json");
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
  JsonArray fxs = getFxs(doc);
  if (fxs.size() > effectCurrent)
  {
    doc["effect"] = fxs.getElement(effectCurrent);
  }
  String payload;
  serializeJson(doc, payload);
  mqtt->publish(subuf, 0, false, payload.c_str()); // do not retain message
  releaseJSONBufferLock();
}

void setState(String payloadStr, char *payload, char *topic)
{
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
          }
        }

        if (strcmp_P(commandJson["state"], "OFF") == 0)
        {
          briLast = bri;
          bri = 0;
          wifi_set_sleep_type(LIGHT_SLEEP_T);
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
              }
            }
          }
          else
          {
            bri = briLast;
          }
        }
        stateUpdated(CALL_MODE_DIRECT_CHANGE);
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
          }
        }
        // update fx
        if (commandJson.containsKey("effect"))
        {
          String fx = commandJson["effect"].as<String>();
          JsonArray fxs = getFxs(doc);
          for (size_t i = 0; i < fxs.size(); i++)
          {
            if (fxs.getElement(i).as<String>().compareTo(fx) == 0)
            {
              String apireq = "FX=";
              apireq += i;
              fakeApi(apireq);
              break;
            }
          }
        }
      }
    }

    releaseJSONBufferLock();
    DEBUG_PRINTLN("update state done");
  }
}

void sendHADiscoveryMQTT()
{
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
  doc["dev"] = getDevice(doc);
  // add fx_list
  doc["fx_list"] = getFxs(doc);

  DEBUG_PRINT("HA Discovery Sending >>");
  char pubt[25 + 12 + 8];
  strcpy(pubt, "homeassistant/light/");
  strcat(pubt, mqttClientID);
  strcat(pubt, "/config");
  String payload;
  serializeJson(doc, payload);
  DEBUG_PRINTLN(payload);
  mqtt->publish(pubt, 0, true, payload.c_str());

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
  doc["dev"] = getDevice(doc);
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

  releaseJSONBufferLock();
#endif
}

void parseMQTTBriPayload(char *payload)
{
  if (strstr(payload, "ON") || strstr(payload, "on") ||
      strstr(payload, "true"))
  {
    bri = briLast;
    stateUpdated(1);
  }
  else if (strstr(payload, "T") || strstr(payload, "t"))
  {
    toggleOnOff();
    stateUpdated(1);
  }
  else
  {
    uint8_t in = strtoul(payload, NULL, 10);
    if (in == 0 && bri > 0)
      briLast = bri;
    bri = in;
    stateUpdated(CALL_MODE_DIRECT_CHANGE);
  }
}

void onMqttConnect(bool sessionPresent)
{
  //(re)subscribe to required topics
  char subuf[38];

  if (mqttDeviceTopic[0] != 0)
  {
    strlcpy(subuf, mqttDeviceTopic, 33);
    mqtt->subscribe(subuf, 0);
    strcat_P(subuf, PSTR("/col"));
    mqtt->subscribe(subuf, 0);
    strlcpy(subuf, mqttDeviceTopic, 33);
    strcat_P(subuf, PSTR("/api"));
    strlcpy(subuf, mqttDeviceTopic, 33);
    strcat_P(subuf, PSTR("/command"));
    mqtt->subscribe(subuf, 0);
    sendHADiscoveryMQTT();
  }

  if (mqttGroupTopic[0] != 0)
  {
    strlcpy(subuf, mqttGroupTopic, 33);
    mqtt->subscribe(subuf, 0);
    strcat_P(subuf, PSTR("/col"));
    mqtt->subscribe(subuf, 0);
    strlcpy(subuf, mqttGroupTopic, 33);
    strcat_P(subuf, PSTR("/api"));
    mqtt->subscribe(subuf, 0);
  }

  usermods.onMqttConnect(sessionPresent);

  doPublishMqtt = true;
  DEBUG_PRINTLN(F("MQTT ready"));
}

void onMqttMessage(char *topic, char *payload,
                   AsyncMqttClientMessageProperties properties, size_t len,
                   size_t index, size_t total)
{
  DEBUG_PRINT(F("MQTT msg: "));
  DEBUG_PRINTLN(topic);

  // paranoia check to avoid npe if no payload
  if (payload == nullptr)
  {
    DEBUG_PRINTLN(F("no payload -> leave"));
    return;
  }
  // make a copy of the payload to 0-terminate it
  char *payloadStr = new char[len + 1];
  if (payloadStr == nullptr)
    return; // no mem
  strncpy(payloadStr, payload, len);
  payloadStr[len] = '\0';
  DEBUG_PRINTLN(payloadStr);

  size_t topicPrefixLen = strlen(mqttDeviceTopic);
  if (strncmp(topic, mqttDeviceTopic, topicPrefixLen) == 0)
  {
    topic += topicPrefixLen;
  }
  else
  {
    topicPrefixLen = strlen(mqttGroupTopic);
    if (strncmp(topic, mqttGroupTopic, topicPrefixLen) == 0)
    {
      topic += topicPrefixLen;
    }
    else
    {
      // Non-Wled Topic used here. Probably a usermod subscribed to this topic.
      usermods.onMqttMessage(topic, payloadStr);
      delete[] payloadStr;
      return;
    }
  }

  // homeassistant command
  setState(payloadStr, payload, topic);

  // Prefix is stripped from the topic at this point

  if (strcmp_P(topic, PSTR("/col")) == 0)
  {
    colorFromDecOrHexString(col, (char *)payloadStr);
    colorUpdated(CALL_MODE_DIRECT_CHANGE);
  }
  else if (strcmp_P(topic, PSTR("/api")) == 0)
  {
    if (payload[0] == '{')
    { // JSON API
#ifdef WLED_USE_DYNAMIC_JSON
      DynamicJsonDocument doc(JSON_BUFFER_SIZE);
#else
      if (!requestJSONBufferLock(15))
        return;
#endif
      deserializeJson(doc, payloadStr);
      deserializeState(doc.as<JsonObject>());
      releaseJSONBufferLock();
    }
    else
    { // HTTP API
      String apireq = "win&";
      apireq += (char *)payloadStr;
      handleSet(nullptr, apireq);
    }
  }
  else if (strlen(topic) != 0)
  {
    // non standard topic, check with usermods
    usermods.onMqttMessage(topic, payloadStr);
  }
  else
  {
    // topmost topic (just wled/MAC)
    parseMQTTBriPayload(payloadStr);
  }

  delete[] payloadStr;
}

void publishMqtt()
{
  doPublishMqtt = false;
  if (!WLED_MQTT_CONNECTED)
    return;
  DEBUG_PRINTLN(F("Publish MQTT"));

  char s[10];
  char subuf[38];

  sprintf_P(s, PSTR("%u"), bri);
  strlcpy(subuf, mqttDeviceTopic, 33);
  strcat_P(subuf, PSTR("/g"));
  mqtt->publish(subuf, 0, true, s); // retain message

  sprintf_P(s, PSTR("#%06X"),
            (col[3] << 24) | (col[0] << 16) | (col[1] << 8) | (col[2]));
  strlcpy(subuf, mqttDeviceTopic, 33);
  strcat_P(subuf, PSTR("/c"));
  mqtt->publish(subuf, 0, true, s); // retain message

  strlcpy(subuf, mqttDeviceTopic, 33);
  strcat_P(subuf, PSTR("/status"));
  mqtt->publish(subuf, 0, true, "online"); // retain message for a LWT

  char apires[1024]; // allocating 1024 bytes from stack can be risky
  XML_response(nullptr, apires);
  strlcpy(subuf, mqttDeviceTopic, 33);
  strcat_P(subuf, PSTR("/v"));
  mqtt->publish(subuf, 0, false, apires); // do not retain message

  sendState();
}

// HA autodiscovery was removed in favor of the native integration in HA
// v0.102.0

bool initMqtt()
{
  if (!mqttEnabled || mqttServer[0] == 0 || !WLED_CONNECTED)
    return false;

  if (mqtt == nullptr)
  {
    mqtt = new AsyncMqttClient();
    mqtt->onMessage(onMqttMessage);
    mqtt->onConnect(onMqttConnect);
  }
  if (mqtt->connected())
    return true;

  DEBUG_PRINTLN(F("Reconnecting MQTT"));
  IPAddress mqttIP;
  if (mqttIP.fromString(mqttServer)) // see if server is IP or domain
  {
    mqtt->setServer(mqttIP, mqttPort);
  }
  else
  {
    mqtt->setServer(mqttServer, mqttPort);
  }
  mqtt->setClientId(mqttClientID);
  if (mqttUser[0] && mqttPass[0])
    mqtt->setCredentials(mqttUser, mqttPass);

  strlcpy(mqttStatusTopic, mqttDeviceTopic, 33);
  strcat_P(mqttStatusTopic, PSTR("/status"));
  mqtt->setWill(mqttStatusTopic, 0, true, "offline"); // LWT message
  mqtt->setKeepAlive(MQTT_KEEP_ALIVE_TIME);
  mqtt->connect();
  return true;
}

#else
bool initMqtt() { return false; }
void publishMqtt() {}
#endif

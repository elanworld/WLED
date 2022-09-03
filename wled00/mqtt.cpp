#include "wled.h"

/*
 * MQTT communication protocol for home automation
 */

#ifdef WLED_ENABLE_MQTT
#define MQTT_KEEP_ALIVE_TIME 60  // contact the MQTT broker every 60 seconds

const char HA_static_JSON[] PROGMEM =
    R"=====(,"bri_val_tpl":"{{value}}","rgb_cmd_tpl":"{{'#%02x%02x%02x' | format(red, green, blue)}}","rgb_val_tpl":"{{value[1:3]|int(base=16)}},{{value[3:5]|int(base=16)}},{{value[5:7]|int(base=16)}}","qos":0,"opt":true,"pl_on":"ON","pl_off":"OFF","fx_val_tpl":"{{value}}","fx_list":[)=====";

void sendHADiscoveryMQTT() {
// TODO: With LwIP 1 the ESP loses MQTT connection and causes memory leak when
// sending discovery packet
#if ARDUINO_ARCH_ESP32 || LWIP_VERSION_MAJOR >= 1
  /*

  YYYY is device topic
  XXXX is device name

  Send out HA MQTT Discovery message on MQTT connect (~2.4kB):
  {
  "name": "XXXX",
  "stat_t":"YYYY/c",
  "cmd_t":"YYYY",
  "rgb_stat_t":"YYYY/c",
  "rgb_cmd_t":"YYYY/col",
  "bri_cmd_t":"YYYY",
  "bri_stat_t":"YYYY/g",
  "bri_val_tpl":"{{value}}",
  "rgb_cmd_tpl":"{{'#%02x%02x%02x' | format(red, green, blue)}}",
  "rgb_val_tpl":"{{value[1:3]|int(base=16)}},{{value[3:5]|int(base=16)}},{{value[5:7]|int(base=16)}}",
  "qos": 0,
  "opt":true,
  "pl_on": "ON",
  "pl_off": "OFF",
  "fx_cmd_t":"YYYY/api",
  "fx_stat_t":"YYYY/api",
  "fx_val_tpl":"{{value}}",
  "fx_list":[
  "[FX=00] Solid",
  "[FX=01] Blink",
  "[FX=02] ...",
  "[FX=79] Ripple"
  ]
  }

    */
  if (mqtt == nullptr || !mqtt->connected()) return;

  char bufs[36], bufc[38], bufa[40];

  strcpy(bufs, mqttDeviceTopic);
  strcpy(bufc, mqttDeviceTopic);
  strcpy(bufa, mqttDeviceTopic);

  strcat(bufs, "/state");
  strcat(bufc, "/command");
  strcat(bufa, "/status");

  StaticJsonDocument<JSON_OBJECT_SIZE(9) + 512> root;
  root["schema"] = "json";
  root["brightness"] = true;
  root["color_mode"] = true;
  JsonArray modes = root.createNestedArray("supported_color_modes");
  modes.add("rgb");
  root["effect"] = true;
  root["name"] = serverDescription;
  root["stat_t"] = bufs;
  root["cmd_t"] = bufc;
  root["availability_topic"] = bufa;
  root["unique_id"] = mqttDeviceTopic;
  JsonObject device = root.createNestedObject("device");
  device["identifiers"] = mqttDeviceTopic;
  device["name"] = mqttDeviceTopic;

  DEBUG_PRINT("HA Discovery Sending >>");

  char pubt[25 + 12 + 8];
  strcpy(pubt, "homeassistant/light/");
  strcat(pubt, mqttClientID);
  strcat(pubt, "/config");
  String payload;
  serializeJson(root, payload);
  bool success = mqtt->publish(pubt, 0, true, payload.c_str());
  DEBUG_PRINTLN(success);
#endif
}

void parseMQTTBriPayload(char* payload) {
  if (strstr(payload, "ON") || strstr(payload, "on") ||
      strstr(payload, "true")) {
    bri = briLast;
    stateUpdated(1);
  } else if (strstr(payload, "T") || strstr(payload, "t")) {
    toggleOnOff();
    stateUpdated(1);
  } else {
    uint8_t in = strtoul(payload, NULL, 10);
    if (in == 0 && bri > 0) briLast = bri;
    bri = in;
    stateUpdated(CALL_MODE_DIRECT_CHANGE);
  }
}

void onMqttConnect(bool sessionPresent) {
  //(re)subscribe to required topics
  char subuf[38];

  if (mqttDeviceTopic[0] != 0) {
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

  if (mqttGroupTopic[0] != 0) {
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

void onMqttMessage(char* topic, char* payload,
                   AsyncMqttClientMessageProperties properties, size_t len,
                   size_t index, size_t total) {
  DEBUG_PRINT(F("MQTT msg: "));
  DEBUG_PRINTLN(topic);

  // paranoia check to avoid npe if no payload
  if (payload == nullptr) {
    DEBUG_PRINTLN(F("no payload -> leave"));
    return;
  }
  // make a copy of the payload to 0-terminate it
  char* payloadStr = new char[len + 1];
  if (payloadStr == nullptr) return;  // no mem
  strncpy(payloadStr, payload, len);
  payloadStr[len] = '\0';
  DEBUG_PRINTLN(payloadStr);

  size_t topicPrefixLen = strlen(mqttDeviceTopic);
  if (strncmp(topic, mqttDeviceTopic, topicPrefixLen) == 0) {
    topic += topicPrefixLen;
  } else {
    topicPrefixLen = strlen(mqttGroupTopic);
    if (strncmp(topic, mqttGroupTopic, topicPrefixLen) == 0) {
      topic += topicPrefixLen;
    } else {
      // Non-Wled Topic used here. Probably a usermod subscribed to this topic.
      usermods.onMqttMessage(topic, payloadStr);
      delete[] payloadStr;
      return;
    }
  }

  // homeassistant command
  if (strcmp_P(topic, PSTR("/command")) == 0) {
    if (payload[0] == '{') {
#ifdef WLED_USE_DYNAMIC_JSON
      DynamicJsonDocument doc(JSON_BUFFER_SIZE);
#else
      if (!requestJSONBufferLock(15)) return;
#endif
      deserializeJson(doc, payloadStr);
      JsonObject commandJson = doc.as<JsonObject>();
      if (strcmp_P(commandJson["state"], "OFF") == 0) {
        briLast = bri;
        bri = 0;
      } else if (strcmp_P(commandJson["state"], "ON") == 0) {
        if (commandJson.containsKey("brightness")) {
          if (commandJson["brightness"].is<int>()) {
            if (commandJson["brightness"] > 0) {
              bri = commandJson["brightness"];
            }
          }
        } else {
          bri = briLast;
        }
      }
      stateUpdated(CALL_MODE_DIRECT_CHANGE);
      if (commandJson.containsKey("color")) {
        if (commandJson["color"].containsKey("r") &&
            commandJson["color"].containsKey("g") &&
            commandJson["color"].containsKey("b")) {
          col[0] = commandJson["color"]["r"];
          col[1] = commandJson["color"]["g"];
          col[2] = commandJson["color"]["b"];
          colorUpdated(CALL_MODE_DIRECT_CHANGE);
        }
      }
      releaseJSONBufferLock();
    }
  }

  // Prefix is stripped from the topic at this point

  if (strcmp_P(topic, PSTR("/col")) == 0) {
    colorFromDecOrHexString(col, (char*)payloadStr);
    colorUpdated(CALL_MODE_DIRECT_CHANGE);
  } else if (strcmp_P(topic, PSTR("/api")) == 0) {
    if (payload[0] == '{') {  // JSON API
#ifdef WLED_USE_DYNAMIC_JSON
      DynamicJsonDocument doc(JSON_BUFFER_SIZE);
#else
      if (!requestJSONBufferLock(15)) return;
#endif
      deserializeJson(doc, payloadStr);
      deserializeState(doc.as<JsonObject>());
      releaseJSONBufferLock();
    } else {  // HTTP API
      String apireq = "win&";
      apireq += (char*)payloadStr;
      handleSet(nullptr, apireq);
    }
  } else if (strlen(topic) != 0) {
    // non standard topic, check with usermods
    usermods.onMqttMessage(topic, payloadStr);
  } else {
    // topmost topic (just wled/MAC)
    parseMQTTBriPayload(payloadStr);
  }

  delete[] payloadStr;
}

void publishMqtt() {
  doPublishMqtt = false;
  if (!WLED_MQTT_CONNECTED) return;
  DEBUG_PRINTLN(F("Publish MQTT"));

  char s[10];
  char subuf[38];

  sprintf_P(s, PSTR("%u"), bri);
  strlcpy(subuf, mqttDeviceTopic, 33);
  strcat_P(subuf, PSTR("/g"));
  mqtt->publish(subuf, 0, true, s);  // retain message

  sprintf_P(s, PSTR("#%06X"),
            (col[3] << 24) | (col[0] << 16) | (col[1] << 8) | (col[2]));
  strlcpy(subuf, mqttDeviceTopic, 33);
  strcat_P(subuf, PSTR("/c"));
  mqtt->publish(subuf, 0, true, s);  // retain message

  strlcpy(subuf, mqttDeviceTopic, 33);
  strcat_P(subuf, PSTR("/status"));
  mqtt->publish(subuf, 0, true, "online");  // retain message for a LWT

  char apires[1024];  // allocating 1024 bytes from stack can be risky
  XML_response(nullptr, apires);
  strlcpy(subuf, mqttDeviceTopic, 33);
  strcat_P(subuf, PSTR("/v"));
  mqtt->publish(subuf, 0, false, apires);  // do not retain message

  char state[1024];
  strlcpy(subuf, mqttDeviceTopic, 33);
  strcat_P(subuf, PSTR("/state"));
  StaticJsonDocument<300> doc;
  if (bri == 0) {
    doc["state"] = "OFF";
  } else {
    doc["state"] = "ON";
  }
  doc["brightness"] = bri;
  doc["color_mode"] = "rgb";
  JsonObject color = doc.createNestedObject("color");
  color["r"] = col[0];
  color["g"] = col[1];
  color["b"] = col[2];
  String payload;
  serializeJson(doc, payload);
  mqtt->publish(subuf, 0, false, payload.c_str());  // do not retain message
}

// HA autodiscovery was removed in favor of the native integration in HA
// v0.102.0

bool initMqtt() {
  if (!mqttEnabled || mqttServer[0] == 0 || !WLED_CONNECTED) return false;

  if (mqtt == nullptr) {
    mqtt = new AsyncMqttClient();
    mqtt->onMessage(onMqttMessage);
    mqtt->onConnect(onMqttConnect);
  }
  if (mqtt->connected()) return true;

  DEBUG_PRINTLN(F("Reconnecting MQTT"));
  IPAddress mqttIP;
  if (mqttIP.fromString(mqttServer))  // see if server is IP or domain
  {
    mqtt->setServer(mqttIP, mqttPort);
  } else {
    mqtt->setServer(mqttServer, mqttPort);
  }
  mqtt->setClientId(mqttClientID);
  if (mqttUser[0] && mqttPass[0]) mqtt->setCredentials(mqttUser, mqttPass);

  strlcpy(mqttStatusTopic, mqttDeviceTopic, 33);
  strcat_P(mqttStatusTopic, PSTR("/status"));
  mqtt->setWill(mqttStatusTopic, 0, true, "offline");  // LWT message
  mqtt->setKeepAlive(MQTT_KEEP_ALIVE_TIME);
  mqtt->connect();
  return true;
}

#else
bool initMqtt() { return false; }
void publishMqtt() {}
#endif

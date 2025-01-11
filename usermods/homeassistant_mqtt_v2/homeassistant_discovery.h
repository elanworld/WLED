#include "../common_tools/wled_common_tools.h"
#include "wled.h"

#ifdef USERMOD_BATTERY
  #include "../usermods/Battery/usermod_v2_Battery.h"
#endif

#ifdef ESP8266
std::vector<int> indices = {
    FX_MODE_STATIC,       FX_MODE_BLINK,          FX_MODE_BREATH,
    FX_MODE_COLOR_WIPE,   FX_MODE_RANDOM_COLOR,   FX_MODE_SCAN,
    FX_MODE_RAINBOW,      FX_MODE_RAINBOW_CYCLE,  FX_MODE_BLINK_RAINBOW,
    FX_MODE_CHASE_RANDOM, FX_MODE_SINELON_RAINBOW};
#else
std::vector<int> indices = {};
#endif

void addFxsToJsonArray(JsonArray &jsonArray, const std::vector<int> &indices) {
  jsonArray.clear();
  char lineBuffer[128];
  if (indices.empty()) {
    for (int i = 0; i < strip.getModeCount(); i++) {
      strncpy_P(lineBuffer, strip.getModeData(i), 127);
      if (lineBuffer[0] != 0) {
        char *dataPtr = strchr(lineBuffer, '@');
        if (dataPtr) *dataPtr = 0;  // terminate mode data after name
        jsonArray.add(lineBuffer);
      }
    }
  } else {
    for (size_t i = 0; i < indices.size(); i++) {
      int index = indices[i];
      if (index >= 0 && index < strip.getModeCount()) {
        strncpy_P(lineBuffer, strip.getModeData(index), 127);
        if (lineBuffer[0] != 0) {
          char *dataPtr = strchr(lineBuffer, '@');
          if (dataPtr) *dataPtr = 0;  // terminate mode data after name
          jsonArray.add(lineBuffer);
        }
      }
    }
  }
}

void setDeviceAttr(JsonObject &device) {
  device["ids"] = escapedMac;
  if (strcmp_P(serverDescription, PSTR("WLED")) == 0) {
    char bufn[15];
    device["name"] = strcat(strcpy(bufn, "WLED "), escapedMac.c_str() + 6);
  } else {
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

void fakeApi(String api) {
  String apireq = "win&";
  apireq += api;
  DEBUG_PRINTLN("fake api from json:" + apireq);
  handleSet(nullptr, apireq);
}

class UsermodHomeAssistantDiscovery : public Usermod {
 private:
  UsermodBattery *usermodBattery;

  void sendHADiscoveryMQTT() {
#if ARDUINO_ARCH_ESP32 || LWIP_VERSION_MAJOR >= 1
    if (mqtt == nullptr || !mqtt->connected()) return;
    char bufcom[45];
#ifdef WLED_USE_DYNAMIC_JSON
    DynamicJsonDocument doc(JSON_BUFFER_SIZE);
#else
    if (!requestJSONBufferLock(15)) return;
#endif
    doc["schema"] = "json";
    doc["brightness"] = true;
    doc["color_mode"] = true;
    JsonArray modes = doc.createNestedArray("supported_color_modes");
    modes.add("rgb");
    doc["effect"] = true;
    memset(bufcom, 0, sizeof bufcom);
    if (strcmp_P(serverDescription, PSTR("WLED")) == 0) {
      doc["name"] =
          strcat(strcat(strcat(strcpy(bufcom, serverDescription), " "),
                        escapedMac.c_str() + 6),
                 " light");
    } else {
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
    setDeviceAttr(dev);
    // add fx_list
    JsonArray fxs = doc.createNestedArray("fx_list");
    addFxsToJsonArray(fxs, indices);
    DEBUG_PRINTLN("HA Discovery Sending >>");
    char pubt[25 + 12 + 8];
    strcpy(pubt, "homeassistant/light/");
    strcat(pubt, mqttClientID);
    strcat(pubt, "/config");
    String payload;
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
    if (strcmp_P(serverDescription, PSTR("WLED")) == 0) {
      doc["name"] =
          strcat(strcat(strcat(strcpy(bufcom, serverDescription), " "),
                        escapedMac.c_str() + 6),
                 " ip");
    } else {
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
    mqtt->publish(strcat(strcpy(bufcom, mqttDeviceTopic), "_ip/state"), 0,
                  false, Network.localIP().toString().c_str());
    // voltage
    if (usermodBattery) {
      doc.clear();
      memset(bufcom, 0, sizeof bufcom);
      doc["uniq_id"] = strcat(strcpy(bufcom, "wled_batt_"), escapedMac.c_str());
      memset(bufcom, 0, sizeof bufcom);
      if (strcmp_P(serverDescription, PSTR("WLED")) == 0) {
        doc["name"] =
            strcat(strcat(strcat(strcpy(bufcom, serverDescription), " "),
                          escapedMac.c_str() + 6),
                   " battery voltage");
      } else {
        doc["name"] =
            strcat(strcpy(bufcom, serverDescription), " battery voltage");
      }
      memset(bufcom, 0, sizeof bufcom);
      doc["stat_t"] = strcat(strcpy(bufcom, mqttDeviceTopic), "/voltage");
      doc["unit_of_meas"] = "V";
      doc["dev_cla"] = "voltage";

      JsonObject device = doc.createNestedObject("dev");
      setDeviceAttr(device);

      payload.clear();
      serializeJson(doc, payload);

      memset(bufcom, 0, sizeof bufcom);
      DEBUG_PRINTLN(payload);
      mqtt->publish(
          strcat(strcat(strcpy(bufcom, "homeassistant/sensor/"), mqttClientID),
                 "_batt/config"),
          0, true, payload.c_str());

    }

    // override switch
    doc.clear();
    memset(bufcom, 0, sizeof bufcom);
    doc["uniq_id"] =
        strcat(strcpy(bufcom, "wled_override"), escapedMac.c_str());
    memset(bufcom, 0, sizeof bufcom);
    if (strcmp_P(serverDescription, PSTR("WLED")) == 0) {
      doc["name"] =
          strcat(strcat(strcat(strcpy(bufcom, serverDescription), " "),
                        escapedMac.c_str() + 6),
                 " override");
    } else {
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
    mqtt->publish(doc["avty_t"].as<String>().c_str(), 0, true,
                  "online");  // retain message for a LWT
    mqtt->subscribe(bufcom, 0);
    releaseJSONBufferLock();
#endif
  }

  void sendState() {
    char subuf[38];
    strlcpy(subuf, mqttDeviceTopic, 33);
    strcat_P(subuf, PSTR("/state"));
    String payload = getState();
    DEBUG_PRINTLN(payload);
    mqtt->publish(subuf, 0, false, payload.c_str());  // do not retain message
    // override switch
    payload.clear();
    if (realtimeOverride == REALTIME_OVERRIDE_NONE) {
      payload = "OFF";
    } else {
      payload = "ON";
    }
    memset(subuf, 0, sizeof subuf);
    strlcpy(subuf, mqttDeviceTopic, 33);
    strcat_P(subuf, PSTR("_override/state"));
    DEBUG_PRINTLN(subuf);
    DEBUG_PRINTLN(payload);
    mqtt->publish(subuf, 0, false, payload.c_str());  // do not retain message

    if (usermodBattery) {
    memset(subuf, 0, sizeof subuf);
    char buffer[16];
    const char* result = buffer;
    DEBUG_PRINTLN(result);
    snprintf(buffer, sizeof(buffer), "%.2f", usermodBattery->getVoltage());
    mqtt->publish(strcat(strcpy(subuf, mqttDeviceTopic), "/voltage"), 0,
                  false,result);

    }
  }

 public:
  virtual void setup() {
    usermodBattery = usermods.getUsermod<UsermodBattery>(USERMOD_ID_BATTERY);
    if (usermodBattery) {
      DEBUG_PRINTLN("UsermodHomeAssistantDiscovery inited");
    }
  }
  virtual void loop() {
    // implementation here
  }

  inline void onMqttConnect(bool sessionPresent) { sendHADiscoveryMQTT(); }
  inline bool onMqttMessage(char *topic, char *payload) {
    setState(payload, topic);
    publishMqtt();
    return true;
  }
  inline void publishMqtt() { sendState(); }
};

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

const char HA_static_JSON[] PROGMEM =
    R"=====(,"bri_val_tpl":"{{value}}","rgb_cmd_tpl":"{{'#%02x%02x%02x' | format(red, green, blue)}}","rgb_val_tpl":"{{value[1:3]|int(base=16)}},{{value[3:5]|int(base=16)}},{{value[5:7]|int(base=16)}}","fx_val_tpl":"{{value}}","state_value_template": "{% if value | int == 0 %} OFF {% else %} ON {% endif %}"})=====";

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
#ifdef USERMOD_BATTERY
 private:
  UsermodBattery *usermodBattery;
#endif

  void sendHADiscoveryMQTT() {
#if ARDUINO_ARCH_ESP32 || LWIP_VERSION_MAJOR >= 1
    if (mqtt == nullptr || !mqtt->connected()) return;
    char bufcom[45];
#ifdef WLED_USE_DYNAMIC_JSON
    DynamicJsonDocument doc(JSON_BUFFER_SIZE);
#else
    if (!requestJSONBufferLock(15)) return;
#endif
    if (strcmp_P(serverDescription, PSTR("WLED")) == 0) {
      doc["name"] =
          strcat(strcat(strcat(strcpy(bufcom, serverDescription), " "),
                        escapedMac.c_str() + 6),
                 " light");
    } else {
      doc["name"] = strcat(strcpy(bufcom, serverDescription), " light");
    }
    doc["avty_t"] = strcat(strcpy(bufcom, mqttDeviceTopic), "/status");
    doc["uniq_id"] = strcat(strcpy(bufcom, "wled_light_"), escapedMac.c_str());

    char bufc[36], bufcol[38], bufg[36], bufapi[38];

    strcpy(bufc, mqttDeviceTopic);
    strcpy(bufcol, mqttDeviceTopic);
    strcpy(bufg, mqttDeviceTopic);
    strcpy(bufapi, mqttDeviceTopic);

    strcat(bufc, "/c");
    strcat(bufcol, "/col");
    strcat(bufg, "/g");
    strcat(bufapi, "/api");

    doc["stat_t"] = bufg;
    doc["cmd_t"] = mqttDeviceTopic;
    doc["rgb_stat_t"] = bufc;
    doc["rgb_cmd_t"] = bufcol;
    doc["bri_cmd_t"] = mqttDeviceTopic;
    doc["bri_stat_t"] = bufg;
    
    doc["fx_cmd_t"] = strcat(strcpy(bufcom, mqttDeviceTopic), "/command");
    doc["fx_stat_t"] = strcat(strcpy(bufcom, mqttDeviceTopic), "_fx/state");

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
    char *payloadBuffer;
    payloadBuffer = new char[4500];
    size_t jlen = measureJson(doc);
    serializeJson(doc, payloadBuffer, 4500);
    // add values which don't change
    strcpy_P(payloadBuffer + jlen - 1, HA_static_JSON);
    if (jlen < 4300)  // make sure buffer not overflow
    {
      DEBUG_PRINTLN(payloadBuffer);
      mqtt->publish(pubt, 0, true, payloadBuffer);
    }
    delete[] payloadBuffer;
    mqtt->subscribe(strcat(strcpy(bufcom, mqttDeviceTopic), "/command"), 0);

    // ip address
    doc.clear();
    String payload;
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
    dev = doc.createNestedObject("dev");
    setDeviceAttr(dev);
    serializeJson(doc, payload);
    DEBUG_PRINTLN(payload);
    memset(bufcom, 0, sizeof bufcom);
    mqtt->publish(
        strcat(strcat(strcpy(bufcom, "homeassistant/sensor/"), mqttClientID),
               "_ip/config"),
        0, true, payload.c_str());
#ifdef USERMOD_BATTERY
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
#endif

      dev = doc.createNestedObject("dev");
      setDeviceAttr(dev);

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
    mqtt->subscribe(bufcom, 0);
    releaseJSONBufferLock();
#endif
  }

  void sendState() {
    // override switch
    char subuf[38];
    String payload;
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
    // send ip state
    memset(subuf, 0, sizeof subuf);
    mqtt->publish(strcat(strcpy(subuf, mqttDeviceTopic), "_ip/state"), 0, false,
                  Network.localIP().toString().c_str());
    // send Fx
    memset(subuf, 0, sizeof subuf);
    
    char lineBuffer[128];
    strncpy_P(lineBuffer, strip.getModeData(effectCurrent), 127);
    if (lineBuffer[0] != 0) {
      char *dataPtr = strchr(lineBuffer, '@');
      if (dataPtr) *dataPtr = 0;
    }
    mqtt->publish(strcat(strcpy(subuf, mqttDeviceTopic), "_fx/state"), 0, false, lineBuffer);
#ifdef USERMOD_BATTERY
    // votage
    if (usermodBattery) {
      memset(subuf, 0, sizeof subuf);
      char buffer[16];
      const char *result = buffer;
      DEBUG_PRINTLN(result);
      snprintf(buffer, sizeof(buffer), "%.2f", usermodBattery->getVoltage());
      mqtt->publish(strcat(strcpy(subuf, mqttDeviceTopic), "/voltage"), 0,
                    false, result);
    }
#endif
  }

 public:
  virtual void setup() {
#ifdef USERMOD_BATTERY
    usermodBattery = usermods.getUsermod<UsermodBattery>(USERMOD_ID_BATTERY);
    if (usermodBattery) {
      DEBUG_PRINTLN("UsermodHomeAssistantDiscovery inited");
    }
#endif
  }
  virtual void loop() {
    // implementation here
  }

  inline void onMqttConnect(bool sessionPresent) { sendHADiscoveryMQTT(); }
  inline bool onMqttMessage(char *topic, char *payload) {
    setState(payload, topic);
    return true;
  }
  inline void publishMqtt() { sendState(); }
};

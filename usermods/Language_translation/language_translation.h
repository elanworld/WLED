#include "wled.h"

class LanguageUsermod : public Usermod {
    private:
     bool enable = true;
     String languageJsUrl = "";
     public:
     
    void setup() {
    }
    void loop() {
    }
     void addToJsonState(JsonObject& root)
    {
      root["useLocalTranslation"] = enable;
      root["languageJsUrl"] = languageJsUrl;
    }
    void readFromJsonState(JsonObject& root)
    {
      enable = root["useLocalTranslation"] | enable;
      languageJsUrl = root["languageJsUrl"] | languageJsUrl;
    }
    void addToConfig(JsonObject& root)
    {
      JsonObject top = root.createNestedObject("language");
      top["enable"] = enable; //save these vars persistently whenever settings are saved
      top["languageJsUrl"] = languageJsUrl;
    }
    bool readFromConfig(JsonObject& root)
    {
      JsonObject top = root["language"];
      bool configComplete = !top.isNull();
      configComplete &= getJsonValue(top["enable"], enable);
      configComplete &= getJsonValue(top["languageJsUrl"], languageJsUrl);
      return configComplete;
    }
};
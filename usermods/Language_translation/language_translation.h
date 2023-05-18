#include "wled.h"

class LanguageUsermod : public Usermod {
    private:
     bool useLocalTranslation = true;
     String languageJsUrl = "";
     public:
     
    void setup() {
    }
    void loop() {
    }
     void addToJsonState(JsonObject& root)
    {
      root["useLocalTranslation"] = useLocalTranslation;
      root["languageJsUrl"] = languageJsUrl;
    }
    void readFromJsonState(JsonObject& root)
    {
      useLocalTranslation = root["useLocalTranslation"] | useLocalTranslation;
      languageJsUrl = root["languageJsUrl"] | languageJsUrl;
    }
    void addToConfig(JsonObject& root)
    {
      JsonObject top = root.createNestedObject("language");
      top["useLocalTranslation"] = useLocalTranslation; //save these vars persistently whenever settings are saved
      top["languageJsUrl"] = languageJsUrl;
    }
    bool readFromConfig(JsonObject& root)
    {
      JsonObject top = root["language"];
      bool configComplete = !top.isNull();
      configComplete &= getJsonValue(top["useLocalTranslation"], useLocalTranslation);
      configComplete &= getJsonValue(top["languageJsUrl"], languageJsUrl);
      return configComplete;
    }
};
#include "wled.h"
#include "gatt_bridge.h"
// #include <NimBLEDevice.h>
#include "../common_tools/wled_common_tools.h"
#ifdef USERMOD_SLEEP
#include "../deep_sleep/usermod_deep_sleep.h"
#endif

class BleMod
{
public:
  String serverUUID;
  String charUUID;
  BleMod() : serverUUID(padStringToUUID(stringToHex("WLED"))),
    charUUID(padStringToUUID(stringToHex("WLED01"))) {
  }

  void setSleep(bool requestWake)
  {
#ifdef USERMOD_SLEEP
    gettimeofday(&change_time, NULL);
    modules["ble"] = requestWake;
#endif
  }

  String padStringToUUID(const String& str)
  {
    String paddedStr = str;
    while (paddedStr.length() < 32)
    {
      paddedStr += "0";
    }

    // Insert hyphens at appropriate positions
    paddedStr = paddedStr.substring(0, 8) + "-" + paddedStr.substring(8, 12) + "-" +
      paddedStr.substring(12, 16) + "-" + paddedStr.substring(16, 20) + "-" +
      paddedStr.substring(20);

    return paddedStr;
  }

  String stringToHex(const String& str)
  {
    String hexString;
    for (char c : str)
    {
      char buf[3];
      sprintf(buf, "%02X", static_cast<unsigned char>(c));
      hexString += buf;
    }
    return hexString;
  }
};

BLEServer* pServer;
class ServerCallbacks : public NimBLEServerCallbacks, BleMod
{
  void onConnect(NimBLEServer* pServer)
  {
    DEBUG_PRINT("Client connected");
    DEBUG_PRINTLN("Multi-connect support: start advertising");
    NimBLEDevice::startAdvertising();
    setSleep(true);
  };

  void onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc)
  {
    DEBUG_PRINT("Client address: ");
    DEBUG_PRINTLN(NimBLEAddress(desc->peer_ota_addr).toString().c_str());
    pServer->updateConnParams(desc->conn_handle, 24, 48, 0, 60);
    setSleep(true);
  };
  void onDisconnect(NimBLEServer* pServer)
  {
    DEBUG_PRINTLN("Client disconnected - start advertising");
    NimBLEDevice::startAdvertising();
    setSleep(false);
  };
  void onMTUChange(uint16_t MTU, ble_gap_conn_desc* desc)
  {
    DEBUG_PRINTLN("MTU updated: for connection ID:");
  };

  uint32_t onPassKeyRequest()
  {
    DEBUG_PRINTLN("Server Passkey Request");
    return 123456;
  };

  bool onConfirmPIN(uint32_t pass_key)
  {
    DEBUG_PRINT("The passkey YES/NO number: ");
    DEBUG_PRINTLN(pass_key);
    return true;
  };

  void onAuthenticationComplete(ble_gap_conn_desc* desc)
  {
    if (!desc->sec_state.encrypted)
    {
      NimBLEDevice::getServer()->disconnect(desc->conn_handle);
      DEBUG_PRINTLN("Encrypt connection failed - disconnecting client");
      return;
    }
    DEBUG_PRINTLN("Starting BLE work!");
  };
};

/** Handler class for characteristic actions */
class CharacteristicCallbacks : public NimBLECharacteristicCallbacks, BleMod
{
  void onRead(NimBLECharacteristic* pCharacteristic)
  {
    DEBUG_PRINT(pCharacteristic->getUUID().toString().c_str());
    pCharacteristic->setValue(getState());
    DEBUG_PRINT(": onRead(), value: ");
    DEBUG_PRINTLN(pCharacteristic->getValue().c_str());
    setSleep(true);
  };

  void onWrite(NimBLECharacteristic* pCharacteristic)
  {
    DEBUG_PRINT(pCharacteristic->getUUID().toString().c_str());
    DEBUG_PRINT(": onWrite(), value: ");
    DEBUG_PRINTLN(pCharacteristic->getValue().c_str());
    setState(pCharacteristic->getValue().c_str(), "/command");
    notifyToCilent();
    setSleep(true);
  };

  void onNotify(NimBLECharacteristic* pCharacteristic)
  {
    DEBUG_PRINTLN("Sending notification to clients");
  };

  void onStatus(NimBLECharacteristic* pCharacteristic, Status status, int code)
  {
    String str = ("Notification/Indication status code: ");
    str += status;
    str += ", return code: ";
    str += code;
    str += ", ";
    str += NimBLEUtils::returnCodeToString(code);
    DEBUG_PRINTLN(str);
    setSleep(true);
  };

  void onSubscribe(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc, uint16_t subValue)
  {
    String str = "Client ID: ";
    str += desc->conn_handle;
    str += " Address: ";
    str += std::string(NimBLEAddress(desc->peer_ota_addr)).c_str();
    if (subValue == 0)
    {
      str += " Unsubscribed to ";
    }
    else if (subValue == 1)
    {
      str += " Subscribed to notfications for ";
    }
    else if (subValue == 2)
    {
      str += " Subscribed to indications for ";
    }
    else if (subValue == 3)
    {
      str += " Subscribed to notifications and indications for ";
    }
    str += std::string(pCharacteristic->getUUID()).c_str();
    DEBUG_PRINTLN(str);
  };

  // notyfy current state
  void notifyToCilent()
  {
    if (pServer->getConnectedCount())
    {
      NimBLEService* pSvc = pServer->getServiceByUUID(serverUUID.c_str());
      if (pSvc)
      {
        NimBLECharacteristic* pChr = pSvc->getCharacteristic(charUUID.c_str());
        if (pChr)
        {
          pChr->setValue(getState());
          DEBUG_PRINT("notify:");
          DEBUG_PRINTLN(pChr->getValue());
          pChr->notify(true);
        }
      }
    }
  }
};

class BleGattApiServer : public Usermod, BleMod
{

public:
  virtual void setup()
  {
    if (!bleOpen)
    {
      DEBUG_PRINTLN("ble server closed!");
      return;
    }
    DEBUG_PRINTLN("Starting NimBLE Server");
    if (noWifiSleep)
    {
      noWifiSleep = false;
      ESP.restart();
      return;
    }
    
    initBLE([this]() { createBleServer(); });
  }

  virtual void loop()
  {
  }


  void createBleServer() {
    DEBUG_PRINTLN("Starting bleServer");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    NimBLEService* pWledService = pServer->createService(serverUUID.c_str());
    NimBLECharacteristic* pFoodCharacteristic = pWledService->createCharacteristic(
      charUUID.c_str(),
      NIMBLE_PROPERTY::READ |
      NIMBLE_PROPERTY::WRITE |
      NIMBLE_PROPERTY::NOTIFY);

    pFoodCharacteristic->setValue("ON");
    pFoodCharacteristic->setCallbacks(new CharacteristicCallbacks());
    pWledService->start();
  }

  virtual void addToConfig(JsonObject& root)
  {
    JsonObject top = root.createNestedObject("Bluetooth");
    top["bleOpen"] = bleOpen;
  }
  virtual bool readFromConfig(JsonObject& root)
  {
    JsonObject top = root["Bluetooth"];
    bool configComplete = !top.isNull();
    configComplete &= getJsonValue(top["bleOpen"], bleOpen);
    return configComplete;
  }
};

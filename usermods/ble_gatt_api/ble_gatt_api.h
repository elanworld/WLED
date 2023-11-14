#include "wled.h"
#include <NimBLEDevice.h>
#include "../common_tools/wled_common_tools.h"

static NimBLEServer *pServer;

class ServerCallbacks : public NimBLEServerCallbacks
{
  void onConnect(NimBLEServer *pServer)
  {
    DEBUG_PRINT("Client connected");
    DEBUG_PRINTLN("Multi-connect support: start advertising");
    NimBLEDevice::startAdvertising();
  };

  void onConnect(NimBLEServer *pServer, ble_gap_conn_desc *desc)
  {
    DEBUG_PRINT("Client address: ");
    DEBUG_PRINTLN(NimBLEAddress(desc->peer_ota_addr).toString().c_str());
    pServer->updateConnParams(desc->conn_handle, 24, 48, 0, 60);
  };
  void onDisconnect(NimBLEServer *pServer)
  {
    DEBUG_PRINTLN("Client disconnected - start advertising");
    NimBLEDevice::startAdvertising();
  };
  void onMTUChange(uint16_t MTU, ble_gap_conn_desc *desc)
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

  void onAuthenticationComplete(ble_gap_conn_desc *desc)
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
class CharacteristicCallbacks : public NimBLECharacteristicCallbacks
{
  void onRead(NimBLECharacteristic *pCharacteristic)
  {
    DEBUG_PRINT(pCharacteristic->getUUID().toString().c_str());
    pCharacteristic->setValue(getState());
    DEBUG_PRINT(": onRead(), value: ");
    DEBUG_PRINTLN(pCharacteristic->getValue().c_str());
  };

  void onWrite(NimBLECharacteristic *pCharacteristic)
  {
    DEBUG_PRINT(pCharacteristic->getUUID().toString().c_str());
    DEBUG_PRINT(": onWrite(), value: ");
    DEBUG_PRINTLN(pCharacteristic->getValue().c_str());
    setState(pCharacteristic->getValue().c_str(), "/command");
    notifyToCilent();
  };

  void onNotify(NimBLECharacteristic *pCharacteristic)
  {
    DEBUG_PRINTLN("Sending notification to clients");
  };

  void onStatus(NimBLECharacteristic *pCharacteristic, Status status, int code)
  {
    String str = ("Notification/Indication status code: ");
    str += status;
    str += ", return code: ";
    str += code;
    str += ", ";
    str += NimBLEUtils::returnCodeToString(code);
    DEBUG_PRINTLN(str);
  };

  void onSubscribe(NimBLECharacteristic *pCharacteristic, ble_gap_conn_desc *desc, uint16_t subValue)
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
      NimBLEService *pSvc = pServer->getServiceByUUID("DEAD");
      if (pSvc)
      {
        NimBLECharacteristic *pChr = pSvc->getCharacteristic("BEEF");
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

class BleGattApiServer : public Usermod
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
    if (strcmp_P(serverDescription, PSTR("WLED")) == 0)
    {
      char bufn[15];
      NimBLEDevice::init(strcat(strcpy(bufn, "WLED "), escapedMac.c_str() + 6));
    }
    else
    {
      NimBLEDevice::init(serverDescription);
    }
    NimBLEDevice::setSecurityAuth(/*BLE_SM_PAIR_AUTHREQ_BOND | BLE_SM_PAIR_AUTHREQ_MITM |*/ BLE_SM_PAIR_AUTHREQ_SC);

    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    NimBLEService *pBaadService = pServer->createService("DEAD");
    NimBLECharacteristic *pFoodCharacteristic = pBaadService->createCharacteristic(
        "BEEF",
        NIMBLE_PROPERTY::READ |
            NIMBLE_PROPERTY::WRITE |
            NIMBLE_PROPERTY::NOTIFY);

    pFoodCharacteristic->setValue("ON");
    pFoodCharacteristic->setCallbacks(new CharacteristicCallbacks());
    pBaadService->start();

    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(pBaadService->getUUID());
    pAdvertising->setScanResponse(true);
    pAdvertising->start();
    DEBUG_PRINTLN("Advertising Started");
  }

  virtual void loop()
  {
  }

  virtual void addToConfig(JsonObject &root)
  {
    JsonObject top = root.createNestedObject("Bluetooth");
    top["bleOpen"] = bleOpen;
    top["wifiOpen"] = wifiOpen;
  }
  virtual bool readFromConfig(JsonObject &root)
  {
    JsonObject top = root["Bluetooth"];
    bool configComplete = !top.isNull();
    configComplete &= getJsonValue(top["bleOpen"], bleOpen);
    configComplete &= getJsonValue(top["wifiOpen"], wifiOpen);
    // board will crash if wifi bluetooth open togather
    if (bleOpen == wifiOpen)
    {
      DEBUG_PRINTLN("readFromConfig: wifi open bluetooth close");
      wifiOpen = true;
      bleOpen = false;
    }
    return configComplete;
  }
};

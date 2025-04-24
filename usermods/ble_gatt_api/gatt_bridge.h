#pragma onece
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <HTTPClient.h>
#include <StreamString.h>
#include <base64.h>
// BLE UUIDs
#define SERVICE_UUID_NAME "bridge"
#define CHARACTERISTIC_UUID_NAME "bridge01"
BLEService* pService;
BLECharacteristic* pCharacteristic;

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

void handleBLERequest(const std::string& jsonStr, BLECharacteristic* pCharacteristic)
{
  DEBUG_PRINTF("completed JSON: %s\n", jsonStr.c_str());

  DynamicJsonDocument doc(JSON_BUFFER_SIZE * 4);
  DeserializationError error = deserializeJson(doc, jsonStr);
  if (error)
  {
    Serial.print("Failed to parse JSON: ");
    DEBUG_PRINTLN(error.f_str());
    pCharacteristic->setValue("Invalid JSON");
    pCharacteristic->notify();
    releaseJSONBufferLock();
    return;
  }

  const char* url = doc["url"];
  const char* method = doc["method"];
  const char* body = doc["body"];
  JsonObject headers = doc["header"].as<JsonObject>();

  HTTPClient http;
  const char* headersC[] = {
      "Content-Type",    // 内容类型
      "Content-Length",  // 内容长度
      "Accept-Encoding",
      "content-encoding",
      "Cache-Control",   // 缓存控制
      "Location",        // 重定向目标
      "Set-Cookie",      // Cookie 信息
      "Authorization",   // 认证信息
      "etag",          // 服务器类型
      "Date",            // 响应日期
  };

  // 在发送请求之前收集这些响应头
  http.collectHeaders(headersC, 10);
  http.begin(url);

  for (JsonPair kv : headers)
  {
    http.addHeader(kv.key().c_str(), kv.value().as<const char*>());
  }

  int httpCode;
  if (strcmp(method, "GET") == 0)
  {
    httpCode = http.GET();
  }
  else if (strcmp(method, "POST") == 0)
  {
    httpCode = http.POST(body);
  }
  else
  {
    pCharacteristic->setValue("Unsupported HTTP method");
    pCharacteristic->notify();
    http.end();
    return;
  }

  doc["status"] = httpCode;

  JsonObject resHeaders = doc.createNestedObject("header");
  bool isGzipped = false;
  for (int i = 0; i < http.headers(); i++) {
    String name = http.headerName(i);
    String value = http.header(i);
    resHeaders[name] = value;
    if (name.equalsIgnoreCase("content-encoding") && value.equalsIgnoreCase("gzip")) {
      isGzipped = true;
    }
  }
  String responsePayload;
  if (httpCode > 0)
  {
    if (isGzipped)
    {

      StreamString s;
      int bytesWritten = http.writeToStream(&s);
      DEBUG_PRINTF("bytesWritten %d\n", s.length());
      // 分配 buffer 用于读取
      size_t len = s.length();
      std::unique_ptr<uint8_t[]> buffer(new uint8_t[len]);  // 使用智能指针防止内存泄漏

      // 将流数据读入 buffer
      s.readBytes(buffer.get(), len);

      // Base64 编码
      String base64Encoded = base64::encode(buffer.get(), len);
      DEBUG_PRINTF("base64Encoded len: %d\n", base64Encoded.length());
      doc["raw"] = true;
      doc["body"] = base64Encoded;
    } else {
      doc["body"] = http.getString();
    }
  }

  http.end();

  String response;
  serializeJson(doc, response);
  DEBUG_PRINTF("result: %s\n", response.c_str());

  const size_t chunkSize = 500;
  size_t responseLength = response.length();
  size_t sentLength = 0;
  while (sentLength < responseLength)
  {
    size_t currentChunkSize = min(chunkSize, responseLength - sentLength);
    String chunk = response.substring(sentLength, sentLength + currentChunkSize);
    if (sentLength == 0)
    {
      chunk = "START" + chunk;
    }
    if (sentLength + currentChunkSize == responseLength)
    {
      chunk += "END";
    }
    pCharacteristic->setValue(chunk.c_str());
    pCharacteristic->notify();
    sentLength += currentChunkSize;
    delay(20);
  }
}

void BLEWriteTask(void* parameter)
{
  auto* args = (std::pair<std::string, BLECharacteristic*> *)parameter;
  handleBLERequest(args->first, args->second);
  delete args;
  vTaskDelete(NULL);
}

class BridgeServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer* pServer) override
  {
    DEBUG_PRINTLN("clients connected");
  }

  void onDisconnect(BLEServer* pServer) override
  {
    DEBUG_PRINTLN("clients disconnect...");
    BLEDevice::startAdvertising();
  }
};

// Function to handle BLE read/write requests
class BridgeCallbacks : public BLECharacteristicCallbacks
{
  std::string accumulatedValue;
  bool receiving = false;
  void onWrite(BLECharacteristic* pCharacteristic)
  {
    std::string value = pCharacteristic->getValue();
    if (value.find("START") == 0)
    {
      accumulatedValue.clear();
      receiving = true;
      value.erase(0, 5);
    }
    accumulatedValue += value;
    if (value.rfind("END") == value.size() - 3)
    {
      accumulatedValue.erase(accumulatedValue.size() - 3);
      receiving = false;
    }

    if (!receiving)
    {
      auto* args = new std::pair<std::string, BLECharacteristic*>(accumulatedValue, pCharacteristic);
      xTaskCreatePinnedToCore(BLEWriteTask, "BLEWriteTask", 8192, args, 1, NULL, 1);
    }
  }

  void onStatus(BLECharacteristic* pCharacteristic, Status s, uint32_t code) override
  {
    DEBUG_PRINTLN("onStatus...");
  }
};

// Function to initialize BLE
void initBLE()
{
  BLEDevice::init("ESP32-Bridge");
  BLEDevice::setMTU(512);
  BLEServer* pServer = BLEDevice::createServer();
  BLEService* pService = pServer->createService(padStringToUUID(stringToHex(SERVICE_UUID_NAME)).c_str());
  pCharacteristic = pService->createCharacteristic(
    padStringToUUID(stringToHex(CHARACTERISTIC_UUID_NAME)).c_str(),
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_NOTIFY);

  pCharacteristic->setValue("");
  pService->start();

  // Start advertising
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(padStringToUUID(stringToHex(SERVICE_UUID_NAME)).c_str());
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  pServer->setCallbacks(new BridgeServerCallbacks());

  DEBUG_PRINTLN("BLE service and characteristic started, advertising...");
  // Set BLE characteristic callbacks
  pCharacteristic->setCallbacks(new BridgeCallbacks());
}

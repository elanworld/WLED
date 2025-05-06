#pragma onece
#include "wled.h"
#include <Arduino.h>
#include <NimBLEDevice.h>
#include <HTTPClient.h>
#include <StreamString.h>
#include <base64.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
// BLE UUIDs
#define SERVICE_UUID_NAME "bridge"
#define CHARACTERISTIC_UUID_NAME "bridge01"
BLEService *pService;
BLECharacteristic *pCharacteristic;
size_t MTUSIZE = 20;

SemaphoreHandle_t httpSemaphore;

String padStringToUUID(const String &str)
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

String stringToHex(const String &str)
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

void sendData(String &response, BLECharacteristic *pCharacteristic, bool addEnd=true)
{

  const size_t chunkSize = MTUSIZE - 3 - 100; // mtu ATT header 3, START TAG 4
    DEBUG_PRINTF("chunkSize: %d\n", chunkSize);
  // const size_t chunkSize = 500;
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
    if (sentLength + currentChunkSize == responseLength && addEnd)
    {
      chunk += "END";
    }
    pCharacteristic->setValue((uint8_t *)chunk.c_str(), chunk.length());
    pCharacteristic->notify();
    DEBUG_PRINTF("chunk: %s\n", chunk.c_str());
    sentLength += currentChunkSize;
    delay(20);
  }
}

void handleBLERequest(const std::string &jsonStr, BLECharacteristic *pCharacteristic)
{
  DEBUG_PRINTF("completed JSON: %s\n", jsonStr.c_str());

  size_t allocSize = std::min((uint32_t)(ESP.getMaxAllocHeap() / 2), (uint32_t)(JSON_BUFFER_SIZE * 4));

  DynamicJsonDocument *docs = new DynamicJsonDocument(allocSize);
  DynamicJsonDocument &doc = *docs;
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

  const char *url = doc["url"];
  const char *method = doc["method"];
  const char *body = doc["body"];
  JsonObject headers = doc["header"].as<JsonObject>();

  HTTPClient http;
  const char *headersC[] = {
      "Content-Type",   // 内容类型
      "Content-Length", // 内容长度
      "Accept-Encoding",
      "content-encoding",
      "If-None-Match",
      "Cache-Control", // 缓存控制
      "Location",      // 重定向目标
      "Set-Cookie",    // Cookie 信息
      "Authorization", // 认证信息
      "ETag",          // 服务器类型
      "Date",          // 响应日期
  };

  // 在发送请求之前收集这些响应头
  http.collectHeaders(headersC, 11);
  http.begin(url);

  for (JsonPair kv : headers)
  {
    http.addHeader(kv.key().c_str(), kv.value().as<const char *>());
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
  for (int i = 0; i < http.headers(); i++)
  {
    String name = http.headerName(i);
    String value = http.header(i);
    if (value == "" || value == nullptr)
      continue;

    resHeaders[name] = value;
    if (name.equalsIgnoreCase("content-encoding") && value.equalsIgnoreCase("gzip"))
    {
      isGzipped = true;
    }
  }
  StreamString s;
  String responsePayload;
  if (httpCode > 0)
  {
      int bytesWritten = http.writeToStream(&s);
      // unsigned int size = s.length();
      // DEBUG_PRINTF("bytesWritten %d\n", size);

      // // Base64 编码
      // String base64Encoded = base64::encode((uint8_t *)s.c_str(), size);
      // DEBUG_PRINTF("base64Encoded len: %d\n", base64Encoded.length());
      // DEBUG_PRINTF("StreamString content: %s\n", s.c_str());
      doc["raw"] = true;
      // doc["body"] = base64Encoded;

  }


  String response;
  serializeJson(doc, response);
  delete docs;
  // 去掉结尾的 }
if (response.endsWith("}")) {
  response.remove(response.length() - 1);
}

response = response + ",\"body\":\"";  // 追加 body 字段开始
  DEBUG_PRINTF("result: %s\n", response.c_str());
  

  sendData(response, pCharacteristic, false);
  DEBUG_PRINTF("written ble size: %d\n", response.length());
  std::vector<uint8_t> buffer;
  const size_t readChunk = 256; // 原始读取块大小
  uint8_t temp[readChunk];
  
  while (int n = s.readBytes(temp, readChunk))
  {
    buffer.insert(buffer.end(), temp, temp + n);

    while (buffer.size() >= 3)
    {
      size_t toEncode = (buffer.size() / 3) * 3; // 取3的倍数部分
      String encoded = base64::encode(buffer.data(), toEncode);
      DEBUG_PRINTF("setValue: %s\n", encoded.c_str());
      pCharacteristic->setValue((uint8_t *)encoded.c_str(),encoded.length());
      pCharacteristic->notify();
      delay(20);

      // 保留尾部不足3字节的数据
      std::vector<uint8_t> remain(buffer.begin() + toEncode, buffer.end());
      buffer = remain;
    }
  }

  // 编码剩下不足3字节的最后一部分（会加 padding）
  if (!buffer.empty())
  {
    String finalChunk = base64::encode(buffer.data(), buffer.size());
    DEBUG_PRINTF("setValue: %s\n", finalChunk.c_str());
    pCharacteristic->setValue((uint8_t *)finalChunk.c_str(), finalChunk.length());
    pCharacteristic->notify();
    delay(20);
  }

  String endStr =  "\"}END";
  pCharacteristic->setValue((uint8_t *)endStr.c_str(),endStr.length());
  pCharacteristic->notify();
  http.end();
}

void BLEWriteTask(void *parameter)
{
  DEBUG_PRINTLN("BLEWriteTask");
  // if (xSemaphoreTake(httpSemaphore, portMAX_DELAY) == pdTRUE)
  // {
    DEBUG_PRINTLN("BLEWriteTask start");
    auto *args = (std::pair<std::string, BLECharacteristic *> *)parameter;
    handleBLERequest(args->first, args->second);
    delete args;
    xSemaphoreGive(httpSemaphore);
    DEBUG_PRINTLN("BLEWriteTask end");
    vTaskDelete(NULL);
  // }
}

class BridgeServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer) override
  {
    DEBUG_PRINTLN("clients connected");
  }

  void onDisconnect(BLEServer *pServer) override
  {
    DEBUG_PRINTLN("clients disconnect...");
    BLEDevice::startAdvertising();
  }

  void onMTUChange(uint16_t MTU, ble_gap_conn_desc *desc) override
  {
    DEBUG_PRINTF("MTU Change to: %d\n", MTU);
    MTUSIZE = MTU;
  }
};

// Function to handle BLE read/write requests
class BridgeCallbacks : public BLECharacteristicCallbacks
{
  std::string accumulatedValue;
  bool receiving = false;
  void onWrite(BLECharacteristic *pCharacteristic)
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

      // handleBLERequest(accumulatedValue, pCharacteristic);
    }
  }

  void onStatus(BLECharacteristic *pCharacteristic, Status s, int code) override
  {
    DEBUG_PRINTF("status: %d\n", code);
  }
};

// Function to initialize BLE
void initBLE()
{
  BLEDevice::init("ESP32-Bridge");
  BLEDevice::setMTU(512);
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(padStringToUUID(stringToHex(SERVICE_UUID_NAME)).c_str());
  pCharacteristic = pService->createCharacteristic(
      padStringToUUID(stringToHex(CHARACTERISTIC_UUID_NAME)).c_str(),
      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY);

  pServer->setCallbacks(new BridgeServerCallbacks());

  DEBUG_PRINTLN("BLE service and characteristic started, advertising...");
  pCharacteristic->setCallbacks(new BridgeCallbacks());
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  // pAdvertising->addServiceUUID(padStringToUUID(stringToHex(SERVICE_UUID_NAME)).c_str());
  pAdvertising->setScanResponse(false);
  BLEDevice::startAdvertising();

  httpSemaphore = xSemaphoreCreateBinary();
  xSemaphoreGive(httpSemaphore); // 初始化为可用
}

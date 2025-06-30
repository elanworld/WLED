#pragma onece
#include "wled.h"
#include <Arduino.h>
#include <NimBLEDevice.h>
#include <HTTPClient.h>
#include <StreamString.h>
#include <base64.h>
#include "lwip/sockets.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
// BLE UUIDs
#define SERVICE_UUID_NAME "bridge"
#define CHARACTERISTIC_UUID_NAME "bridge01"
#define BLE_CHUNK_TAG_START "START"
#define BLE_CHUNK_TAG_END   "END"
#define SERVER_IP   "127.0.0.1"  // Example.com IP
#define SERVER_PORT 80
BLEService* pService;
BLECharacteristic* pCharacteristic;
size_t MTUSIZE = 20;
std::vector<uint8_t> bleInputBuffer;
bool collectingBLE = false;
int sock = -1;
bool socketConnected = false;


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

void sendData(String& response, BLECharacteristic* pCharacteristic, bool addEnd = true)
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
    DEBUG_PRINTF("chunk: %s\n", chunk.c_str());
    pCharacteristic->setValue((uint8_t*)chunk.c_str(), chunk.length());
    pCharacteristic->notify();
    sentLength += currentChunkSize;
    delay(20);
  }
}


// 将响应数据加上标记后分片发 BLE
void sendBLEChunk(const uint8_t* data, size_t len, bool first = false, bool end = false) {
  DEBUG_PRINTF("send ble len=%d\n", len);
  const String TAG_START = BLE_CHUNK_TAG_START;
  const String TAG_END = BLE_CHUNK_TAG_END;
  std::vector<uint8_t> outChunk;

  if (first) {
    outChunk.insert(outChunk.end(),
      BLE_CHUNK_TAG_START,
      BLE_CHUNK_TAG_START + strlen(BLE_CHUNK_TAG_START));
  }

  outChunk.insert(outChunk.end(), data, data + len);

  if (end) {
    outChunk.insert(outChunk.end(),
      BLE_CHUNK_TAG_END,
      BLE_CHUNK_TAG_END + strlen(BLE_CHUNK_TAG_END));
  }

  // 打印调试用字符形式
  std::string debugStr(outChunk.begin(), outChunk.end());
  DEBUG_PRINTF("send ble (str): %.*s:", (int)outChunk.size(), (const char*)outChunk.data());
  DEBUG_PRINTLN();

  // 发送数据
  pCharacteristic->setValue(outChunk.data(), outChunk.size());
  pCharacteristic->notify();

  delay(100);
}

void handleBLERequest(BLECharacteristic* pCharacteristic, std::string value)
{
  const char* data = value.c_str();
  size_t length = value.length();

  // DEBUG_PRINTF("ble len: %d\n",length);  // 推荐这种写法，防止 \0 截断
  std::string chunk((const char*)data, length);

  if (chunk.find("START") != std::string::npos && !socketConnected) {
    // 建立 socket
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    sockaddr_in dest;
    dest.sin_family = AF_INET;
    dest.sin_port = htons(80);
    inet_pton(AF_INET, SERVER_IP, &dest.sin_addr);

    if (connect(sock, (struct sockaddr*)&dest, sizeof(dest)) < 0) {
      Serial.println("Socket connect failed");
      close(sock);
      sock = -1;
      return;
    }
    socketConnected = true;
  }

  // 移除 tag 再发送
  std::string cleanChunk = chunk;
  size_t startPos = cleanChunk.find("START");
  if (startPos != std::string::npos)
    cleanChunk.erase(startPos, 5);

  size_t endPos = cleanChunk.find("END");
  bool isFinal = endPos != std::string::npos;
  if (isFinal)
    cleanChunk.erase(endPos, 3);

  // 发送到 socket
  if (socketConnected && sock >= 0 && !cleanChunk.empty()) {
    send(sock, cleanChunk.data(), cleanChunk.size(), 0);
  }
  else {
    DEBUG_PRINTLN("skip send socket");
  }

  if (isFinal) {
    DEBUG_PRINTLN("start read response");
    // 读取 socket 响应
    uint8_t respBuf[400];
    int len = 0;
    bool first = true;
    while (true) {
      esp_task_wdt_reset();
      len = recv(sock, respBuf, sizeof(respBuf), 0);
      if (len < 400) {
        sendBLEChunk(respBuf, len, first, true);
        break;
      }
      else if (len == 0) {
        // 对方关闭连接
        DEBUG_PRINTLN("recv finished, sending final BLE chunk");
        sendBLEChunk(respBuf, 0, first, true);
        break;
      }
      else if (len > 0) {
        sendBLEChunk(respBuf, len, first);
      }
      else {
        DEBUG_PRINTF("recv error: %d\n", errno);
        break;
      }
      first = false;
    }
    DEBUG_PRINTLN("response done");

    close(sock);
    sock = -1;
    socketConnected = false;
  }
}

void BLEWriteTask(void* parameter)
{
  DEBUG_PRINTLN("BLEWriteTask start");
  try {
    auto* args = (std::pair<std::string, BLECharacteristic*> *)parameter;
    handleBLERequest(args->second, args->first);
    delete(args);
    DEBUG_PRINTLN("BLEWriteTask end");
    vTaskDelete(NULL);
  }
  catch (const std::exception& e) {
    DEBUG_PRINTF("Exception: %s\n", e.what());
  }
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

  void onMTUChange(uint16_t MTU, ble_gap_conn_desc* desc) override
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
  void onWrite(BLECharacteristic* pCharacteristic)
  {
    if (pCharacteristic->getDataLength() <= 0) return;

    auto argsPair = new std::pair<std::string, BLECharacteristic*>(pCharacteristic->getValue(), pCharacteristic);
    xTaskCreate(
      BLEWriteTask,
      "BLEWriteTask",
      8192,
      argsPair,     // 传递参数
      1,
      NULL
    );
  }

  void onStatus(BLECharacteristic* pCharacteristic, Status s, int code) override
  {
    DEBUG_PRINTF("status: %d\n", code);
  }
};

// Function to initialize BLE
void initBLE(std::function<void()> createServer)
{

  if (strcmp_P(serverDescription, PSTR("WLED")) == 0)
  {
    char bufn[15];
    BLEDevice::init(strcat(strcpy(bufn, "WLED "), escapedMac.c_str() + 6));
  }
  else
  {
    BLEDevice::init(serverDescription);
  }
  BLEDevice::setMTU(512);
  BLEServer* pServer = BLEDevice::createServer();
  BLEService* pService = pServer->createService(padStringToUUID(stringToHex(SERVICE_UUID_NAME)).c_str());
  pCharacteristic = pService->createCharacteristic(
    padStringToUUID(stringToHex(CHARACTERISTIC_UUID_NAME)).c_str(),
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY);

  pServer->setCallbacks(new BridgeServerCallbacks());

  DEBUG_PRINTLN("BLE service and characteristic started, advertising...");
  pCharacteristic->setCallbacks(new BridgeCallbacks());
  pService->start();
  createServer();
  // Start advertising
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  // pAdvertising->addServiceUUID(padStringToUUID(stringToHex(SERVICE_UUID_NAME)).c_str());
  pAdvertising->setScanResponse(false);
  BLEDevice::startAdvertising();
}


void checkConncet() {
}

#include <M5Atom.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <iostream>

#define SERVICE_UUID "357e8779-99b3-45b7-9d92-edc98ee5c0b4"
#define WRITE_CHARACTERISTIC_UUID "e0761f40-6a05-4e08-a853-b87d67524a38"
#define READ_CHARACTERISTIC_UUID "201bcd2c-74ad-11ed-a1eb-0242ac120002"

#define PIN_COUNT 4

#define MODE 25
#define START 33
#define STOP 21
#define LIGHT 19

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristicForWrite = NULL;
BLECharacteristic *pCharacteristicForRead = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
std::string rxValue;
std::string txValue;
int buttonCount = 0;
bool bleOn = false;

int pins[PIN_COUNT] = {MODE, START, STOP, LIGHT};

class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    deviceConnected = true;
    Serial.println("onConnect");
  };
  void onDisconnect(BLEServer *pServer)
  {
    deviceConnected = false;
    Serial.println("onDisconnect");
  }
};

int invertLowHight(int pin)
{
  return digitalRead(pin) == LOW ? HIGH : LOW;
}

void pinValues(uint8_t *result)
{
  for (int i = 0; i < PIN_COUNT; i++)
  {
    result[i] = digitalRead(pins[i]);
  }
}


void writePinValues()
{
  uint8_t readValues[PIN_COUNT];
  pinValues(readValues);
  pCharacteristicForRead->setValue((uint8_t *)&readValues, PIN_COUNT);
}

class MyCharacteristicCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    Serial.println("write");
    int targetPin = pins[pCharacteristic->getValue()[0]];
    digitalWrite(targetPin, invertLowHight(targetPin));
    delay(300);
    digitalWrite(targetPin, LOW); 
    writePinValues();
  }
};

void settingPinMode()
{
  pinMode(MODE, OUTPUT);
  pinMode(START, OUTPUT);
  pinMode(STOP, OUTPUT);
  pinMode(LIGHT, OUTPUT);
}

void setup()
{
  Serial.begin(115200);
  settingPinMode();

  BLEDevice::init("TSUSHIMA Watch Winder");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristicForWrite = pService->createCharacteristic(
      WRITE_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_WRITE);
  pCharacteristicForWrite->setCallbacks(new MyCharacteristicCallbacks());
  pCharacteristicForWrite->setValue("");

  pCharacteristicForRead = pService->createCharacteristic(
      READ_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ);
  // デフォルト
  uint8_t initialValues[PIN_COUNT] = {0, 0, 0, 0};
  pCharacteristicForRead->setValue((uint8_t *)&initialValues, PIN_COUNT);

  pService->start();

  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); // iPhone接続の問題に役立つ
  pAdvertising->setMinPreferred(0x12);
  pAdvertising->start();
  BLEDevice::startAdvertising();
}


void loop()
{
  if (!deviceConnected && oldDeviceConnected)
  {
    delay(500);
    pServer->startAdvertising();
    oldDeviceConnected = deviceConnected;
  }
  if (deviceConnected && !oldDeviceConnected)
  {
    oldDeviceConnected = deviceConnected;
  }
}

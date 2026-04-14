#include <Wire.h>
#include <Arduino.h>

// I2C多路复用器地址
#define I2C_ADDR_1 0x70  // 第一个多路复用器
#define I2C_ADDR_2 0x73  // 第二个多路复用器
#define MMC5603_ADDR 0x30 // MMC5603传感器地址

// I2C总线引脚定义
#define SDA1_PIN 3
#define SCL1_PIN 8
#define SDA2_PIN 18
#define SCL2_PIN 17

// 定时控制
#define SENSOR_FREQ_HZ 120          // 传感器输出频率
#define SENSOR_PERIOD_US (1000000 / SENSOR_FREQ_HZ) // 周期微秒

uint32_t t_next = 0;

// 数据缓冲区
struct SensorData {
  uint32_t x, y, z;
} sensor_data[16];

// 第二条I2C总线实例
TwoWire I2Cone = TwoWire(1);

// 写寄存器
void writeReg(TwoWire &bus, uint8_t reg, uint8_t val) {
    bus.beginTransmission(MMC5603_ADDR);
    bus.write(reg);
    bus.write(val);
    bus.endTransmission();
}

// 读寄存器
uint8_t readReg(TwoWire &bus, uint8_t reg) {
    bus.beginTransmission(MMC5603_ADDR);
    bus.write(reg);
    bus.endTransmission(false);
    bus.requestFrom((uint8_t)MMC5603_ADDR, (uint8_t)1);
    return bus.read();
}

// 读多个寄存器
void readMulti(TwoWire &bus, uint8_t reg, uint8_t* buf, uint8_t len) {
    bus.beginTransmission(MMC5603_ADDR);
    bus.write(reg);
    bus.endTransmission(false);
    bus.requestFrom((uint8_t)MMC5603_ADDR, len);
    for (int i = 0; i < len; i++) {
        buf[i] = bus.read();
    }
}

// 启用指定通道
void selectChannel(TwoWire &bus, uint8_t addr, uint8_t channel) {
    uint8_t controlByte = 1 << channel;
    bus.beginTransmission(addr);
    bus.write(controlByte);
    bus.endTransmission();
    delayMicroseconds(5);
}

// 初始化所有传感器
void initAllSensors() {
    for (uint8_t ch = 0; ch < 8; ch++) {
        // 总线1
        selectChannel(Wire, I2C_ADDR_1, ch);
        writeReg(Wire, 0x1C, 0x03);
        writeReg(Wire, 0x1A, SENSOR_FREQ_HZ);
        writeReg(Wire, 0x1B, 0x20);    // SET
        delayMicroseconds(100);         // 等待 SET 完成
        writeReg(Wire, 0x1B, 0xA0);    // 启用连续模式
        writeReg(Wire, 0x1D, 0x10);
        delay(10);

        // 总线2
        selectChannel(I2Cone, I2C_ADDR_2, ch);
        writeReg(I2Cone, 0x1C, 0x03);
        writeReg(I2Cone, 0x1A, SENSOR_FREQ_HZ);
        writeReg(I2Cone, 0x1B, 0x20);    // SET
        delayMicroseconds(100);         // 等待 SET 完成
        writeReg(I2Cone, 0x1B, 0xA0);    // 启用连续模式
        writeReg(I2Cone, 0x1D, 0x10);
        delay(10);
    }
}

// 读取单个传感器
void readSensorData(TwoWire &bus, uint8_t mux_addr, uint8_t channel, uint32_t *x, uint32_t *y, uint32_t *z) {
    selectChannel(bus, mux_addr, channel);

    uint8_t buf[9];
    readMulti(bus, 0x00, buf, 9);

    *x = ((uint32_t)buf[0] << 12) | ((uint32_t)buf[1] << 4)  | ((uint32_t)(buf[6] & 0x0F));
    *y = ((uint32_t)buf[2] << 12) | ((uint32_t)buf[3] << 4)  | ((uint32_t)(buf[7] & 0x0F));
    *z = ((uint32_t)buf[4] << 12) | ((uint32_t)buf[5] << 4)  | ((uint32_t)(buf[8] & 0x0F));
}

// 读取所有16个传感器
void readAllSensors() {
    for (uint8_t i = 0; i < 16; i++) {
        uint8_t mux_idx = i / 8;
        uint8_t ch = i % 8;
        if (mux_idx == 0) {
            readSensorData(Wire, I2C_ADDR_1, ch, &sensor_data[i].x, &sensor_data[i].y, &sensor_data[i].z);
        } else {
            readSensorData(I2Cone, I2C_ADDR_2, ch, &sensor_data[i].x, &sensor_data[i].y, &sensor_data[i].z);
        }
    }
}

void setup() {
    Serial.begin(921600);
    Wire.begin(SDA1_PIN, SCL1_PIN, 400000);
    I2Cone.begin(SDA2_PIN, SCL2_PIN, 400000);
    delay(10);

    initAllSensors();
    t_next = micros();

    Serial.println("I2C Dual-Bus MMC5603 Initialized (120Hz)");
}

void loop() {
    if ((int32_t)(micros() - t_next) >= 0) {
        t_next += SENSOR_PERIOD_US;

        readAllSensors();

        Serial.print("F,");
        for (uint8_t i = 0; i < 16; i++) {
            Serial.print(sensor_data[i].x);
            Serial.write(',');
            Serial.print(sensor_data[i].y);
            Serial.write(',');
            Serial.print(sensor_data[i].z);
            if (i < 15) Serial.write(',');
        }
        Serial.println();
    }
}
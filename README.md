An ESP32-based multi-sensor acquisition system for MMC5603, utilizing dual I2C multiplexers to read 16 sensors. The design adopts continuous measurement mode (ODR = 120 Hz) combined with fixed-timing polling, ensuring deterministic sampling and efficient data streaming.

# 代码简介

这是一个ESP32 Arduino程序，用于**同时读取16个MMC5603磁力计传感器**的数据。

## 主要功能

**硬件配置：**
- 两个I2C多路复用器（PCA9548A）控制16个通道
- 多路复用器1地址：0x70，控制8个传感器通道
- 多路复用器2地址：0x73，控制另外8个传感器通道
- MMC5603磁力计传感器地址：0x30

**工作模式：**
- **连续测量模式**：传感器以120Hz的频率持续采样
- **自动SET/RESET**：每次测量后自动复位，提高测量精度
- **周期性读取**：主循环以8.33ms的固定周期（对应120Hz）读取所有16个传感器

**数据处理：**
- 读取每个传感器的磁场X、Y、Z三轴数据
- 将数据通过串口以**CSV格式**实时输出
- 每行为一个完整的数据帧：`F, X1, Y1, Z1, X2, Y2, Z2, ..., X16, Y16, Z16`

**应用场景：**适用于需要高频率、多通道磁场测量的应用（如3D定位、空间磁场分布测量等）

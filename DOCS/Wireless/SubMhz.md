# RFM95 (LoRa) vs RFM69 (FSK/OOK) Comparison

https://github.com/Inteform/esp32-lora-library.git

## 1. Technology & Protocol
| Feature         | RFM95 (LoRa)                          | RFM69 (FSK/OOK)                     |
|-----------------|---------------------------------------|-------------------------------------|
| **Modulation**  | LoRa (Long Range)                     | FSK/GFSK/OOK                       |
| **Protocol**    | Proprietary (Semtech LoRa)            | Standard FSK                       |
| **Best For**    | Long-range IoT (LoRaWAN)              | Short-range, high-speed            |

## 2. Frequency Bands
| Module    | Frequency Options                   |
|-----------|-------------------------------------|
| **RFM95** | 868 MHz (EU), 915 MHz (US), 433 MHz |
| **RFM69** | 433 MHz, 868 MHz, 915 MHz           |

## 3. Performance
| Metric          | RFM95              | RFM69              |
|----------------|--------------------|--------------------|
| **Max Range**  | 10+ km (rural)     | 1-2 km             |
| **Data Rate**  | 0.3–50 kbps        | 1–300 kbps         |
| **Penetration**| Excellent          | Poor               |

## 4. Power Consumption
| Scenario       | RFM95     | RFM69     |
|---------------|----------|----------|
| **TX (17dBm)**| ~120 mA  | ~100 mA  |
| **RX Mode**   | ~10 mA   | ~16 mA   |
| **Sleep Mode**| ~1 µA    | ~0.1 µA  |

## 5. Hardware
| Feature       | RFM95                | RFM69                |
|--------------|----------------------|----------------------|
| **Chipset**  | SX1276/SX1278        | RFM69HCW             |
| **SPI**      | Yes                  | Yes                  |
| **DIO Pins** | DIO0-DIO5            | DIO0-DIO5            |


## RFM69 Modulation Comparison: FSK vs OOK

| Feature       | FSK (RFM69)                  | OOK (RFM69)                |
|--------------|-----------------------------|---------------------------|
| **Complexity** | Moderate (needs sync)       | Simple (just on/off)      |
| **Data Rate** | 1–300 kbps                  | ≤10 kbps                  |
| **Range**     | 1–2 km                      | <1 km                     |
| **Power Use** | Higher (constant carrier)   | Lower (carrier off for 0) |
| **Use Cases** | Sensor networks, telemetry  | Remote controls, alarms   |
## ESP32-C6 WROOM 1

| Left Pin      | Right Pin   |
|---------------|-------------|
| 1. `GND`      | `GND`       |
| 2. `3V3`      | `IO2`       |
| 3. `EN (RST)` | `IO3`       |
| 4. `IO4`      | `TX`        |
| 5. `IO5`      | `RX`        |
| 6. `IO6`      | `IO15`      |
| 7. `IO7`      | `NC`        |
| 8. `IO0`      | `IO23`      |
| 9. `IO1`      | `IO22`      |
| 10. `IO8`     | `IO21`      |
| 11. `IO10`    | `IO20`      |
| 12. `IO11`    | `IO19`      |
| 13. `IO12`    | `IO18`      |
| 14. `IO13`    | `IO9`       |


| Boot Mode       | GPIO8 | GPIO9 |
|-----------------|-------|-------|
| `SPI Boot`      | x     | 1     |
| `Download Boot` | 1     | 0     |


## ESP32 break out

| Left_0            | Left_1        | && | Right_0  | Right_1   |
|-------------------|---------------|----|----------|-----------| 
| 1. `IO5`          | `IO4`         | && | `GND`    | `3V3`     |        
| 2. `IO7`          | `IO6`         | && | `EN`     | `TX`      |
| 3. `IO1`          | `IO0`         | && | `IO2`    | `RX`      |
| 4. `IO10`         | `IO8`         | && | `IO3`    |
| 5. `IO12 (D-)`    | `IO11`        | && | `IO23`   | `IO15`    |
| 6.                | `IO13 (D+)`   | && | `IO21`   | `IO22`    |
| 7.                |               | && | `IO19`   | `IO20`    |
| 8.                |               | && | `IO9`    | `IO18`    |
| 9.                |               | && |          |           |

USB driver:
https://ftdichip.com/drivers/

USB/Flashing page 362, page 1000
https://www.espressif.com/sites/default/files/documentation/esp32-c6_technical_reference_manual_en.pdf
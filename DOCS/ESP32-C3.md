# ESP32-C3 GPIO Functions

| GPIO   | Analog Function  | Special Functions       | Comments                     |
|--------|------------------|-------------------------|------------------------------|
| GPIO0  | ADC1_CH0         |                         | RTC GPIO                     |
| GPIO1  | ADC1_CH1         |                         | RTC GPIO                     |
| GPIO2  | ADC1_CH2         | **Strapping Pin**       | RTC GPIO                     |
| GPIO3  | ADC1_CH3         |                         | RTC GPIO                     |
| GPIO4  | ADC1_CH4         |                         | RTC GPIO                     |
| GPIO5  | ADC2_CH0         |                         | RTC GPIO                     |
| GPIO6  |                  |                         | General Purpose I/O          |
| GPIO7  |                  |                         | General Purpose I/O          |
| GPIO8  |                  | **Strapping Pin**       |                              |
| GPIO9  |                  | **Strapping Pin**       |                              |
| GPIO10 |                  |                         | General Purpose I/O          |
| GPIO11 |                  |                         | General Purpose I/O          |
| GPIO12 |                  | SPI0/SPI1               |                              |
| GPIO13 |                  | SPI0/SPI1               |                              |
| GPIO14 |                  | SPI0/SPI1               |                              |
| GPIO15 |                  | SPI0/SPI1               |                              |
| GPIO16 |                  | SPI0/SPI1               |                              |
| GPIO17 |                  | SPI0/SPI1               |                              |
| GPIO18 |                  | **USB-JTAG**            | Default for JTAG debugging   |
| GPIO19 |                  | **USB-JTAG**            | Default for JTAG debugging   |
| GPIO20 |                  |                         | General Purpose I/O          |
| GPIO21 |                  |                         | **Note**: Special functions  |

## Key Notes:
1. **Strapping Pins**: 
   - GPIO2, GPIO8, GPIO9 affect boot mode if pulled high/low at startup
2. **RTC GPIOs**: 
   - Can be used in deep sleep (GPIO0-5)
3. **SPI Pins**: 
   - GPIO12-17 can be used for either SPI0 or SPI1
4. **JTAG Pins**: 
   - GPIO18/19 are shared with USB-JTAG interface
5. **ADC Channels**:
   - ADC1 (GPIO0-4), ADC2 (GPIO5 only)
<br><br>


# ESP32-C3 Console Interface Comparison

| Feature                | UART Default                  | USB CDC                        | USB Serial/JTAG            |
|------------------------|-------------------------------|--------------------------------|----------------------------|
| **Interface**          | Hardware UART (TTL)           | Native USB CDC                 | USB + JTAG                 |
| **Pins Used**          | `GPIO16 (TX)`, `GPIO17 (RX)`  | `GPIO18/19` or `GPIO20/21`     | `GPIO18/19` (fixed)        |
| **Max Speed**          | 5 Mbps                        | 12 Mbps (Full-Speed USB)       | ~1 Mbps (shared with JTAG) |
| **Debugging**          | ❌ No JTAG                    | ❌ No JTAG                     | ✅ Full JTAG support      |
| **Driver Required**    | CH340/CP210x (USB-UART bridge)| Built-in OS CDC drivers        | ESP USB-JTAG drivers       |
| **Boot Logs**          | ✅ Early boot messages        | ❌ After USB init              | ❌ After USB init         |
| **Power Source**       | Self-powered                  | USB VBUS detection             | USB VBUS detection        |
| **Typical Use Case**   | Legacy 3.3V serial devices    | Clean USB logging              | Debugging + serial combo  |
| **GPIO Flexibility**   | ✅ Any available GPIO         | Limited to USB-capable pins    | ❌ Fixed to GPIO18/19    |
| **Driver Stability**   | ⚠️ Adapter-dependent         | ✅ Reliable (OS-native)        | ⚠️ Windows drivers needed |

## Key Notes:
1. **UART Default**:
   - Requires external USB-to-UART adapter
   - Only option for pre-USB initialization logs
2. **USB CDC**:
   - Uses ESP32-C3's native USB peripheral
   - Supports alternate pins (`GPIO20/21`) if `CONFIG_ESP_CONSOLE_SECONDARY_USB` is enabled
3. **USB Serial/JTAG**:
   - Combines serial console and JTAG debugging
   - Shares bandwidth between logging and debug traffic

> 💡 **Recommendation**: Use **USB Serial/JTAG** during development, switch to **USB CDC** for production if JTAG isn't needed.
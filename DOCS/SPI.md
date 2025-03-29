# SPI Interface

Default Pins (Hardware Pins):

| SPI	    | MOSI	        | MISO	        | SCLK	        | CS        |
|-----------|---------------|---------------|---------------|-----------|
| VSPI	    | 23	        | 19    	    | 18	        | 5         |
| HSPI	    | 13	        | 12	        | 14	        | 15        |


ESP32 SPI Peripherals: SPI0, SPI1, and SPI2 (HSPI & VSPI)
The ESP32 has three SPI controllers:

- `SPI0` (Reserved for Cache & Internal Flash) → Not usable by applications.
- `SPI1` (GPSPI: General-Purpose SPI) → Flexible but with restrictions.
- `SPI2` (HSPI & VSPI) → Dedicated for user applications (most commonly used).
<br><br><br>

# spi_bus_initialize()

In ESP-IDF, spi_bus_initialize() uses two critical parameters:
- `spi_host_device_t` → Specifies the SPI controller (hardware instance).
- `spi_dma_chan_t` → Specifies the DMA channel (for direct memory access).

Here’s the breakdown of their roles and differences:

### 1. spi_host_device_t (SPI Controller)

Purpose: Identifies which hardware SPI peripheral to use (ESP32 has multiple SPI controllers).

Options:
| Value	        | Controller	    | Notes |
|---------------|-------------------|-------|
| `SPI1_HOST`   | SPI1	            | Used for cache-access (reserved for flash/PSRAM, do not use). |
| `SPI2_HOST`   | SPI2 (HSPI)	    | General-purpose (labeled "HSPI" in datasheets). |
| `SPI3_HOST`   | SPI3 (VSPI)	    | General-purpose (labeled "VSPI" in datasheets). Default for most apps. |

Key Points:
- Avoid SPI1_HOST (reserved for internal memory).
- HSPI/VSPI are identical in functionality; pick one not used by other peripherals.

### 2. spi_dma_chan_t (DMA Channel)

Purpose: Selects the DMA channel to offload SPI transfers (reduces CPU usage).

Options:
Value	                Description
SPI_DMA_DISABLED	    No DMA (transfers block CPU).
SPI_DMA_CH_AUTO	        Let ESP-IDF pick an available DMA channel (recommended).
SPI_DMA_CH1	            Manual DMA channel 1 (rarely needed).
SPI_DMA_CH2	            Manual DMA channel 2 (conflicts with some Wi-Fi/BT operations).

Key Points:
- SPI_DMA_CH_AUTO is safest (avoids conflicts with Wi-Fi/BT).
- Disable DMA (SPI_DMA_DISABLED) for low-throughput tasks to save resources.


| Value               | Description                                                                 |
|---------------------|-----------------------------------------------------------------------------|
| `SPI_DMA_DISABLED`  | No DMA (transfers block CPU).                                               |
| `SPI_DMA_CH_AUTO`   | Let ESP-IDF pick an available DMA channel (recommended).                    |
| `SPI_DMA_CH1`       | Manual DMA channel 1 (rarely needed).                                       |
| `SPI_DMA_CH2`       | Manual DMA channel 2 (conflicts with some Wi-Fi/BT operations).             |
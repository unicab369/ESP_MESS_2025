## Make dev_config.h file

Create a `dev_config.h` file under `main/` with the following template. You can also make a copy of the `dev_config_copy.h` file and renamed it to `dev_config.h`

```
#define WIFI_SSID "My_SSID"
#define WIFI_PASSWORD "MY_PASSWORD"
#define DEVICE_NAME "Device Name"
```

## Open ESP-IDF terminal on vsCode

1. Use shortcut `Ctrl+Shift+P` to open the `Command Pallet`. Next type `terminal`, on the suggested options, select `ESP-IDF: Open ESP-IDF Terminal`.
2. Or use the shortcut `Ctrl+E`  `T` (wait for split second after `Ctrl+E` before typing `T`)

## Build the app

1. Build the project: `idf.py build` or `idf.py app build`
2. Flash the project: `idf.py flash -p COMx` if a COM port already selected then use `idf.py flash`
3. Open the monitor: `idf.py monitor`
   
Example work flow: <br>
```
idf.py app build
idf.py flash monitor -p COM4
```

## Configuration Options

show confuration menu: `idf.py menuconfig`

1. Enable/Disable deepSleep or lightSleep:
   - Component config -> Power Mamagement 
   - Enable Support for power management
   - Enable lightsleep 
2. Set custom partition table for littlefs:
   - Partition table -> Custom partition table CSV
3. Set Log level:
   - Component config -> Log -> Log Level -> Choose default log verbosity
4. Enable FreeRTOS trace facility:
   - Component config- > FreeRTOS -> Kernel
   - config USE_TRACE_FACILITY -> config USE_STATS_FORMATTING_FUNCTIONS
5. UART/CDC/JTAG
   - Component config -> ESP System Settings

Note: to clear and update the environment target and idf target, do the following<br>
```
Remove-Item Env:IDF_TARGET
set IDF_TARGET=esp32x
idf.py set-target esp32x
```

## Keyboard Shortcuts:

| Shortcuts       | Description |
| --------------- | --------- |
|Ctrl+`           | Open terminal |
|Ctrl+T           | New Tab |
|Ctrl+W           | Close tab |
|Ctrl+E T         | Open ESP-IDF Terminal |
|Ctrl+T Ctrl+X    | Close ESP-IDF Terminal |

## Suggested vsCode plugins

1. `ESP-IDF`: required for this project.
2. `Better Comments`: Commenting styles.
3. `Back & Forth`: Back and Forth button to navigate between files. 
4. `Indent-Rainbow`: Code Indent styles (enable light mode).
5. `Numbered Bookmarks`: hotkey your code line similar to Starcraft control group hotkeys.
6. `Power Mode`: Unleash your magical typing power.


## View memory usages

`idf.py size-components`: identify IRAM consumers
`heap_caps_print_heap_info(MALLOC_CAP_8BIT);`: Monitor heap fragmentation. (88KB free is healthy for most applications)

## Critical Thresholds

|Memory     | Warning Level	| Emergency Level |
|-----------|-----------------| --------------- |
|IRAM	      | >110KB	         | >125KB |
|DRAM	      | >100KB	         | >120KB |
|Flash      | >2.5MB	         | >3MB   |

## Other idf.py commands

`idf.py --version`: Displays the version of ESP-IDF being used in the current environment.<br>
`idf.py partition-table`: Prints the partition table information for the project.<br>
`idf.py show_targets`: Lists all supported target chips.<br> 
`app-flash`: Flash only the app part of the project.<br>
`erase_flash`: Erase the ESP32's entire flash chip.<br>
`reconfigure`: Re-run CMake even if it doesn't seem to need re-running.<br>
`clean`: Remove the build output.<br>
`fullclean`: Delete the entire build contents.<br>
`create-component`: Create a new component.<br>
`set-target` [esp32x]: Select the target chip.<br>
`size`: Print app size information.<br>
`size-components`: Print size information for each component.<br>
`size-files`: Print size information per source file.<br>
`partition-table-flash`: Flash only the partition table to the device.<br>
`esptool.py flash_id`: queries the connected ESP device to retrieve its flash memory ID.<br>
`idf.py check`: check for common coding issues in your project.<br>
`idf.py sdkconfig`: Opens the project's sdkconfig file in a text editor for manual editing. This is an alternative to idf.py menuconfig.<br>

## ESP32 ADC Channels

**ADC1 Channels**
| ADC1 Channel	G |   PIO Pin  |
|----------------|------------|
| ADC1_CHANNEL_0 |   GPIO 36  |  
| ADC1_CHANNEL_1 |   GPIO 37  |
| ADC1_CHANNEL_2 |	GPIO 38  |
| ADC1_CHANNEL_3 |	GPIO 39  |
| ADC1_CHANNEL_4 |	GPIO 32  |
| ADC1_CHANNEL_5 |	GPIO 33  |
| ADC1_CHANNEL_6 |	GPIO 34  |
| ADC1_CHANNEL_7 |	GPIO 35  |
<br>

**ADC2 Channels**
| ADC2 Channel	G |   PIO Pin
|----------------|------------|
| ADC2_CHANNEL_0 |	GPIO 4   |
| ADC2_CHANNEL_1 |	GPIO 0   |
| ADC2_CHANNEL_2 |	GPIO 2   |
| ADC2_CHANNEL_3 |	GPIO 15  |
| ADC2_CHANNEL_4 |	GPIO 13  |
| ADC2_CHANNEL_5 |	GPIO 12  |
| ADC2_CHANNEL_6 |	GPIO 14  |
| ADC2_CHANNEL_7 |	GPIO 27  |
| ADC2_CHANNEL_8 |	GPIO 25  |
| ADC2_CHANNEL_9 |	GPIO 26  |


## Important Notes
ADC2 and Wi-Fi:
   ADC2 cannot be used when Wi-Fi is enabled because it is shared with the Wi-Fi module.

ADC Calibration:
   The ESP32 ADC is not perfectly linear. For accurate measurements, consider using the ADC calibration feature provided by ESP-IDF.

Noise Reduction:
   To reduce noise, you can average multiple ADC readings or use a capacitor on the ADC pin.


## NimBLE vs Blueroid
   
|Feature	         | NimBLE	      | Bluedroid       |
|-----------------|--------------|-----------------|
|Memory Usage	   | ~25KB RAM 	| ~100KB RAM      |
|BLE 5.0 Support	| Full      	| Partial         |
|API Complexity	| Simpler	   | More complex    |
|Throughput	Good	| Excellent|   |                 |
|Dual Mode        | (BT+BLE)	   | BLE only	Both BT Classic and BLE |
|Configuration	   | Menuconfig   | option	Default in ESP-ID |



## Memory Type Usage Summary

| Memory Type/Section            | Used [bytes] | Used [%] | Remain [bytes] | Total [bytes] |
|--------------------------------|--------------|----------|----------------|---------------|
| `Flash Code`                   |       436078 |    13.05 |        2906226 |       3342304 |
|&nbsp;&nbsp;&nbsp;.text         |       436078 |    13.05 |                |               |
| `IRAM`                         |       115375 |    88.02 |          15697 |        131072 |
|&nbsp;&nbsp;&nbsp;.text         |       114347 |    87.24 |                |               |
|&nbsp;&nbsp;&nbsp;.vectors      |         1027 |     0.78 |                |               |
| `Flash Data`                   |        99656 |     2.38 |        4094616 |       4194272 |
|&nbsp;&nbsp;&nbsp;.rodata       |        99400 |     2.37 |                |               |
|&nbsp;&nbsp;&nbsp;.appdesc      |          256 |     0.01 |                |               |
| `DRAM`                         |        36188 |    29.05 |          88392 |        124580 |
|&nbsp;&nbsp;&nbsp;.data         |        21116 |    16.95 |                |               |
|&nbsp;&nbsp;&nbsp;.bss          |        15072 |     12.1 |                |               |
| `RTC FAST`                     |           28 |     0.34 |           8164 |          8192 |
|&nbsp;&nbsp;&nbsp;.force_fast   |           28 |     0.34 |                |               |
| `RTC SLOW`                     |           24 |     0.31 |           7656 |          7680 |
|&nbsp;&nbsp;&nbsp;.rtc_slow     |           24 |     0.31 |                |               |


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

1. Build the project: `idf.py build`
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


## List of idf.py commands

`menuconfig`: Start the graphical configuration tool.<br>
`idf.py --version`: Displays the version of ESP-IDF being used in the current environment.<br>
`idf.py partition-table`: Prints the partition table information for the project.<br>
`idf.py show_targets`: Lists all supported target chips.<br>
`build`: Build the project.<br>
`flash`: Flash the project to the target.<br>
`monitor`: Display serial output from the ESP32.<br>
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
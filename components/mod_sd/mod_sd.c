#include "mod_sd.h"

#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "sd_test_io.h"

// link: https://github.com/espressif/esp-idf/tree/master/examples/storage/sd_card

#if SOC_SDMMC_IO_POWER_EXTERNAL
#include "sd_pwr_ctrl_by_on_chip_ldo.h"
#endif

#define MAX_CHAR_SIZE    255

static const char *TAG = "STORAGE_SD";
#define MOUNT_POINT "/sdcard"


static FILE *file;

//# Open File
esp_err_t mod_sd_fopen(const char *path) {
    char full_path[MAX_CHAR_SIZE];
    // Concatenate MOUNT_POINT and the provided path
    snprintf(full_path, sizeof(full_path), "%s%s", MOUNT_POINT, path);


    ESP_LOGI(TAG, "Open file %s", path);

    file = fopen(full_path, "r");
    if (file == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return ESP_FAIL;
    }

    return ESP_OK;
}

//# Read File
size_t mod_sd_fread(char *buff, size_t len) {
    // Reads a specified number of bytes (or records) from a file.
    return fread(buff, 1, len, file);
}

int mod_sd_fclose() {
    if (file == NULL) return 0;
    ESP_LOGI(TAG, "mod_sd_fclose");
    return fclose(file);
}

//# Write File
esp_err_t mod_sd_write(const char *path, char *buff) {
    ESP_LOGI(TAG, "Opening file %s", path);

    FILE *f = fopen(path, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }
    fprintf(f, buff);
    fclose(f);
    ESP_LOGI(TAG, "File written");

    return ESP_OK;
}

esp_err_t mod_sd_get(const char *path, char *buff, size_t len) {
    ESP_LOGI(TAG, "Reading file %s", path);
    file = fopen(path, "r");
    if (file == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return ESP_FAIL;
    }

    // Reads a line of text from a file (up to a specified number of characters 
    // or until a newline is encountered).
    fgets(buff, len, file);
    fclose(file);

    // strip newline
    char *pos = strchr(buff, '\n');
    if (pos) *pos = '\0';
    ESP_LOGI(TAG, "Read from file: '%s'", buff);

    return ESP_OK;
}

static sdmmc_card_t *card;
static esp_err_t ret;
const char mount_point[] = MOUNT_POINT;


static void check_sd_card(esp_err_t ret) {
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                    "Format the card if needed before use.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                    "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
            #ifdef CONFIG_EXAMPLE_DEBUG_PIN_CONNECTIONS
                check_sd_card_pins(&config, pin_count);
            #endif
        }

        return;
    }


    //# Card has been initialized, print its properties
    ESP_LOGI(TAG, "Filesystem mounted");
    sdmmc_card_print_info(stdout, card);
}

void mod_sd_spi_config(uint8_t spi_host, uint8_t cs_pin) {
    ESP_LOGI(TAG, "Initializing SD card. Using SPI peripheral");

    // For SoCs where the SD power can be supplied both via an internal or external (e.g. on-board LDO) power supply.
    // When using specific IO pins (which can be used for ultra high-speed SDMMC) to connect to the SD card
    // and the internal LDO power supply, we need to initialize the power supply first.
    #if CONFIG_EXAMPLE_SD_PWR_CTRL_LDO_INTERNAL_IO
        sd_pwr_ctrl_ldo_config_t ldo_config = {
            .ldo_chan_id = CONFIG_EXAMPLE_SD_PWR_CTRL_LDO_IO_ID,
        };
        sd_pwr_ctrl_handle_t pwr_ctrl_handle = NULL;

        ret = sd_pwr_ctrl_new_on_chip_ldo(&ldo_config, &pwr_ctrl_handle);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to create a new on-chip LDO power control driver");
            return;
        }
        host.pwr_ctrl_handle = pwr_ctrl_handle;
    #endif

    
    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = cs_pin;
    slot_config.host_id = spi_host;

    static sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    // host.slot = spi_host;

    ESP_LOGI(TAG, "Mounting filesystem");
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    //# Mounting SD card
    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);
    check_sd_card(ret);
}



#define EXAMPLE_IS_UHS1    (CONFIG_EXAMPLE_SDMMC_SPEED_UHS_I_SDR50 || CONFIG_EXAMPLE_SDMMC_SPEED_UHS_I_DDR50)

#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"

void mod_sd_mmc_config(int8_t clk, int8_t cmd, int8_t d0, int8_t d1, int8_t d2, int8_t d3) {
    // // By default, SD card frequency is initialized to SDMMC_FREQ_DEFAULT (20MHz)
    // // For setting a specific frequency, use host.max_freq_khz (range 400kHz - 40MHz for SDMMC)
    // // Example: for fixed frequency of 10MHz, use host.max_freq_khz = 10000;
    // sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    // #if CONFIG_EXAMPLE_SDMMC_SPEED_HS
    //     host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;
    // #elif CONFIG_EXAMPLE_SDMMC_SPEED_UHS_I_SDR50
    //     host.slot = SDMMC_HOST_SLOT_0;
    //     host.max_freq_khz = SDMMC_FREQ_SDR50;
    //     host.flags &= ~SDMMC_HOST_FLAG_DDR;
    // #elif CONFIG_EXAMPLE_SDMMC_SPEED_UHS_I_DDR50
    //     host.slot = SDMMC_HOST_SLOT_0;
    //     host.max_freq_khz = SDMMC_FREQ_DDR50;
    // #endif

    // // For SoCs where the SD power can be supplied both via an internal or external (e.g. on-board LDO) power supply.
    // // When using specific IO pins (which can be used for ultra high-speed SDMMC) to connect to the SD card
    // // and the internal LDO power supply, we need to initialize the power supply first.
    // #if CONFIG_EXAMPLE_SD_PWR_CTRL_LDO_INTERNAL_IO
    //     sd_pwr_ctrl_ldo_config_t ldo_config = {
    //         .ldo_chan_id = CONFIG_EXAMPLE_SD_PWR_CTRL_LDO_IO_ID,
    //     };
    //     sd_pwr_ctrl_handle_t pwr_ctrl_handle = NULL;

    //     ret = sd_pwr_ctrl_new_on_chip_ldo(&ldo_config, &pwr_ctrl_handle);
    //     if (ret != ESP_OK) {
    //         ESP_LOGE(TAG, "Failed to create a new on-chip LDO power control driver");
    //         return;
    //     }
    //     host.pwr_ctrl_handle = pwr_ctrl_handle;
    // #endif

    // // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    // sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    // #if EXAMPLE_IS_UHS1
    //     slot_config.flags |= SDMMC_SLOT_FLAG_UHS1;
    // #endif

    // //! Note that even if card's D3 line is not connected to the ESP chip, 
    // //! it still has to be pulled up, otherwise the card will go into SPI protocol mode.
    // slot_config.clk = clk;
    // slot_config.cmd = cmd;
    // slot_config.d0 = d0;
    // slot_config.width = 1;

    // // if (config->mmc.enable_width4) {
    // //     slot_config.d1 = config->mmc.d1;
    // //     slot_config.d2 = config->mmc.d2;
    // //     slot_config.d3 = config->mmc.d3;
    // //     slot_config.width = 4;
    // // }

    // // Enable internal pullups on enabled pins. The internal pullups
    // // are insufficient however, please make sure 10k external pullups are
    // // connected on the bus. This is for debug / example purpose only.
    // slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    // ESP_LOGI(TAG, "Mounting filesystem");
    // esp_err_t ret;
    // esp_vfs_fat_sdmmc_mount_config_t mount_config = {
    //     .format_if_mount_failed = false,
    //     .max_files = 5,
    //     .allocation_unit_size = 16 * 1024
    // };

    // ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);
    // check_sd_card(ret);
}


//# Format Card
void storage_sd_format_card() {
    ret = esp_vfs_fat_sdcard_format(mount_point, card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to format FATFS (%s)", esp_err_to_name(ret));
        return;
    }

    struct stat st;
    const char *file_foo = MOUNT_POINT"/foo.txt";

    if (stat(file_foo, &st) == 0) {
        ESP_LOGI(TAG, "file still exists");
        return;
    } else {
        ESP_LOGI(TAG, "file doesn't exist, formatting done");
    }
}

//# Unmount card
void mod_sd_deinit(spi_host_device_t slot) {
    //! unmount partition and disable SPI peripheral
    esp_vfs_fat_sdcard_unmount(mount_point, card);
    ESP_LOGI(TAG, "Card unmounted");

    //! deinitialize the bus after all devices are removed
    spi_bus_free(slot);
}

void mod_sd_test(void) {
    if (ret != ESP_OK) return;
    
    //# First create a file.
    const char *file_hello = MOUNT_POINT"/hello.txt";
    char data[MAX_CHAR_SIZE];
    printf("card name: %s", card->cid.name);

    snprintf(data, MAX_CHAR_SIZE, "%s %s!\n", "Hello", card->cid.name);
    ret = mod_sd_write(file_hello, data);
    if (ret != ESP_OK) return;


    //# Check if destination file exists before renaming
    const char *file_foo = MOUNT_POINT"/foo2.txt";
    struct stat st;
    if (stat(file_foo, &st) == 0) {
        // Delete it if it exists
        unlink(file_foo);
    }

    //# Rename original file
    ESP_LOGI(TAG, "Renaming file %s to %s", file_hello, file_foo);
    if (rename(file_hello, file_foo) != 0) {
        ESP_LOGE(TAG, "Rename failed");
        return;
    }

    char line[MAX_CHAR_SIZE];
    ret = mod_sd_get(file_foo, line, sizeof(line));
    if (ret != ESP_OK) return;

    const char *file_nihao = MOUNT_POINT"/nihao.txt";
    memset(data, 0, MAX_CHAR_SIZE);
    snprintf(data, MAX_CHAR_SIZE, "%s %s!\n", "Nihao", card->cid.name);
    ret = mod_sd_write(file_nihao, data);
    if (ret != ESP_OK) return;

    //# Open file for reading
    ret = mod_sd_get(file_foo, line, sizeof(line));
    if (ret != ESP_OK) return;

    //     // Deinitialize the power control driver if it was used
    // #if CONFIG_EXAMPLE_SD_PWR_CTRL_LDO_INTERNAL_IO
    //     ret = sd_pwr_ctrl_del_on_chip_ldo(pwr_ctrl_handle);
    //     if (ret != ESP_OK) {
    //         ESP_LOGE(TAG, "Failed to delete the on-chip LDO power control driver");
    //         return;
    //     }
    // #endif
}

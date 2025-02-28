#include "littlefs.h"
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "esp_log.h"
#include "esp_err.h"
#include "esp_littlefs.h"
#include <dirent.h>

static const char *TAG = "LITTLEFS";

FILE *f;

esp_vfs_littlefs_conf_t conf = {
    .base_path = "/littlefs",
    .partition_label = "storage",
    .format_if_mount_failed = true,
    .dont_mount = false,
};

void littlefs_setup() {
    ESP_LOGI(TAG, "Initializing LittleFS");

    // Use settings defined above to initialize and mount LittleFS filesystem.
    // Note: esp_vfs_littlefs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_littlefs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find LittleFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize LittleFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_littlefs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get LittleFS partition information (%s)", esp_err_to_name(ret));
        esp_littlefs_format(conf.partition_label);
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
}

static void read_file_content(const char* path, char* file_name, littlefs_readfile_cb callback) {
    // Construct the full file path
    char file_path[512];
    snprintf(file_path, sizeof(file_path), "%s/%s", path, file_name);
    ESP_LOGI("TAG", "Read file: %s", file_path);

    FILE* file = fopen(file_path, "r");
    if (file == NULL) {
        ESP_LOGE("TAG", "Failed to open file: %s", file_path);
        return;
    }

    char buff[128]; // Buffer to store file content
    while (fgets(buff, sizeof(buff), file) != NULL) {
        callback(file_name, buff, sizeof(buff));
    }

    fclose(file);
}


void littlefs_loadFiles(const char* path, littlefs_readfile_cb callback) {
    // Open the directory
    DIR* dir = opendir(path);
    if (dir == NULL) {
        ESP_LOGE("TAG", "Failed to open directory: %s", path);
        return;
    }

    // Read directory entries
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip directories
        if (entry->d_type == DT_DIR) continue;
        read_file_content(path, entry->d_name, callback);
    }

    // Close the directory
    closedir(dir);
}

void littlefs_writeFile(const char* path, const char*) {
    ESP_LOGI(TAG, "Opening file");
    f = fopen(path, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
    }
    fprintf(f, "Hello World!\n");
    fclose(f);
    ESP_LOGI(TAG, "File written");

    // Check if destination file exists before renaming
    struct stat st;
    if (stat("/littlefs/foo.txt", &st) == 0) {
        // Delete it if it exists
        unlink("/littlefs/foo.txt");
    }

    // Rename original file
    ESP_LOGI(TAG, "Renaming file");
    if (rename("/littlefs/hello.txt", "/littlefs/foo.txt") != 0) {
        ESP_LOGE(TAG, "Rename failed");
        return;
    }
}

void littlefs_readFile(const char* path) {
    // Open renamed file for reading
    ESP_LOGI(TAG, "Reading file %s", path);
    f = fopen(path, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return;
    }

    char line[128] = {0};
    char* pos = strpbrk(line, "\r\n");          // strip newline
    fgets(line, sizeof(line), f);
    fclose(f);

    if (pos) *pos = '\0';
    ESP_LOGI(TAG, "Read from file: '%s'", line);
}

void littlefs_test() {
    littlefs_writeFile("/littlefs/hello.txt", "Hello World!\n");
    littlefs_readFile("/littlefs/foo.txt");
    littlefs_readFile("/littlefs/example.txt");

    // All done, unmount partition and disable LittleFS
    esp_vfs_littlefs_unregister(conf.partition_label);
    ESP_LOGI(TAG, "LittleFS unmounted");
}
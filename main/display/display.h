
#include <unistd.h>
#include <stdint.h>

void display_setup(uint8_t scl_pin, uint8_t sda_pin, uint8_t address);
void display_print_str(const char *str, uint8_t line);
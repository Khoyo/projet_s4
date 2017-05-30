#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <linux/random.h>
#define SUM 0xc6ef3720
#define DELTA 0x9e3779b9

int tea_encrypt(uint8_t* data, uint32_t len, uint32_t* key);

int tea_decrypt(uint8_t* data, uint32_t len, uint32_t* key);

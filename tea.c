#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <linux/random.h>

#define SUM 0xc6ef3720
#define DELTA 0x9e3779b9

/* encrypt()
 * Encrypt 64 bits (2 * 32 bits array) with a 128 bits key
 * (4 * 32 bits array)
 *
 * IN/OUT v : array of two 32 bit uints to be encoded in place
 * IN     k : array of four 32 bit uints to act as key */
void tea_encrypt_one_block(uint32_t* v, uint32_t* k)
{
  uint32_t v0 = v[0];
  uint32_t v1 = v[1];
  uint32_t sum = 0;

  uint32_t delta = DELTA;
  uint32_t k0 = k[0];
  uint32_t k1 = k[1];
  uint32_t k2 = k[2];
  uint32_t k3 = k[3];

  for (uint32_t i = 0 ; i < 32 ; i++)
  {
    sum += delta;
    v0 += ((v1 << 4) + k0) ^ (v1 + sum) ^ ((v1 >> 5) + k1);
    v1 += ((v0 << 4) + k2) ^ (v0 + sum) ^ ((v0 >> 5) + k3);
  }

  v[0] = v0;
  v[1] = v1;
}

/* decrypt()
 * Decrypt 64 bits (2 * 32 bits array) with a 128 bits key
 * (4 * 32 bits array)
 *
 * IN/OUT v : array of two 32 bit uints to be decoded in place
 * IN     k : array of four 32 bit uints to act as key */
void tea_decrypt_one_block(uint32_t* v, uint32_t* k)
{
  uint32_t v0 = v[0];
  uint32_t v1 = v[1];
  uint32_t sum = SUM;
  uint32_t delta = DELTA;

  uint32_t k0 = k[0];
  uint32_t k1 = k[1];
  uint32_t k2 = k[2];
  uint32_t k3 = k[3];

  for (uint32_t i = 0 ; i < 32 ; i++)
  {
    v1 -= ((v0 << 4) + k2) ^ (v0 + sum) ^ ((v0 >> 5) + k3);
    v0 -= ((v1 << 4) + k0) ^ (v1 + sum) ^ ((v1 >> 5) + k1);
    sum -= delta;
  }

  v[0] = v0;
  v[1] = v1;
}

int tea_encrypt(uint8_t* data, uint32_t len, uint32_t* key)
{
  if (len % 8 != 0)
    return -1;

  uint32_t* data32 = (uint32_t*) data;
  uint32_t blocks = len / 8;

  for (uint32_t i = 0 ; i < blocks ; i++)
  {
    tea_encrypt_one_block(&data32[i*2], key);
  }

  return 0;
}

int tea_decrypt(uint8_t* data, uint32_t len, uint32_t* key)
{
  if (len % 8 != 0)
    return -1;

  uint32_t* data32 = (uint32_t *) data;
  uint32_t blocks = len / 8;

  for (uint32_t i = 0 ; i < blocks ; i++)
  {
    tea_decrypt_one_block(&data32[i*2], key);
  }

  return 0;
}

static void print_array(uint8_t* array, size_t len)
{
  for (size_t i = 0 ; i < len ; i++)
  {
    printf("%c", array[i]);
  }
  printf("\n");
}

static void print_key(uint32_t* key, uint32_t len)
{
  for (size_t i = 0 ; i < len ; i++)
  {
    printf("%d;", key[i]);
  }
  printf("\n");
}

/*
int main()
{
  unsigned char data[16] = "aaaabbbbaaaabbb";
  uint32_t key[4] = {134123, 765254, 345387, 987445};

  printf("Key : ");
  print_key(key, 4);

  printf("Message : ");
  print_array(data, 16);

  tea_encrypt(data, 16, key);

  printf("Encrypted message : ");
  print_array(data, 16);
  tea_decrypt(data, 16, key);

  printf("Decrypted message : ");
  print_array(data, 16);

  return 0;
}
*/

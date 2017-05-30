#define _XOPEN_SOURCE
#include <stdio.h>
#include <gmp.h>
#include <memory.h>
#include <stdlib.h>
#include <sys/random.h>
#include <unistd.h>

void secure_random_num(mpz_t n, size_t size, int prime)
{
  char buffer[size];
  getrandom(buffer, size, 0);
  if(prime)
  {
    buffer[0] |= 1 << 8; // Only use big prime numbers
    buffer[size - 1] |= 1 >> 8;
  }
  // Endianness doesn't matter, but the word order is important for primes
  mpz_import(n, size, 1, 1, 0, 0, buffer);
  // Zero out buffer
  memset(buffer, 0, size);
}

// size is in bits, must be a multiple of 8
int generate_prime(mpz_t res, size_t size)
{
  int i = 1;
  do
  {
    secure_random_num(res, size/8, 1);
    i++;
  } while (!mpz_probab_prime_p(res, 50));
  return i;
}

void generate_rsa_key(mpz_t n, mpz_t e, mpz_t d)
{
  mpz_t p, q, lambda, gcd;
  mpz_init(p);
  mpz_init(q);
  mpz_init(lambda);
  mpz_init(gcd);

  mpz_set_ui(e, 65537);

  do {
    do {
      generate_prime(p, 1024);
      generate_prime(q, 1024);

      mpz_mul(n, p, q);

      mpz_sub_ui(p, p, 1u);
      mpz_sub_ui(p, q, 1u);
      mpz_lcm(lambda, p, q);
      mpz_gcd(gcd, e, lambda);
    } while(mpz_cmp_ui(gcd, 1) != 0);

  } while(mpz_invert(d, e, lambda) == 0);

  mpz_clear(p);
  mpz_clear(q);
  mpz_clear(lambda);
  mpz_clear(gcd);
}

void print_key(char* filename, mpz_t n, mpz_t e_or_d)
{
  FILE* f = fopen(filename, "w");
  gmp_fprintf(f, "%#Zx\n", n);
  gmp_fprintf(f, "%#Zx\n", e_or_d);
  fclose(f);
}

void read_key(char* filename, mpz_t n, mpz_t e_or_d)
{
  FILE* f = fopen(filename, "r");
  gmp_fscanf(f, "%#Zx\n", n);
  gmp_fscanf(f, "%#Zx\n", e_or_d);
  fclose(f);
}

struct rsa_payload
{
  size_t size;
  size_t capacity;
  char* data;
};

struct rsa_payload new_rsa_payload()
{
  return (struct rsa_payload) {0, 1024, malloc(1024)};
}

void free_rsa_payload(struct rsa_payload* p)
{
  p->size = 0;
  p->capacity = 0;
  free(p->data);
  p->data = NULL;
}

void add_data_to_rsa_payload(struct rsa_payload* p, char* data, size_t len)
{
  if(p->capacity < (p->size + len))
  {
    p->data = realloc(p->data, (p->size + len)*2);
    p->capacity = (p->size + len) * 2;
  }
  memcpy(p->data + p->size, data, len);
  p->size += len;
}

void rsa_crypt_block(char* buffer, size_t size, char** out, size_t* out_size, mpz_t n, mpz_t e)
{
  mpz_t m;
  mpz_t c;
  mpz_init(m);
  mpz_init(c);
  mpz_import(n, size, 1, 1, 0, 0, buffer);
  mpz_powm(c, m, e, n);
  *out = mpz_export(NULL, out_size, 1, 1, 0, 0, c);
  mpz_clear(c);
  mpz_clear(m);
}

struct rsa_payload rsa_crypt(char* buffer, size_t size, mpz_t n, mpz_t e)
{
  size_t i = 0;

  struct rsa_payload payload = new_rsa_payload();

  for (; i + 1024 < size; i += 1024)
  {
    size_t tmp;
    char* new_cipher;
    rsa_crypt_block(buffer + i, 1024, &new_cipher, &tmp, n, e);
    add_data_to_rsa_payload(&payload, new_cipher, tmp);
  }
  if(i < size)
  {
    size_t tmp;
    char* new_cipher;
    rsa_crypt_block(buffer+i, size - i, &new_cipher, &tmp, n, e);
    add_data_to_rsa_payload(&payload, new_cipher, tmp);
  }
  return payload;
}


char* key_fingerprint(mpz_t n, mpz_t e)
{
  char* key;
  char salt[] = "$5$$";
  gmp_asprintf(&key, "%#Zx\n%#Zx\n", n, e);
  printf("%s", key);
  char* res = crypt(key, salt);
  printf("Your key fingerprint is %s\n", res + 4);
  free(key);
  return key;
}



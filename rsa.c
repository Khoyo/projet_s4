#include <stdio.h>
#include <gmp.h>
#include <memory.h>
#include <sys/random.h>

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


int main()
{
  mpz_t n, d, e;
  mpz_init(n);
  mpz_init(e);
  mpz_init(d);
  generate_rsa_key(n, e, d);
  mpz_out_str(stdout, 16, n);
  printf("\n");
  printf("\n");
  printf("\n");
  printf("\n");
  mpz_out_str(stdout, 16, d);
  printf("\n");
  print_key("id_rsa", n, e);
  print_key("id_rsa.pub", n, d);

  mpz_clear(n);
  mpz_clear(e);
  mpz_clear(d);
}


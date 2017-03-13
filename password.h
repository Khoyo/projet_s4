#define _XOPEN_SOURCE
#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <crypt.h>
#include <errno.h>

#define SALT_SIZE 16
#define PWD_SIZE 100

/* pwd_get_hash()
 * returns the hash of the given password
 *
 * IN username  : name of the user, associated with the hash
 *                (16 char max, must not contains '$')
 * IN clear_pwd : password of the user, not encrypted yet
 *
 * return value : hash of the password */
char* pwd_get_hash(const char* username, const char* clear_pwd);

/* pwd_add_new_hash_in_file()
 * add a new hash in the hash_file pointed by the path
 *
 * IN hash_file_path : str which contains the path to the hash_file
 *                     (aka the file which contains hashes)
 * IN new_hash       : hash to add to the hash_file
 *
 * return value      : -1 if error
 *                      0 else */
int pwd_add_new_hash_in_file(const char* hash_file_path, char* new_hash);

/* pwd_get_hash_from_file()
 * returns the hash which correspond to the given username
 *
 * IN hash_file_path : path to the hash_file
 * IN username       : username which correspond to the hash that we search
 *
 * return value      : NULL if error or not found
 *                     hash else */
char* pwd_get_hash_from_file(const char* hash_file_path, const char* username);

/* pwd_demo()
 * Ask in stdin username and password, search for a matching username in
 * hashfile, and compare the hash of the given pwd to the hash in the file
 *
 * return value : -1 if error
 *                 0 if login failed
 *                 1 if login success */
int pwd_demo();





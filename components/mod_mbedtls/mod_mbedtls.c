
#include <stdio.h>
#include <string.h>
#include "mbedtls/pk.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"

static const char *private_key_pem = 
"-----BEGIN RSA PRIVATE KEY-----\n"
"MIICXQIBAAKBgQC2zPe8xSIdRzWm44lYMIsH4/QIE5Z1fNrhW1Wp3uorLUAL9sZa\n"
"eXkUvIQvUm3RvhzgAwM89o+0d3M0gRvaSHwgWxM70GwbhJL5I20IJRKICsipOli5\n"
"FVBuDOgy8gIlCvDJx2ykZMje/kuNpUGRZEt6lHCsW0NIcqSsi0iZN6VE4wIDAQAB\n"
"AoGBAJzV1mdbKx27jsiUx62mWGJ4rVKQm7JJYPGgjyqjWn2Y4S5il2PgfIXSCAch\n"
"zZ76YYPAkfKoCDtpkKona2IodnVGhG5R098MLDDLCwe7oKZXXBAJ5BxYnUjG5wFO\n"
"o3foCoBSY4nkK2GM7zUVfoFmxQ4hC986yrg4iGHyuauuyHLBAkEA8raqbZsa0bBk\n"
"byi/ZTt8YPrv3Qe6CTTaJKebxQJofy+X4GRDk5cTSGMvSjXLvleXLFfAxt14Eg9I\n"
"tIGMemnoOwJBAMDOsiy8fhXQVBfP3Pq8B2IvTezzSZ7QDE2VHAWMl2WEgyiO/y+w\n"
"RGpYLaxFfVMGNH5AcCXTj7Ye25owe9iMc3kCQQDhjT0OBHDE4r5CysQqqEAqw6e6\n"
"aN7KhXIfVB8e0uEXKLxkC+j8tA14XKvqs9l2NmHHFHmSlOdrKQbwr99DNz5XAkA1\n"
"YENlLR1+rnjJSsszQqCMpuncwhFMfO4bAD+Mrbov6Xph3Qx6SEL4acbjHS2vsVUQ\n"
"6ofhgOsVEP/cdLWkYmWJAkBZluV23+7InsKoKTLtpw6lTRHrqWdouLfCw0qyFoDk\n"
"BRdWtg0M2RXbclkUwYiWxNeybkjqKmRMV1yRw75KiUhz\n"
"-----END RSA PRIVATE KEY-----\n";

static mbedtls_pk_context pk;
mbedtls_ctr_drbg_context ctr_drbg;

static int handle_error(int ret, const char *msg) {
    if (ret == 0) return ret;
    char error_buf[100];
    mbedtls_strerror(ret, error_buf, sizeof(error_buf));
    printf("%s: -0x%04X - %s\n", msg, -ret, error_buf);

    // Clean up
    mbedtls_pk_free(&pk);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    return ret;
}

void mod_mbedtls_setup(void) { 
    mbedtls_entropy_context entropy;   
    int ret;
    char error_buf[200];

    mbedtls_pk_init(&pk);
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    // Seed RNG (Random Number Generator)
    ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0);
    if (handle_error(ret, "Failed to seed RNG") != 0) return;

    // Parse the private key
    ret = mbedtls_pk_parse_key(&pk, 
        (const unsigned char*)private_key_pem,
        strlen(private_key_pem) + 1, NULL, 0,
        mbedtls_ctr_drbg_random, &ctr_drbg);
    if (handle_error(ret, "Failed to parse private key") != 0) return;
    printf("Private key parsed successfully!\n");

    // Use the key for encryption and decryption
    unsigned char plaintext[] = "Hello, RSA!";
    unsigned char ciphertext[256];
    unsigned char decrypted[256];
    size_t plaintext_len = strlen((char *)plaintext);
    size_t ciphertext_len, decrypted_len;

    // Encrypt the plaintext
    ret = mbedtls_pk_encrypt(&pk, plaintext, plaintext_len,
                            ciphertext, &ciphertext_len, sizeof(ciphertext),
                            mbedtls_ctr_drbg_random, &ctr_drbg);
    if (handle_error(ret, "Encryption failed") != 0) return;
    printf("Encryption successful!\n");

    // Decrypt the ciphertext
    ret = mbedtls_pk_decrypt(&pk, ciphertext, ciphertext_len,
                            decrypted, &decrypted_len, sizeof(decrypted),
                            mbedtls_ctr_drbg_random, &ctr_drbg);
    if (handle_error(ret, "Decryption failed") != 0) return;                        
    decrypted[decrypted_len] = '\0'; // Null-terminate the decrypted string
    printf("Decrypted: %s\n", decrypted);
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/core_names.h>

void ensure_keys_directory()
{
    struct stat st = {0};
    if (stat("keys", &st) == -1)
        mkdir("keys", 0700);
}

int generate_keypair_for_port(int port)
{
    EVP_PKEY_CTX *ctx = NULL;
    EVP_PKEY *pkey = NULL;

    char private_path[64];
    char public_path[64];

    snprintf(private_path, sizeof(private_path),
             "keys/%d_private.pem", port);
    snprintf(public_path, sizeof(public_path),
             "keys/%d_public.pem", port);

    /* Create context for RSA key generation */
    ctx = EVP_PKEY_CTX_new_from_name(NULL, "RSA", NULL);
    if (!ctx)
    {
        printf("Failed to create context\n");
        return 0;
    }

    if (EVP_PKEY_keygen_init(ctx) <= 0)
    {
        printf("Keygen init failed\n");
        EVP_PKEY_CTX_free(ctx);
        return 0;
    }

    /* Set key size to 2048 */
    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 2048) <= 0)
    {
        printf("Failed to set RSA bits\n");
        EVP_PKEY_CTX_free(ctx);
        return 0;
    }

    /* Generate key */
    if (EVP_PKEY_keygen(ctx, &pkey) <= 0)
    {
        printf("Key generation failed\n");
        EVP_PKEY_CTX_free(ctx);
        return 0;
    }

    EVP_PKEY_CTX_free(ctx);

    /* Write private key */
    FILE *fp = fopen(private_path, "w");
    if (!fp)
    {
        printf("Cannot open private key file\n");
        EVP_PKEY_free(pkey);
        return 0;
    }

    PEM_write_PrivateKey(fp, pkey, NULL, NULL, 0, NULL, NULL);
    fclose(fp);

    /* Write public key */
    fp = fopen(public_path, "w");
    if (!fp)
    {
        printf("Cannot open public key file\n");
        EVP_PKEY_free(pkey);
        return 0;
    }

    PEM_write_PUBKEY(fp, pkey);
    fclose(fp);

    EVP_PKEY_free(pkey);

    printf("Generated keys for port %d\n", port);
    return 1;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: %s <port1> <port2> ...\n", argv[0]);
        return 1;
    }

    ensure_keys_directory();

    for (int i = 1; i < argc; i++)
    {
        int port = atoi(argv[i]);
        if (port > 0)
            generate_keypair_for_port(port);
    }

    printf("Key generation complete.\n");
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/err.h>

#include "signature.h"

// binary to hex string
void bin_to_hex(const unsigned char *bin, size_t len, char *hex)
{
    for (size_t i = 0; i < len; i++)
        sprintf(hex + (i * 2), "%02x", bin[i]);

    hex[len * 2] = '\0';
}

// hex string to binary
int hex_to_bin(const char *hex, unsigned char *bin)
{
    size_t len = strlen(hex);
    if (len % 2 != 0)
        return 0;

    for (size_t i = 0; i < len / 2; i++)
        sscanf(hex + 2 * i, "%2hhx", &bin[i]);

    return len / 2;
}

// sign data using OpenSSL EVP
int sign_data(const char *data,
              const char *private_key_path,
              char signature_hex[513])
{
    FILE *fp = fopen(private_key_path, "r");
    if (!fp)
    {
        printf("[CRYPTO] Private key file not found: %s\n", private_key_path);
        return 0;
    }

    EVP_PKEY *pkey = PEM_read_PrivateKey(fp, NULL, NULL, NULL);
    fclose(fp);

    if (!pkey)
    {
        printf("[CRYPTO] Failed to load private key.\n");
        return 0;
    }

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx)
    {
        EVP_PKEY_free(pkey);
        return 0;
    }

    if (EVP_DigestSignInit(ctx, NULL, EVP_sha256(), NULL, pkey) <= 0)
    {
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return 0;
    }

    if (EVP_DigestSignUpdate(ctx, data, strlen(data)) <= 0)
    {
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return 0;
    }

    size_t sig_len;
    EVP_DigestSignFinal(ctx, NULL, &sig_len);

    unsigned char *sig = malloc(sig_len);
    if (!sig)
    {
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return 0;
    }

    if (EVP_DigestSignFinal(ctx, sig, &sig_len) <= 0)
    {
        free(sig);
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return 0;
    }

    bin_to_hex(sig, sig_len, signature_hex);

    free(sig);
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);

    return 1;
}

// verify signature using OpenSSL EVP
int verify_signature(const char *data,
                     const char *public_key_path,
                     const char *signature_hex)
{
    FILE *fp = fopen(public_key_path, "r");
    if (!fp)
    {
        printf("[CRYPTO] Public key file not found: %s\n", public_key_path);
        return 0;
    }

    EVP_PKEY *pkey = PEM_read_PUBKEY(fp, NULL, NULL, NULL);
    fclose(fp);

    if (!pkey)
    {
        printf("[CRYPTO] Failed to load public key.\n");
        return 0;
    }

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx)
    {
        EVP_PKEY_free(pkey);
        return 0;
    }

    if (EVP_DigestVerifyInit(ctx, NULL, EVP_sha256(), NULL, pkey) <= 0)
    {
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return 0;
    }

    if (EVP_DigestVerifyUpdate(ctx, data, strlen(data)) <= 0)
    {
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return 0;
    }

    unsigned char sig_bin[512];
    int sig_len = hex_to_bin(signature_hex, sig_bin);

    if (sig_len <= 0)
    {
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return 0;
    }

    int result = EVP_DigestVerifyFinal(ctx, sig_bin, sig_len);

    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);

    return result == 1;
}

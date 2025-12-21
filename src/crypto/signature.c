#include <string.h>
#include <stdio.h>
#include "hash.h"
#include "signature.h"

void sign_data(const char *data, const char *private_key, char signature[65]) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%s%s", data, private_key);
    sha256(buffer, signature);
}

int verify_signature(const char *data, const char *public_key, const char *signature) {
    char expected[65];
    char buffer[256];

    snprintf(buffer, sizeof(buffer), "%s%s", data, public_key);
    sha256(buffer, expected);

    return strcmp(expected, signature) == 0;
}

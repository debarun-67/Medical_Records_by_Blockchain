#ifndef SIGNATURE_H
#define SIGNATURE_H

void sign_data(const char *data, const char *private_key, char signature[65]);
int verify_signature(const char *data, const char *public_key, const char *signature);

#endif

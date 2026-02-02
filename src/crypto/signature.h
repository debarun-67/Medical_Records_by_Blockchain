#ifndef SIGNATURE_H
#define SIGNATURE_H

int sign_data(const char *data,
              const char *private_key_path,
              char signature_hex[513]);

int verify_signature(const char *data,
                     const char *public_key_path,
                     const char *signature_hex);

#endif

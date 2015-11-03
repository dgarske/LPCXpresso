#include <stdint.h>
extern uint32_t xTaskGetTickCount(void);

/* Configuration */
#ifndef FREERTOS
#define FREERTOS
#endif
#define WOLFSSL_LWIP
#define WOLFSSL_USER_IO
#define WOLFSSL_GENERAL_ALIGNMENT 4
#define WOLFSSL_SMALL_STACK
#define WOLFSSL_BASE64_ENCODE
#define WOLFSSL_SHA512

#define HAVE_ECC
#define HAVE_AESGCM
#define HAVE_CURVE25519
#define HAVE_HKDF
#define HAVE_HASHDRBG
#define HAVE_CHACHA
#define HAVE_POLY1305
#define HAVE_ONE_TIME_AUTH
#define HAVE_TLS_EXTENSIONS
#define HAVE_SUPPORTED_CURVES
#define HAVE_ERRNO_H
#define HAVE_LWIP_NATIVE
#define HAVE_SHA512

#define FP_MAX_BITS              4096
#define FP_MAX_BITS_ECC          512
#define ALT_ECC_SIZE
#define FP_LUT                   4
#define USE_FAST_MATH
#define SMALL_SESSION_CACHE
#define CURVED25519_SMALL
#define TFM_TIMING_RESISTANT
//#define TFM_ARM
#define RSA_LOW_MEM
#define GCM_SMALL
#define ECC_SHAMIR
#define USE_SLOW_SHA2

/* Remove Features */
#define NO_DEV_RANDOM
#define NO_FILESYSTEM
#define NO_WRITEV
#define NO_MAIN_DRIVER
#define NO_WOLFSSL_MEMORY
#define NO_DEV_RANDOM
#define NO_MD4
#define NO_RABBIT
#define NO_HC128
#define NO_DSA
#define NO_PWDBASED
#define NO_PSK
#define NO_64BIT
#define NO_WOLFSSL_SERVER
#define NO_OLD_TLS
#define ECC_USER_CURVES // Disables P-112, P-128, P-160, P-192, P-224, P-384, P-521 but leaves P-256 enabled
#define NO_DES3
#define NO_MD5
#define NO_RC4
#define NO_DH
#define NO_SHA  // Can't disable SHA because it's needed for OCSP

/* HW Crypto Acceleration */
// See README.md for instructions

/* Benchmark */
#define BENCH_EMBEDDED
#define USE_CERT_BUFFERS_2048

/* Custom functions */
extern uint32_t Chip_OTP_GenRand(void);
#define CUSTOM_RAND_GENERATE Chip_OTP_GenRand
#define WOLFSSL_USER_CURRTIME

/* Debugging - Optional */
#if 0
#define fprintf(file, format, ...)   printf(format, ##__VA_ARGS__)
#define DEBUG_WOLFSSL
#endif
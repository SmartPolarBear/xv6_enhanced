cmake_minimum_required(VERSION 3.22)

add_subdirectory(port)

# crypto
target_sources(kernel
        PRIVATE mbedtls/library/aes.c
        PRIVATE mbedtls/library/aesni.c
        PRIVATE mbedtls/library/arc4.c
        PRIVATE mbedtls/library/aria.c
        PRIVATE mbedtls/library/asn1parse.c
        PRIVATE mbedtls/library/asn1write.c
        PRIVATE mbedtls/library/base64.c
        PRIVATE mbedtls/library/bignum.c
        PRIVATE mbedtls/library/blowfish.c
        PRIVATE mbedtls/library/camellia.c
        PRIVATE mbedtls/library/ccm.c
        PRIVATE mbedtls/library/chacha20.c
        PRIVATE mbedtls/library/chachapoly.c
        PRIVATE mbedtls/library/cipher.c
        PRIVATE mbedtls/library/cipher_wrap.c
        PRIVATE mbedtls/library/constant_time.c
        PRIVATE mbedtls/library/cmac.c
        PRIVATE mbedtls/library/ctr_drbg.c
        PRIVATE mbedtls/library/des.c
        PRIVATE mbedtls/library/dhm.c
        PRIVATE mbedtls/library/ecdh.c
        PRIVATE mbedtls/library/ecdsa.c
        PRIVATE mbedtls/library/ecjpake.c
        PRIVATE mbedtls/library/ecp.c
        PRIVATE mbedtls/library/ecp_curves.c
        PRIVATE mbedtls/library/entropy.c
        PRIVATE mbedtls/library/entropy_poll.c
        PRIVATE mbedtls/library/error.c
        PRIVATE mbedtls/library/gcm.c
        PRIVATE mbedtls/library/havege.c
        PRIVATE mbedtls/library/hkdf.c
        PRIVATE mbedtls/library/hmac_drbg.c
        PRIVATE mbedtls/library/md.c
        PRIVATE mbedtls/library/md2.c
        PRIVATE mbedtls/library/md4.c
        PRIVATE mbedtls/library/md5.c
        PRIVATE mbedtls/library/memory_buffer_alloc.c
        PRIVATE mbedtls/library/mps_reader.c
        PRIVATE mbedtls/library/mps_trace.c
        PRIVATE mbedtls/library/nist_kw.c
        PRIVATE mbedtls/library/oid.c
        PRIVATE mbedtls/library/padlock.c
        PRIVATE mbedtls/library/pem.c
        PRIVATE mbedtls/library/pk.c
        PRIVATE mbedtls/library/pk_wrap.c
        PRIVATE mbedtls/library/pkcs12.c
        PRIVATE mbedtls/library/pkcs5.c
        PRIVATE mbedtls/library/pkparse.c
        PRIVATE mbedtls/library/pkwrite.c
        PRIVATE mbedtls/library/platform.c
        PRIVATE mbedtls/library/platform_util.c
        PRIVATE mbedtls/library/poly1305.c
        PRIVATE mbedtls/library/psa_crypto.c
        PRIVATE mbedtls/library/psa_crypto_aead.c
        PRIVATE mbedtls/library/psa_crypto_cipher.c
        PRIVATE mbedtls/library/psa_crypto_client.c
        PRIVATE mbedtls/library/psa_crypto_driver_wrappers.c
        PRIVATE mbedtls/library/psa_crypto_ecp.c
        PRIVATE mbedtls/library/psa_crypto_hash.c
        PRIVATE mbedtls/library/psa_crypto_mac.c
        PRIVATE mbedtls/library/psa_crypto_rsa.c
        PRIVATE mbedtls/library/psa_crypto_se.c
        PRIVATE mbedtls/library/psa_crypto_slot_management.c
        PRIVATE mbedtls/library/psa_crypto_storage.c
        PRIVATE mbedtls/library/psa_its_file.c
        PRIVATE mbedtls/library/ripemd160.c
        PRIVATE mbedtls/library/rsa.c
        PRIVATE mbedtls/library/rsa_internal.c
        PRIVATE mbedtls/library/sha1.c
        PRIVATE mbedtls/library/sha256.c
        PRIVATE mbedtls/library/sha512.c
        PRIVATE mbedtls/library/threading.c
        PRIVATE mbedtls/library/timing.c
        PRIVATE mbedtls/library/version.c
        PRIVATE mbedtls/library/version_features.c
        PRIVATE mbedtls/library/xtea.c
        )

# x509
target_sources(kernel
        PRIVATE mbedtls/library/certs.c
        PRIVATE mbedtls/library/pkcs11.c
        PRIVATE mbedtls/library/x509.c
        PRIVATE mbedtls/library/x509_create.c
        PRIVATE mbedtls/library/x509_crl.c
        PRIVATE mbedtls/library/x509_crt.c
        PRIVATE mbedtls/library/x509_csr.c
        PRIVATE mbedtls/library/x509write_crt.c
        PRIVATE mbedtls/library/x509write_csr.c)

# tls
target_sources(kernel
        PRIVATE mbedtls/library/debug.c
        PRIVATE mbedtls/library/net_sockets.c
        PRIVATE mbedtls/library/ssl_cache.c
        PRIVATE mbedtls/library/ssl_ciphersuites.c
        PRIVATE mbedtls/library/ssl_cli.c
        PRIVATE mbedtls/library/ssl_cookie.c
        PRIVATE mbedtls/library/ssl_msg.c
        PRIVATE mbedtls/library/ssl_srv.c
        PRIVATE mbedtls/library/ssl_ticket.c
        PRIVATE mbedtls/library/ssl_tls.c
        PRIVATE mbedtls/library/ssl_tls13_keys.c)

target_include_directories(kernel PRIVATE mbedtls/include)
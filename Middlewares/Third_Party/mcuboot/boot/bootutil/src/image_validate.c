/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/*
 * Modifications are Copyright (c) 2019-2020 Arm Limited.
 */

#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>

#include <flash_map_backend/flash_map_backend.h>

#include "bootutil/image.h"
#include "bootutil/sha256.h"
#include "bootutil/sign_key.h"
#include "bootutil/security_cnt.h"
#include "bootutil/bootutil_log.h"
#include "mcuboot_config/mcuboot_config.h"

#ifdef MCUBOOT_ENC_IMAGES
#include "bootutil/enc_key.h"
#endif
#if defined(MCUBOOT_SIGN_RSA)
#include "mbedtls/rsa.h"
#endif
#if defined(MCUBOOT_SIGN_EC) || defined(MCUBOOT_SIGN_EC256)
#include "mbedtls/ecdsa.h"
#endif
#include "mbedtls/asn1.h"

#include "bootutil_priv.h"

/*
 * Compute SHA256 over the image.
 */
static int
bootutil_img_hash(struct enc_key_data *enc_state, int image_index,
                  struct image_header *hdr, const struct flash_area *fap,
                  uint8_t *tmp_buf, uint32_t tmp_buf_sz, uint8_t *hash_result,
                  uint8_t *seed, int seed_len)
{
    bootutil_sha256_context sha256_ctx;
    uint32_t blk_sz;
    uint32_t size;
#ifdef MCUBOOT_ENC_IMAGES
    uint16_t hdr_size;
#endif
    uint32_t off;
    int rc;
#ifdef MCUBOOT_ENC_IMAGES
    uint32_t blk_off;
    uint32_t tlv_off;
#endif

#if (BOOT_IMAGE_NUMBER == 1) || !defined(MCUBOOT_ENC_IMAGES)
    (void)enc_state;
    (void)image_index;
#endif


#if defined(MCUBOOT_ENC_IMAGES) && !defined(MCUBOOT_PRIMARY_ONLY)
    /* Encrypted images only exist in the secondary slot  */
    if (MUST_DECRYPT(fap, image_index, hdr) &&
            !boot_enc_valid(enc_state, image_index, fap)) {
        return -1;
    }
#endif

    bootutil_sha256_init(&sha256_ctx);

    /* in some cases (split image) the hash is seeded with data from
     * the loader image */
    if (seed && (seed_len > 0)) {
        bootutil_sha256_update(&sha256_ctx, seed, seed_len);
    }

    /* Hash is computed over image header and image itself. */
#ifdef MCUBOOT_ENC_IMAGES
    size = hdr_size = hdr->ih_hdr_size;
#else
    size = hdr->ih_hdr_size;
#endif
    size += hdr->ih_img_size;
#ifdef MCUBOOT_ENC_IMAGES
    tlv_off = size;
#endif

    /* If protected TLVs are present they are also hashed. */
    size += hdr->ih_protect_tlv_size;

    for (off = 0; off < size; off += blk_sz) {
        blk_sz = size - off;
        if (blk_sz > tmp_buf_sz) {
            blk_sz = tmp_buf_sz;
        }
#ifdef MCUBOOT_ENC_IMAGES
        /* The only data that is encrypted in an image is the payload;
         * both header and TLVs (when protected) are not.
         */
        if ((off < hdr_size) && ((off + blk_sz) > hdr_size)) {
            /* read only the header */
            blk_sz = hdr_size - off;
        }
        if ((off < tlv_off) && ((off + blk_sz) > tlv_off)) {
            /* read only up to the end of the image payload */
            blk_sz = tlv_off - off;
        }
#endif
        rc = flash_area_read(fap, off, tmp_buf, blk_sz);
        if (rc) {
            return rc;
        }

#ifdef MCUBOOT_ENC_IMAGES
#ifdef MCUBOOT_PRIMARY_ONLY
    if (MUST_DECRYPT_PRIMARY_ONLY(fap, image_index, hdr)) {
#else
    if (MUST_DECRYPT(fap, image_index, hdr)) {
#endif
            /* Only payload is encrypted (area between header and TLVs) */
            if (off >= hdr_size && off < tlv_off) {
                blk_off = (off - hdr_size) & 0xf;
                boot_encrypt(enc_state, image_index, fap, off - hdr_size,
                        blk_sz, blk_off, tmp_buf);
            }
#ifdef MCUBOOT_PRIMARY_ONLY
            else {
                /* header must not be flagged decrypted to compute the correct hash */
                struct image_header *header= (struct image_header *)tmp_buf;
                BOOT_LOG_INF("Controlling an encrypted primary image");
                /* double check that header content is in buffer */
                if (blk_sz == hdr_size)
                    header->ih_flags &=~IMAGE_F_ENCRYPTED;
                else {
                    BOOT_LOG_INF("Header does not fit in a block size: not supported with MCUBOOT_PRIMARY_ONLY");
                    return -1;
                }
            }
#endif

        }
#endif
        bootutil_sha256_update(&sha256_ctx, tmp_buf, blk_sz);
    }
    bootutil_sha256_finish(&sha256_ctx, hash_result);

    return 0;
}

/*
 * Currently, we only support being able to verify one type of
 * signature, because there is a single verification function that we
 * call.  List the type of TLV we are expecting.  If we aren't
 * configured for any signature, don't define this macro.
 */
#if (defined(MCUBOOT_SIGN_RSA)      + \
     defined(MCUBOOT_SIGN_EC)       + \
     defined(MCUBOOT_SIGN_EC256)    + \
     defined(MCUBOOT_SIGN_ED25519)) > 1
#error "Only a single signature type is supported!"
#endif

#if defined(MCUBOOT_SIGN_RSA)
#    if MCUBOOT_SIGN_RSA_LEN == 2048
#        define EXPECTED_SIG_TLV IMAGE_TLV_RSA2048_PSS
#    elif MCUBOOT_SIGN_RSA_LEN == 3072
#        define EXPECTED_SIG_TLV IMAGE_TLV_RSA3072_PSS
#    else
#        error "Unsupported RSA signature length"
#    endif
#    define SIG_BUF_SIZE (MCUBOOT_SIGN_RSA_LEN / 8)
#    define EXPECTED_SIG_LEN(x) ((x) == SIG_BUF_SIZE) /* 2048 bits */
#elif defined(MCUBOOT_SIGN_EC)
#    define EXPECTED_SIG_TLV IMAGE_TLV_ECDSA224
#    define SIG_BUF_SIZE 128
#    define EXPECTED_SIG_LEN(x)  (1) /* always true, ASN.1 will validate */
#elif defined(MCUBOOT_SIGN_EC256)
#    define EXPECTED_SIG_TLV IMAGE_TLV_ECDSA256
#    define SIG_BUF_SIZE 128
#    define EXPECTED_SIG_LEN(x)  (1) /* always true, ASN.1 will validate */
#elif defined(MCUBOOT_SIGN_ED25519)
#    define EXPECTED_SIG_TLV IMAGE_TLV_ED25519
#    define SIG_BUF_SIZE 64
#    define EXPECTED_SIG_LEN(x) ((x) == SIG_BUF_SIZE)
#else
#    define SIG_BUF_SIZE 32 /* no signing, sha256 digest only */
#endif

#ifdef EXPECTED_SIG_TLV
static int
bootutil_find_key(int image_index, uint8_t *keyhash, uint8_t keyhash_len)
{
    bootutil_sha256_context sha256_ctx;
    const struct bootutil_key *key;
    uint8_t hash[32];

    if (keyhash_len > 32) {
        return -1;
    }

    key = &bootutil_keys[image_index];
    bootutil_sha256_init(&sha256_ctx);
    bootutil_sha256_update(&sha256_ctx, key->key, *key->len);
    bootutil_sha256_finish(&sha256_ctx, hash);
    if (!boot_secure_memequal(hash, keyhash, keyhash_len)) {
        return (int)image_index;
     }
    return -1;
}
#endif

#ifdef MCUBOOT_HW_ROLLBACK_PROT
/**
 * Reads the value of an image's security counter.
 *
 * @param hdr           Pointer to the image header structure.
 * @param fap           Pointer to a description structure of the image's
 *                      flash area.
 * @param security_cnt  Pointer to store the security counter value.
 *
 * @return              0 on success; nonzero on failure.
 */
int32_t
bootutil_get_img_security_cnt(struct image_header *hdr,
                              const struct flash_area *fap,
                              uint32_t *img_security_cnt)
{
    struct image_tlv_iter it;
    uint32_t off;
    uint16_t len;
    int32_t rc;

    if ((hdr == NULL) ||
        (fap == NULL) ||
        (img_security_cnt == NULL)) {
        /* Invalid parameter. */
        return BOOT_EBADARGS;
    }

    /* The security counter TLV is in the protected part of the TLV area. */
    if (hdr->ih_protect_tlv_size == 0) {
        return BOOT_EBADIMAGE;
    }

    rc = bootutil_tlv_iter_begin(&it, hdr, fap, IMAGE_TLV_SEC_CNT, true);
    if (rc) {
        return rc;
    }

    /* Traverse through the protected TLV area to find
     * the security counter TLV.
     */

    rc = bootutil_tlv_iter_next(&it, &off, &len, NULL);
    if (rc != 0) {
        /* Security counter TLV has not been found. */
        return -1;
    }

    if (len != sizeof(*img_security_cnt)) {
        /* Security counter is not valid. */
        return BOOT_EBADIMAGE;
    }

    rc = flash_area_read(fap, off, img_security_cnt, len);
    if (rc != 0) {
        return BOOT_EFLASH;
    }

    return 0;
}
#endif /* MCUBOOT_HW_ROLLBACK_PROT */

/*
 * Verify the integrity of the image.
 * Return non-zero if image could not be validated/does not validate.
 */
int
bootutil_img_validate(struct enc_key_data *enc_state, int image_index,
                      struct image_header *hdr, const struct flash_area *fap,
                      uint8_t *tmp_buf, uint32_t tmp_buf_sz, uint8_t *seed,
                      int seed_len, uint8_t *out_hash)
{
    uint32_t off;
    uint16_t len;
    uint16_t type;
    int sha256_valid = 0;
#ifdef EXPECTED_SIG_TLV
    int valid_signature = 0;
    int key_id = -1;
#endif
    struct image_tlv_iter it;
    uint8_t buf[SIG_BUF_SIZE];
    uint8_t hash[32];
    int rc;
#ifdef MCUBOOT_HW_ROLLBACK_PROT
    uint32_t security_cnt = UINT32_MAX;
    uint32_t img_security_cnt = 0;
    int32_t security_counter_valid = 0;
#endif

    rc = bootutil_img_hash(enc_state, image_index, hdr, fap, tmp_buf,
            tmp_buf_sz, hash, seed, seed_len);
    if (rc) {
        return rc;
    }

    if (out_hash) {
        memcpy(out_hash, hash, 32);
    }

    rc = bootutil_tlv_iter_begin(&it, hdr, fap, IMAGE_TLV_ANY, false);
    if (rc) {
        return rc;
    }

    /*
     * Traverse through all of the TLVs, performing any checks we know
     * and are able to do.
     */
    while (true) {
        rc = bootutil_tlv_iter_next(&it, &off, &len, &type);
        if (rc < 0) {
            return -1;
        } else if (rc > 0) {
            break;
        }

        if (type == IMAGE_TLV_SHA256) {
            /*
             * Verify the SHA256 image hash.  This must always be
             * present.
             */
            if (len != sizeof(hash)) {
                return -1;
            }
            rc = flash_area_read(fap, off, buf, sizeof hash);
            if (rc) {
                return rc;
            }
            if (boot_secure_memequal(hash, buf, sizeof(hash))) {
                return -1;
            }

            sha256_valid = 1;
#ifdef EXPECTED_SIG_TLV
        } else if (type == IMAGE_TLV_KEYHASH) {
            /*
             * Determine which key we should be checking.
             */
            if (len > 32) {
                return -1;
            }
            rc = flash_area_read(fap, off, buf, len);
            if (rc) {
                return rc;
            }
            key_id = bootutil_find_key(image_index, buf, len);
            /*
             * The key may not be found, which is acceptable.  There
             * can be multiple signatures, each preceded by a key.
             */
        } else if (type == EXPECTED_SIG_TLV) {
            /* Ignore this signature if it is out of bounds. */
            if (key_id < 0 || key_id >= bootutil_key_cnt) {
                key_id = -1;
                continue;
            }
            if (!EXPECTED_SIG_LEN(len) || len > sizeof(buf)) {
                return -1;
            }
            rc = flash_area_read(fap, off, buf, len);
            if (rc) {
                return -1;
            }
			BOOT_LOG_INF("verify sig key id %d", key_id);
            rc = bootutil_verify_sig(hash, sizeof(hash), buf, len, key_id);
            if (rc == 0) {
				BOOT_LOG_INF("signature OK");
                valid_signature = 1;
            }
            key_id = -1;
#endif /* EXPECTED_SIG_TLV */
#ifdef MCUBOOT_HW_ROLLBACK_PROT
        } else if (type == IMAGE_TLV_SEC_CNT) {
            /*
             * Verify the image's security counter.
             * This must always be present.
             */
            if (len != sizeof(img_security_cnt)) {
                /* Security counter is not valid. */
                return -1;
            }

            rc = flash_area_read(fap, off, &img_security_cnt, len);
            if (rc) {
                return rc;
            }

            rc = boot_nv_security_counter_get(image_index, &security_cnt);
            if (rc) {
                return rc;
            }
            BOOT_LOG_INF("verify counter  %d %x %x", image_index,img_security_cnt, security_cnt );

            /* Compare the new image's security counter value against the
             * stored security counter value.
             */
            if (img_security_cnt < security_cnt) {
                /* The image's security counter is not accepted. */
                return -1;
            }
            BOOT_LOG_INF("counter  %d : ok", image_index );
            /* The image's security counter has been successfully verified. */
            security_counter_valid = 1;
#endif /* MCUBOOT_HW_ROLLBACK_PROT */
        }
        else
            /* unexpected TLV , injection of assembly pattern , possible */
        {
            rc = -1;
#if defined(MCUBOOT_ENCRYPT_RSA)
            if (type == IMAGE_TLV_ENC_RSA2048)
            {
                rc = 0;
            }
#elif defined(MCUBOOT_ENCRYPT_KW)
            if (type == IMAGE_TLV_ENC_KW128)
            {
                rc = 0;
            }
#elif defined(MCUBOOT_ENCRYPT_EC256)
            if (type == IMAGE_TLV_ENC_EC256)
            {
                rc = 0;
            }
#endif
            if (type == IMAGE_TLV_DEPENDENCY)
            {
                rc = 0;
            }
            if (rc)
            {

                BOOT_LOG_INF("unexpected TLV %x ", type);
                return rc;
            }
        }
    }

    if (!sha256_valid) {
        return -1;
#ifdef EXPECTED_SIG_TLV
    } else if (!valid_signature) {
        return -1;
#endif
#ifdef MCUBOOT_HW_ROLLBACK_PROT
    } else if (!security_counter_valid) {
        return -1;
#endif
    }
    /* Check pattern in slot, after image payload */
#if !defined(MCUBOOT_PRIMARY_ONLY)
    if (fap->fa_id == FLASH_AREA_IMAGE_SECONDARY(image_index))
    {
        uint64_t data;
        off = it.tlv_end;

        /* read flash per byte, until next doubleword */
        if (off % 8)
        {
            uint32_t end0 = (((off / 8) + 1) * 8);
            while (off < end0)
            {
                uint8_t data;
                rc = flash_area_read(fap, off, &data, sizeof(data));
                if (rc)
                {
                    BOOT_LOG_INF("read failed %x ", off);
                    return rc;
                }
                if (data != 0xff)
                {
                    BOOT_LOG_INF("data wrong at %x", off);
                    return -1;
                }
                off += sizeof(data);
            }
        }
        /* read flash per doubleword */
#if defined(MCUBOOT_OVERWRITE_ONLY)
        /* check pattern till magic at end of slot */
        uint32_t end = boot_magic_off(fap);
#else
        /* check pattern till trailer */
        uint32_t end = boot_status_off(fap);
#endif /* MCUBOOT_OVERWRITE_ONLY */
        while (off < end)
        {
            rc = flash_area_read(fap, off, &data, sizeof(data));
            if (rc)
            {
                BOOT_LOG_INF("read failed %x ", off);
                return rc;
            }
            if (data != 0xffffffffffffffff)
            {
                BOOT_LOG_INF("data wrong at %x", off);
                return -1;
            }
            off += sizeof(data);
        }
    }
#endif /* !defined(MCUBOOT_PRIMARY_ONLY) */
    return 0;
}

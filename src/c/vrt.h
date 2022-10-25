#ifndef VRT_H
#define VRT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// st^H^H adapted from
// https://github.com/aws/s2n-tls/blob/fe8df74ddafb712d3141056107db7228b19b6ef7/utils/s2n_blob.h#L23
typedef struct vrt_blob_t {
  uint32_t *data;
  uint32_t size;
} vrt_blob_t;

typedef enum {
  VRT_SUCCESS = 0,
  VRT_ERROR_TAG_NOT_FOUND,
  VRT_ERROR_MALFORMED,
  VRT_ERROR_WRONG_SIZE,
  VRT_ERROR_NULL_ARGUMENT,
  VRT_ERROR_TREE,
  VRT_ERROR_BOUNDS,
  VRT_ERROR_PUBK,
  VRT_ERROR_DELE,
} vrt_ret_t;

// adapted from
// https://github.com/nahojkap/craggy/blob/ce86b0f6ac90f3e71cbd50c43c4705a1de445e04/library/CraggyProtocol.h#L47

#define MAKE_TAG(val)                                                          \
  ((uint32_t)(val)[0] | (uint32_t)(val)[1] << (uint32_t)8 |                    \
   (uint32_t)(val)[2] << (uint32_t)16 | (uint32_t)(val)[3] << (uint32_t)24)

#define VRT_TAG_PAD MAKE_TAG("PAD\xff")
#define VRT_TAG_VER MAKE_TAG("VER\0")
#define VRT_TAG_SIG MAKE_TAG("SIG\0")
#define VRT_TAG_NONC MAKE_TAG("NONC")
#define VRT_TAG_MIDP MAKE_TAG("MIDP")
#define VRT_TAG_RADI MAKE_TAG("RADI")
#define VRT_TAG_ROOT MAKE_TAG("ROOT")
#define VRT_TAG_PATH MAKE_TAG("PATH")
#define VRT_TAG_SREP MAKE_TAG("SREP")
#define VRT_TAG_CERT MAKE_TAG("CERT")
#define VRT_TAG_INDX MAKE_TAG("INDX")
#define VRT_TAG_PUBK MAKE_TAG("PUBK")
#define VRT_TAG_MINT MAKE_TAG("MINT")
#define VRT_TAG_MAXT MAKE_TAG("MAXT")
#define VRT_TAG_DELE MAKE_TAG("DELE")

#ifdef TESTING_VISIBILITY
#define VISIBILITY_ONLY_TESTING
vrt_ret_t vrt_blob_init(vrt_blob_t *b, uint32_t *data, uint32_t size);
vrt_ret_t vrt_blob_r32(vrt_blob_t *b, uint32_t word_index, uint32_t *out);
vrt_ret_t vrt_blob_slice(const vrt_blob_t *b, vrt_blob_t *slice,
                         uint32_t offset, uint32_t size);
#else
#define VISIBILITY_ONLY_TESTING static
#endif

static const char OLD_CONTEXT_CERT[] = "RoughTime v1 delegation signature--\x00";
#define OLD_CONTEXT_CERT_SIZE (sizeof(OLD_CONTEXT_CERT) - 1)

static const char CONTEXT_CERT[] = "RoughTime v1 delegation signature\x00";
#define CONTEXT_CERT_SIZE (sizeof(CONTEXT_CERT) - 1)

static const char CONTEXT_RESP[] = "RoughTime v1 response signature\x00";
#define CONTEXT_RESP_SIZE (sizeof(CONTEXT_RESP) - 1)

// crypto_hash_sha512_tweet_BYTES
#define ED25519_SIG_SIZE (64)
#define CERT_SIG_SIZE (ED25519_SIG_SIZE)
#define CERT_DELE_SIZE (72)
#define VRT_NONCE_SIZE (64)
#define VRT_HASHOUT_SIZE (64)

#define VRT_NODESIZE_MAX (64)
#define VRT_NODESIZE_ALTERNATE (32)

#define MAX_SREP_SIZE (36 + VRT_NODESIZE_MAX)
#define ALTERNATE_SREP_SIZE (36 + VRT_NODESIZE_ALTERNATE)

#define VRT_DOMAIN_LABEL_LEAF (0x00)
#define VRT_DOMAIN_LABEL_NODE (0x01)

#define VRT_QUERY_LEN 1024
#define VRT_QUERY_PACKET_LEN (12+VRT_QUERY_LEN)

/** Make a roughtime query packet
 *
 * \param nonce pointer to a nonce which should be unique for each query
 * \param nonce_len length of the nonce
 * \param out_query pointer to a buffer where the query will be writtern
 * \param out_query_len pointer to the length of the query buffer
 * \param variant protocol variant (i.e. the roughtime draft number)
 *
 * The value pointed to by out_query_len will be updated with the
 * actual query length.
 *
 * Protocol variants:
 *
 * Variant 4 or earlier: The nonce size must be 64 bytes or larger
 * (only the first 64 bytes will be used).  The size of the out_query
 * buffer must be at least 1024 bytes.
 *
 * Variant 5 or later: The nonce size must be 32 bytes or larger (only
 * the first 32 bytes will be used).  The size of the out_query buffer
 * must be at least 1036 bytes.
 *
 * Variant 7 or later: Currently NOT supported by this implementation.
 */
vrt_ret_t vrt_make_query(uint8_t *nonce, uint32_t nonce_len, uint8_t *out_query,
                         uint32_t *out_query_len, unsigned variant);

/** Parse a roughtime query response
 *
 * \param nonce_sent pointer to the nonce transmitted in the query
 * \param nonce_len lenght of the nonce sent (can be larger than the actual number used in the protocol, see the vrt_make_query function for more information
 * \param reply pointer to buffer with response
 * \param reply_len length of response
 * \param pk public key for the server, must be 32 bytes long
 * \param out_midpoint pointer to where the midpoint value from the response should be written
 * \param out_radii pointer to where the radii value from the response should be written
 * \param variant protocol variant (i.e. the roughtime draft number)
 */
vrt_ret_t vrt_parse_response(uint8_t *nonce_sent, uint32_t nonce_len,
                             uint32_t *reply, uint32_t reply_len, uint8_t *pk,
                             uint64_t *out_midpoint, uint32_t *out_radii, unsigned variant);

#ifdef __cplusplus
}
#endif

#endif /* VRT_H */

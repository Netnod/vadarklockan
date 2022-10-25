// vroughtime: compact rough time client implementation
//
// https://github.com/oreparaz/vroughtime
//
// (c) 2021 Oscar Reparaz <firstname.lastname@esat.kuleuven.be>

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "tweetnacl.h"
#include "vrt.h"

struct vrt_builder
{
    int variant;
    unsigned max_size;
    unsigned num_tags;
    unsigned current_tag;
    uint8_t *buffer;
    uint32_t *size_ptr;
    uint32_t *tag_ptr;
    uint8_t *data_ptr;
    unsigned offset;
};

static uint8_t VRT_MAGIC[8] = { 'R', 'O', 'U', 'G','H', 'T', 'I', 'M' };

/* TODO these have to be modified on a big endian system */
static uint32_t vrt_host_to_le32(uint32_t v) { return v; };
static uint32_t vrt_le32_to_host(uint32_t v) { return v; };

static void vrt_put_uint32(void *buf, uint32_t v)
{
    *(uint32_t *)buf = vrt_host_to_le32(v);
}

static uint32_t vrt_get_uint32(void *buf)
{
    return vrt_le32_to_host(*(uint32_t *)buf);
}

static void vrt_query_init(struct vrt_builder *builder, uint8_t *buffer,
                           unsigned num_tags, int variant)
{
    assert(num_tags >= 1);

    builder->buffer = buffer;
    builder->num_tags = num_tags;
    builder->current_tag = 0;
    builder->variant = variant;

    if (variant >= 5) {
        /* TODO have to check if this is the correct size */
        memcpy(buffer, VRT_MAGIC, sizeof(VRT_MAGIC));
        buffer += sizeof(VRT_MAGIC);
        vrt_put_uint32(buffer, VRT_QUERY_LEN);
        buffer += sizeof(uint32_t);
    }

    vrt_put_uint32(buffer, num_tags);
    buffer += sizeof(uint32_t);

    builder->size_ptr = (uint32_t *)buffer;
    buffer += sizeof(uint32_t) * (builder->num_tags-1);

    builder->tag_ptr = (uint32_t *)buffer;
    buffer += sizeof(uint32_t) * (builder->num_tags);

    builder->data_ptr = buffer;
    builder->offset = 0;
}

static void vrt_query_add_tag(struct vrt_builder *builder, uint32_t tag,
                              const void *data, unsigned size)
{
    assert(builder->current_tag <= builder->num_tags);
    assert((size & 3) == 0);
    assert(builder->data_ptr + size <= builder->buffer + builder->max_size);

    //fprintf(stderr, "%s: %u, 0x%04x\n", __func__, builder->current_tag, tag);

    if (builder->current_tag) {
        assert(vrt_get_uint32(&builder->tag_ptr[builder->current_tag - 1]) < tag);
        vrt_put_uint32(&builder->size_ptr[builder->current_tag - 1], builder->offset);
    }

    vrt_put_uint32(&builder->tag_ptr[builder->current_tag], tag);

    memcpy(builder->data_ptr + builder->offset, data, size);

    builder->current_tag++;
    builder->offset += size;
}

static void vrt_query_finish(struct vrt_builder *builder)
{
    uint8_t *ptr = builder->data_ptr + builder->offset;
    assert(builder->current_tag == builder->num_tags);
    memset(ptr, 0, builder->buffer + builder->max_size - ptr);
}


vrt_ret_t vrt_make_query(uint8_t *nonce, uint32_t nonce_len,
                         uint8_t *out_query, uint32_t *out_query_len,
                         unsigned variant)
{
    struct vrt_builder builder;

    memset(out_query, 0, *out_query_len);

    if (variant >= 5) {
        uint32_t ver = vrt_host_to_le32(0x08000003);
        if (*out_query_len < VRT_QUERY_PACKET_LEN)
            return VRT_ERROR_WRONG_SIZE;
        builder.max_size = 12 + VRT_QUERY_PACKET_LEN;
        vrt_query_init(&builder, out_query, 3, variant);
        vrt_query_add_tag(&builder, VRT_TAG_VER, &ver, sizeof(ver));
        if (nonce_len < 32)
            return VRT_ERROR_WRONG_SIZE;
        vrt_query_add_tag(&builder, VRT_TAG_NONC, nonce, 32);
        vrt_query_add_tag(&builder, VRT_TAG_PAD, NULL, 0);
        *out_query_len = VRT_QUERY_PACKET_LEN;
    } else {
        if (*out_query_len < VRT_QUERY_LEN)
            return VRT_ERROR_WRONG_SIZE;
        builder.max_size = VRT_QUERY_LEN;
        vrt_query_init(&builder, out_query, 2, variant);
        if (nonce_len < 64)
            return VRT_ERROR_WRONG_SIZE;
        vrt_query_add_tag(&builder, VRT_TAG_NONC, nonce, 64);
        vrt_query_add_tag(&builder, VRT_TAG_PAD, NULL, 0);
        *out_query_len = VRT_QUERY_LEN;
    }
    vrt_query_finish(&builder);

    return VRT_SUCCESS;
}

#define CHECK(x)                                                        \
    do {                                                                \
        int ret;                                                        \
        if ((ret = x) != VRT_SUCCESS) {                                 \
            fprintf(stderr, "%s:%u: ret %u\n", __func__, __LINE__, ret); \
            return (ret);                                               \
        }                                                               \
    } while (0)

#define CHECK_TRUE(x, errorcode)                                        \
    do {                                                                \
        if (!(x)) {                                                     \
            fprintf(stderr, "%s:%u: ret %u\n", __func__, __LINE__, errorcode); \
            return (errorcode);                                         \
        }                                                               \
    } while (0)

#define CHECK_NOT_NULL(x) CHECK_TRUE((x) != NULL, VRT_ERROR_NULL_ARGUMENT)

VISIBILITY_ONLY_TESTING vrt_ret_t vrt_blob_init(vrt_blob_t *b, uint32_t *data,
                                                uint32_t size) {
    CHECK_NOT_NULL(b);
    CHECK_NOT_NULL(data);

    // passed size must be multiple of 4-bytes
    if (size & 3) {
        return VRT_ERROR_MALFORMED;
    }
    *b = (vrt_blob_t){.data = data, .size = size};
    return VRT_SUCCESS;
}

VISIBILITY_ONLY_TESTING vrt_ret_t vrt_blob_r32(vrt_blob_t *b,
                                               uint32_t word_index,
                                               uint32_t *out) {
    CHECK_NOT_NULL(b);
    CHECK_NOT_NULL(b->data);
    CHECK_NOT_NULL(out);

    // mind integer overflow if this condition was written as 4*word_index >=
    // b->size instead
    if (word_index >= b->size / 4) {
        return VRT_ERROR_MALFORMED;
    }

    *out = b->data[word_index];
    return VRT_SUCCESS;
}

static vrt_ret_t vrt_blob_r64(vrt_blob_t *b, uint32_t word_index,
                              uint64_t *out) {
    CHECK_NOT_NULL(b);
    CHECK_NOT_NULL(out);

    uint32_t lo = 0;
    uint32_t hi = 0;
    CHECK(vrt_blob_r32(b, word_index, &lo));
    CHECK(vrt_blob_r32(b, word_index + 1, &hi));
    *out = (uint64_t)lo + ((uint64_t)hi << 32);
    return VRT_SUCCESS;
}

VISIBILITY_ONLY_TESTING vrt_ret_t vrt_blob_slice(const vrt_blob_t *b,
                                                 vrt_blob_t *slice,
                                                 uint32_t offset,
                                                 uint32_t size) {
    CHECK_NOT_NULL(b);
    CHECK_NOT_NULL(slice);

    uint32_t slice_end = 4 * offset + size;
    uint64_t slice_end64 =
        4 * (uint64_t)offset + (uint64_t)size; // can't overflow

    if (slice_end64 != slice_end) {
        return VRT_ERROR_MALFORMED;
    }

    if (slice_end > b->size) {
        return VRT_ERROR_MALFORMED;
    }

    slice->data = &b->data[offset];
    slice->size = size;
    return VRT_SUCCESS;
}

static vrt_ret_t vrt_get_tag(vrt_blob_t *out, vrt_blob_t *in,
                             uint32_t tag_wanted) {
    uint32_t num_tags = 0;
    uint32_t tag_read = 0;
    uint32_t offset = 0;
    uint32_t tag_end = 0;

    // arithmetic in vrt_get_tag can overflow

    CHECK(vrt_blob_r32(in, 0, &num_tags));
    for (int i = 0; i < num_tags; i++) {
        CHECK(vrt_blob_r32(in, i + num_tags, &tag_read));
        if (tag_wanted == tag_read) {
            CHECK(vrt_blob_r32(in, i, &offset));
            if (i == 0) {
                offset = 0;
            }
            if (i == (num_tags - 1)) {
                tag_end = in->size - 8 * num_tags;
            } else {
                CHECK(vrt_blob_r32(in, i + 1, &tag_end));
            }

            CHECK(vrt_blob_slice(in, out, (2 * num_tags) + offset / 4,
                                 tag_end - offset));
            return VRT_SUCCESS;
        }
    }
    return VRT_ERROR_TAG_NOT_FOUND;
}

static vrt_ret_t vrt_verify_dele(vrt_blob_t *cert_sig, vrt_blob_t *cert_dele,
                                 uint8_t *root_public_key, unsigned variant) {
    /* OLD_CONTEXT_CERT_SIZE is larger han CONTEXT_CERT_SIZE */
    uint8_t msg[CERT_SIG_SIZE + CERT_DELE_SIZE + OLD_CONTEXT_CERT_SIZE] = {0};
    size_t msg_size = 0;

    CHECK_TRUE(cert_sig->size == CERT_SIG_SIZE, VRT_ERROR_WRONG_SIZE);
    CHECK_TRUE(cert_dele->size == CERT_DELE_SIZE, VRT_ERROR_WRONG_SIZE);

    memcpy(&msg + msg_size, cert_sig->data, cert_sig->size);
    msg_size += cert_sig->size;
    if (variant >= 7) {
        memcpy(msg + msg_size, CONTEXT_CERT, CONTEXT_CERT_SIZE);
        msg_size += CONTEXT_CERT_SIZE;
    } else {
        memcpy(msg + msg_size, OLD_CONTEXT_CERT, OLD_CONTEXT_CERT_SIZE);
        msg_size += OLD_CONTEXT_CERT_SIZE;
    }
    memcpy(msg + msg_size, cert_dele->data, cert_dele->size);
    msg_size += cert_dele->size;

    uint8_t plaintext[sizeof msg] = {0};
    unsigned long long unsigned_message_len;

    int ret = crypto_sign_open(plaintext, &unsigned_message_len, msg, msg_size,
                               root_public_key);
    return (ret == 0) ? VRT_SUCCESS : VRT_ERROR_DELE;
}

static vrt_ret_t vrt_verify_pubk(vrt_blob_t *sig, vrt_blob_t *srep,
                                 uint32_t *pubk, unsigned variant) {
    uint8_t msg[CERT_SIG_SIZE + CONTEXT_RESP_SIZE + MAX_SREP_SIZE] = {0};
    size_t msg_size = 0;

    CHECK_TRUE(sig->size == CERT_SIG_SIZE, VRT_ERROR_WRONG_SIZE);
    CHECK_TRUE(srep->size <= MAX_SREP_SIZE, VRT_ERROR_WRONG_SIZE);

    memcpy(&msg + msg_size, sig->data, sig->size);
    msg_size += sig->size;
    memcpy(msg + msg_size, CONTEXT_RESP, CONTEXT_RESP_SIZE);
    msg_size += CONTEXT_RESP_SIZE;
    memcpy(msg + msg_size, srep->data, srep->size);
    msg_size += srep->size;

    uint8_t plaintext[sizeof msg] = {0};
    unsigned long long unsigned_message_len;

    int ret = crypto_sign_open(plaintext, &unsigned_message_len, msg, msg_size,
                               (uint8_t *)pubk);
    return (ret == 0) ? VRT_SUCCESS : VRT_ERROR_PUBK;
}

static vrt_ret_t vrt_hash_leaf(uint8_t *out, const uint8_t *in, int noncesize,
                               unsigned variant) {
    uint8_t msg[VRT_NODESIZE_MAX + 1 /* domain separation label */];
    msg[0] = VRT_DOMAIN_LABEL_LEAF;
    memcpy(msg + 1, in, noncesize);
    if (variant >= 7)
        crypto_hash_sha512256(out, msg, noncesize + 1);
    else
        crypto_hash_sha512(out, msg, noncesize + 1);
    return VRT_SUCCESS;
}

static vrt_ret_t vrt_hash_node(uint8_t *out, const uint8_t *left,
                               const uint8_t *right, int nodesize,
                               unsigned variant) {
    uint8_t msg[2 * VRT_NODESIZE_MAX + 1 /* domain separation label */];
    msg[0] = VRT_DOMAIN_LABEL_NODE;
    memcpy(msg + 1, left, nodesize);
    memcpy(msg + 1 + nodesize, right, nodesize);
    if (variant >= 7)
        crypto_hash_sha512256(out, msg, 2 * nodesize + 1);
    else
        crypto_hash_sha512(out, msg, 2 * nodesize + 1);
    return VRT_SUCCESS;
}

static vrt_ret_t vrt_verify_nonce(vrt_blob_t *srep, vrt_blob_t *indx,
                                  vrt_blob_t *path, uint8_t *sent_nonce,
                                  unsigned variant) {
    vrt_blob_t root;
    CHECK(vrt_get_tag(&root, srep, VRT_TAG_ROOT));

    uint8_t hash[VRT_HASHOUT_SIZE] = {0};

    CHECK_TRUE(srep->size <= MAX_SREP_SIZE, VRT_ERROR_WRONG_SIZE);

    // IETF version has node size 32 bytes,
    // original version has 64-byte nodes.
    const int nodesize = variant >= 5 ? VRT_NODESIZE_ALTERNATE : VRT_NODESIZE_MAX;

    CHECK(vrt_hash_leaf(hash, sent_nonce, nodesize, variant));

    uint32_t index = 0;
    uint32_t offset = 0;
    vrt_blob_t path_chunk = {0};

    CHECK(vrt_blob_r32(indx, 0, &index));

    for (int i = 0; i < 32; i++) {
        // we're abusing a bit here vrt_blob_slice:
        // we're relying on oob access returning != VRT_SUCCESS to detect
        // there's nothing left in path
        if (vrt_blob_slice(path, &path_chunk, offset, nodesize) != VRT_SUCCESS) {
            break;
        }

        if (index & (1UL << i)) {
            CHECK(vrt_hash_node(hash, (uint8_t *)path_chunk.data, hash, nodesize, variant));
        } else {
            CHECK(vrt_hash_node(hash, hash, (uint8_t *)path_chunk.data, nodesize, variant));
        }
        offset += nodesize / 4;
    }

    CHECK_TRUE(root.size == nodesize, VRT_ERROR_WRONG_SIZE);
    return (memcmp(root.data, hash, nodesize) == 0) ? VRT_SUCCESS
        : VRT_ERROR_TREE;
}

static vrt_ret_t vrt_verify_bounds(vrt_blob_t *srep, vrt_blob_t *dele,
                                   uint64_t *out_midp, uint32_t *out_radi) {
    vrt_blob_t midp = {0};
    vrt_blob_t radi = {0};
    vrt_blob_t mint = {0};
    vrt_blob_t maxt = {0};

    uint64_t min = 0;
    uint64_t max = 0;

    CHECK(vrt_get_tag(&midp, srep, VRT_TAG_MIDP));
    CHECK(vrt_get_tag(&radi, srep, VRT_TAG_RADI));
    CHECK(vrt_get_tag(&mint, dele, VRT_TAG_MINT));
    CHECK(vrt_get_tag(&maxt, dele, VRT_TAG_MAXT));

    CHECK(vrt_blob_r64(&midp, 0, out_midp));
    CHECK(vrt_blob_r32(&radi, 0, out_radi));
    CHECK(vrt_blob_r64(&mint, 0, &min));
    CHECK(vrt_blob_r64(&maxt, 0, &max));

    if (min < *out_midp && max > *out_midp) {
        return VRT_SUCCESS;
    }

    *out_midp = 0;
    *out_radi = 0;
    return VRT_ERROR_BOUNDS;
}

vrt_ret_t vrt_parse_response(uint8_t *nonce_sent, uint32_t nonce_len,
                             uint32_t *reply, uint32_t reply_len, uint8_t *pk,
                             uint64_t *out_midpoint, uint32_t *out_radii, unsigned variant) {
    vrt_blob_t parent;
    vrt_blob_t cert = {0};
    vrt_blob_t cert_sig = {0};
    vrt_blob_t cert_dele = {0};
    vrt_blob_t srep = {0};
    vrt_blob_t pubk = {0};
    vrt_blob_t sig = {0};
    vrt_blob_t indx = {0};
    vrt_blob_t path = {0};

    if (variant >= 5) {
        if (reply_len < 12) {
            fprintf(stderr, "too short reply\n");
            return VRT_ERROR_WRONG_SIZE;
        }
        if (memcmp("ROUGHTIM", reply, 8)) {
            fprintf(stderr, "bad ROUGHTIM magic\n");
            return VRT_ERROR_MALFORMED;
        }
        if (reply_len - 12 < *(reply+2)) {
            fprintf(stderr, "bad length, expected %u, got %u\n",
                    reply_len - 12,
                    (int)*(uint32_t *)(reply+8));
            return VRT_ERROR_MALFORMED;
        }
        reply += 3;
        reply_len -= 12;
    }

    CHECK_TRUE(nonce_len >= VRT_NONCE_SIZE, VRT_ERROR_WRONG_SIZE);
    CHECK(vrt_blob_init(&parent, reply, reply_len));
    CHECK(vrt_get_tag(&srep, &parent, VRT_TAG_SREP));
    CHECK(vrt_get_tag(&sig, &parent, VRT_TAG_SIG));
    CHECK(vrt_get_tag(&cert, &parent, VRT_TAG_CERT));
    CHECK(vrt_get_tag(&cert_sig, &cert, VRT_TAG_SIG));
    CHECK(vrt_get_tag(&cert_dele, &cert, VRT_TAG_DELE));
    CHECK(vrt_get_tag(&pubk, &cert_dele, VRT_TAG_PUBK));
    CHECK(vrt_get_tag(&indx, &parent, VRT_TAG_INDX));
    CHECK(vrt_get_tag(&path, &parent, VRT_TAG_PATH));

    CHECK_TRUE(pubk.size == 32, VRT_ERROR_MALFORMED);

    /* TODO verify that nonce in response matches nonce_sent before
     * trying to hash and check signature */

    CHECK(vrt_verify_dele(&cert_sig, &cert_dele, pk, variant));
    CHECK(vrt_verify_nonce(&srep, &indx, &path, nonce_sent, variant));
    CHECK(vrt_verify_bounds(&srep, &cert_dele, out_midpoint, out_radii));
    CHECK(vrt_verify_pubk(&sig, &srep, pubk.data, variant));

    if (variant >= 5) {
        /* convert new MJD format to microseconds since time_t */
        /* TODO adjust this code so that it handles leap seconds properly */
        *out_midpoint =
            ((*out_midpoint >> 40) - 40587) * 86400000000 +
            (*out_midpoint & 0xffffffffff);
    }

    return VRT_SUCCESS;
}

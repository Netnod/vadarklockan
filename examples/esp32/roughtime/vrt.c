
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

void hd(const void *buffer, unsigned size)
{
    const uint8_t *ptr = buffer;
    unsigned i;

    for (i = 0; i < size; i++) {
	if (i)
	    printf(" ");
	printf("%02x", ptr[i]);
    }
    printf("\n");
}

#define RT_MAX_PAYLOAD_SIZE 1024
#define RT_MAX_PACKET_SIZE (12 + RT_MAX_PAYLOAD_SIZE)

enum RT_VARIANT {
  RT_PRE_IETF,
  RT_IETF_DRAFT_05,
};

struct rt_builder
{
    enum RT_VARIANT variant;
    unsigned num_tags;
    unsigned current_tag;
    uint8_t *buffer;
    uint32_t *size_ptr;
    uint32_t *tag_ptr;
    uint8_t *data_ptr;
    unsigned offset;
};

static uint8_t RT_MAGIC[8] = { 'R', 'O', 'U', 'G','H', 'T', 'I', 'M' };

#define RT_MAKE_TAG(a, b, c, d) 	\
    (((uint32_t)((a) & 255)) | 		\
     (((uint32_t)((b) & 255)) << 8) | 	\
     (((uint32_t)((c) & 255)) << 16) | 	\
     (((uint32_t)((d) & 255)) << 24))

#define RT_TAG_VER  RT_MAKE_TAG('V', 'E', 'R', 0)
#define RT_TAG_NONC RT_MAKE_TAG('N', 'O', 'N', 'C')
#define RT_TAG_PAD  RT_MAKE_TAG('P', 'A', 'D', 0xff)

static uint32_t rt_host_to_le32(uint32_t v) { return v; };
// static uint32_t rt_le32_to_host(uint32_t v) { return v; };

static void rt_put_uint32(void *buf, uint32_t v)
{
    *(uint32_t *)buf = rt_host_to_le32(v);
}

static uint32_t rt_get_uint32(void *buf)
{
    return *(uint32_t *)buf;
}

void rt_query_init(struct rt_builder *builder, uint8_t *buffer,
                   unsigned num_tags, enum RT_VARIANT variant)
{
    assert(num_tags >= 1);

    builder->buffer = buffer;
    builder->num_tags = num_tags;
    builder->current_tag = 0;
    builder->variant = variant;

    if (variant >= RT_IETF_DRAFT_05) {
        memcpy(buffer, RT_MAGIC, sizeof(RT_MAGIC));
        buffer += sizeof(RT_MAGIC);
        rt_put_uint32(buffer, RT_MAX_PAYLOAD_SIZE);
        buffer += sizeof(uint32_t);
    }

    rt_put_uint32(buffer, num_tags);
    buffer += sizeof(uint32_t);

    builder->size_ptr = (uint32_t *)buffer;
    buffer += sizeof(uint32_t) * (builder->num_tags-1);

    builder->tag_ptr = (uint32_t *)buffer;
    buffer += sizeof(uint32_t) * (builder->num_tags);

    builder->data_ptr = buffer;
    builder->offset = 0;
}

void rt_query_add_tag(struct rt_builder *builder, uint32_t tag,
		      const void *data, unsigned size)
{
    assert(builder->current_tag <= builder->num_tags);
    assert((size & 3) == 0);
    assert(builder->data_ptr + size <= builder->buffer + RT_MAX_PACKET_SIZE);

    //fprintf(stderr, "%s: %u, 0x%04x\n", __func__, builder->current_tag, tag);

    if (builder->current_tag) {
        assert(rt_get_uint32(&builder->tag_ptr[builder->current_tag - 1]) < tag);
	rt_put_uint32(&builder->size_ptr[builder->current_tag - 1], builder->offset);
    }

    rt_put_uint32(&builder->tag_ptr[builder->current_tag], tag);

    memcpy(builder->data_ptr + builder->offset, data, size);

    builder->current_tag++;
    builder->offset += size;
}

void rt_query_finish(struct rt_builder *builder)
{
    uint8_t *ptr = builder->data_ptr + builder->offset;
    assert(builder->current_tag == builder->num_tags);
    memset(ptr, 0, builder->buffer + RT_MAX_PACKET_SIZE - ptr);
}

void rt_query_make(struct rt_builder *builder, uint8_t *buffer,
                   void *nonc, enum RT_VARIANT variant)
{
    if (variant >= RT_IETF_DRAFT_05) {
        uint32_t ver = rt_host_to_le32(0x08000003);
        rt_query_init(builder, buffer, 3, variant);
        rt_query_add_tag(builder, RT_TAG_VER, &ver, sizeof(ver));
        rt_query_add_tag(builder, RT_TAG_NONC, nonc, 32);
        rt_query_add_tag(builder, RT_TAG_PAD, NULL, 0);
    } else {
        rt_query_init(builder, buffer, 2, variant);
        rt_query_add_tag(builder, RT_TAG_NONC, nonc, 64);
        rt_query_add_tag(builder, RT_TAG_PAD, NULL, 0);
    }
    rt_query_finish(builder);
}

#define CHECK(x)                                                               \
  do {                                                                         \
    int ret;                                                                   \
    if ((ret = x) != VRT_SUCCESS) {                                            \
    fprintf(stderr, "%s:%u: ret %u\n", __func__, __LINE__, ret); \
      return (ret);                                                            \
    }                                                                          \
  } while (0)

#define CHECK_TRUE(x, errorcode)                                               \
  do {                                                                         \
    if (!(x)) {								\
    fprintf(stderr, "%s:%u: ret %u\n", __func__, __LINE__, errorcode); \
      return (errorcode);                                                      \
    } \
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
                                 uint8_t *root_public_key) {
  uint8_t msg[CERT_SIG_SIZE + CERT_DELE_SIZE + CONTEXT_CERT_SIZE] = {0};

  CHECK_TRUE(cert_sig->size == CERT_SIG_SIZE, VRT_ERROR_WRONG_SIZE);
  CHECK_TRUE(cert_dele->size == CERT_DELE_SIZE, VRT_ERROR_WRONG_SIZE);

  memcpy(&msg, cert_sig->data, cert_sig->size);
  memcpy(msg + cert_sig->size, CONTEXT_CERT, CONTEXT_CERT_SIZE);
  memcpy(msg + cert_sig->size + CONTEXT_CERT_SIZE, cert_dele->data,
         cert_dele->size);

  size_t msg_size = cert_sig->size + cert_dele->size + CONTEXT_CERT_SIZE;
  uint8_t plaintext[sizeof msg] = {0};
  unsigned long long unsigned_message_len;

  int ret = crypto_sign_open(plaintext, &unsigned_message_len, msg, msg_size,
                             root_public_key);
  return (ret == 0) ? VRT_SUCCESS : VRT_ERROR_DELE;
}

static vrt_ret_t vrt_verify_pubk(vrt_blob_t *sig, vrt_blob_t *srep,
                                 uint32_t *pubk) {
  uint8_t msg[CERT_SIG_SIZE + CONTEXT_RESP_SIZE + MAX_SREP_SIZE] = {0};

  CHECK_TRUE(sig->size == CERT_SIG_SIZE, VRT_ERROR_WRONG_SIZE);
  CHECK_TRUE(srep->size <= MAX_SREP_SIZE, VRT_ERROR_WRONG_SIZE);

  memcpy(&msg, sig->data, sig->size);
  memcpy(msg + sig->size, CONTEXT_RESP, CONTEXT_RESP_SIZE);
  memcpy(msg + sig->size + CONTEXT_RESP_SIZE, srep->data, srep->size);
  size_t msg_size = sig->size + srep->size + CONTEXT_RESP_SIZE;

  uint8_t plaintext[sizeof msg] = {0};
  unsigned long long unsigned_message_len;

  int ret = crypto_sign_open(plaintext, &unsigned_message_len, msg, msg_size,
                             (uint8_t *)pubk);
  return (ret == 0) ? VRT_SUCCESS : VRT_ERROR_PUBK;
}

static vrt_ret_t vrt_hash_leaf(uint8_t *out, const uint8_t *in, int noncesize) {
  uint8_t msg[VRT_NODESIZE_MAX + 1 /* domain separation label */];
  msg[0] = VRT_DOMAIN_LABEL_LEAF;
  memcpy(msg + 1, in, noncesize);
  crypto_hash_sha512(out, msg, noncesize + 1);
  return VRT_SUCCESS;
}

static vrt_ret_t vrt_hash_node(uint8_t *out, const uint8_t *left,
                               const uint8_t *right, int nodesize) {
  uint8_t msg[2 * VRT_NODESIZE_MAX + 1 /* domain separation label */];
  msg[0] = VRT_DOMAIN_LABEL_NODE;
  memcpy(msg + 1, left, nodesize);
  memcpy(msg + 1 + nodesize, right, nodesize);
  crypto_hash_sha512(out, msg, 2 * nodesize + 1);
  return VRT_SUCCESS;
}

static vrt_ret_t vrt_verify_nonce(vrt_blob_t *srep, vrt_blob_t *indx,
                                  vrt_blob_t *path, uint8_t *sent_nonce, enum RT_VARIANT variant) {
  vrt_blob_t root;
  CHECK(vrt_get_tag(&root, srep, VRT_TAG_ROOT));

  uint8_t hash[VRT_HASHOUT_SIZE] = {0};

  CHECK_TRUE(srep->size <= MAX_SREP_SIZE, VRT_ERROR_WRONG_SIZE);

  // IETF version has node size 32 bytes,
  // original version has 64-byte nodes.
  const int nodesize = variant >= RT_IETF_DRAFT_05 ? VRT_NODESIZE_ALTERNATE : VRT_NODESIZE_MAX;

  CHECK(vrt_hash_leaf(hash, sent_nonce, nodesize));

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
      CHECK(vrt_hash_node(hash, (uint8_t *)path_chunk.data, hash, nodesize));
    } else {
      CHECK(vrt_hash_node(hash, hash, (uint8_t *)path_chunk.data, nodesize));
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
                             uint64_t *out_midpoint, uint32_t *out_radii, int variant) {
  vrt_blob_t parent;
  vrt_blob_t cert = {0};
  vrt_blob_t cert_sig = {0};
  vrt_blob_t cert_dele = {0};
  vrt_blob_t srep = {0};
  vrt_blob_t pubk = {0};
  vrt_blob_t sig = {0};
  vrt_blob_t indx = {0};
  vrt_blob_t path = {0};

  if (variant >= RT_IETF_DRAFT_05) {
    if (reply_len < 12) {
      fprintf(stderr, "too short reply\n");
      return VRT_ERROR_WRONG_SIZE;
    }
    if (memcmp("ROUGHTIM", reply, 8)) {
      fprintf(stderr, "bad ROUGHTIM magic\n");
      return VRT_ERROR_MALFORMED;
    }
    if (reply_len - 12 != *(reply+2)) {
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

  CHECK(vrt_verify_dele(&cert_sig, &cert_dele, pk));
  CHECK_TRUE(pubk.size == 32, VRT_ERROR_MALFORMED);
  CHECK(vrt_verify_pubk(&sig, &srep, pubk.data));
  CHECK(vrt_verify_nonce(&srep, &indx, &path, nonce_sent, variant));
  CHECK(vrt_verify_bounds(&srep, &cert_dele, out_midpoint, out_radii));

  return VRT_SUCCESS;
}

vrt_ret_t vrt_make_query(uint8_t *nonce, uint32_t nonce_len, uint8_t *out_query,
                         uint32_t out_query_len, int variant) {
  struct rt_builder builder;

  rt_query_make(&builder, out_query, nonce, variant);

  return VRT_SUCCESS;
}

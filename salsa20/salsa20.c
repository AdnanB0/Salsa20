#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static uint32_t rotl32(uint32_t x, int n) {
   return (x << n) | (x >> (32 - n));
}

static void quarterround(uint32_t *z0, uint32_t *z1, uint32_t *z2, uint32_t *z3, uint32_t y0, uint32_t y1, uint32_t y2, uint32_t y3) {
   uint32_t r1 = y1 ^ rotl32(y0 + y3, 7);
   uint32_t r2 = y2 ^ rotl32(r1 + y0, 9);
   uint32_t r3 = y3 ^ rotl32(r2 + r1, 13);
   uint32_t r0 = y0 ^ rotl32(r3 + r2, 18);
   *z0 = r0; *z1 = r1; *z2 = r2; *z3 = r3;
}

static void rowround(uint32_t y[16], uint32_t z[16]) {
   quarterround(&z[0], &z[1], &z[2], &z[3], y[0], y[1], y[2], y[3]);
   quarterround(&z[5], &z[6], &z[7], &z[4], y[5], y[6], y[7], y[4]);
   quarterround(&z[10], &z[11], &z[8], &z[9], y[10], y[11], y[8], y[9]);
   quarterround(&z[15], &z[12], &z[13], &z[14], y[15], y[12], y[13], y[14]);
}

static void columnround(uint32_t x[16], uint32_t y[16]) {
   quarterround(&y[0], &y[4], &y[8], &y[12], x[0], x[4], x[8], x[12]);
   quarterround(&y[5], &y[9], &y[13], &y[1], x[5], x[9], x[13], x[1]);
   quarterround(&y[10], &y[14], &y[2], &y[6], x[10], x[14], x[2], x[6]);
   quarterround(&y[15], &y[3], &y[7], &y[11], x[15], x[3], x[7], x[11]);
}

static void doubleround(uint32_t x[16], uint32_t z[16]) {
   uint32_t y[16];
   columnround(x, y);
   rowround(y, z);
}

static uint32_t littleendian(const uint8_t b[4]) {
   return (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
}

static void littleendian_inv(uint32_t w, uint8_t out[4]) {
   out[0] = w & 0xFF;
   out[1] = (w >> 8) & 0xFF;
   out[2] = (w >> 16) & 0xFF;
   out[3] = (w >> 24) & 0xFF;
}

static void salsa20_hash(const uint8_t in[64], uint8_t out[64], int rounds) {
   uint32_t x[16], x_orig[16], tmp[16];
   for (int i = 0; i < 16; i++) {
       x[i] = littleendian(in + 4 * i);
       x_orig[i] = x[i];
   }
   for (int r = 0; r < rounds / 2; r++) {
       doubleround(x, tmp);
       memcpy(x, tmp, sizeof(x));
   }
   for (int i = 0; i < 16; i++) {
       littleendian_inv(x[i] + x_orig[i], out + 4 * i);
   }
}

#define ROUNDS 10

static const uint8_t A[4] = {'e','x','p','a'};
static const uint8_t B[4] = {'n','d',' ','3'};
static const uint8_t C[4] = {'2','-','b','y'};
static const uint8_t D[4] = {'t','e',' ','k'};

static const uint8_t A1[4] = {'e','x','p','a'};
static const uint8_t B1[4] = {'n','d',' ','1'};
static const uint8_t C1[4] = {'6','-','b','y'};
static const uint8_t D1[4] = {'t','e',' ','k'};

static const uint8_t A2[4] = {'e','x','p','a'};
static const uint8_t B2[4] = {'n','d',' ','0'};
static const uint8_t C2[4] = {'8','-','b','y'};
static const uint8_t D2[4] = {'t','e',' ','k'};


static void expand_256(const uint8_t key32[32], const uint8_t n16[16], uint8_t out[64]) {
   uint8_t block[64];
   uint8_t *p = block;
   memcpy(p, A, 4); p += 4;
   memcpy(p, key32, 16); p += 16;
   memcpy(p, B, 4); p += 4;
   memcpy(p, n16, 16); p += 16;
   memcpy(p, C, 4); p += 4;
   memcpy(p, key32 + 16, 16); p += 16;
   memcpy(p, D, 4); p += 4;
   salsa20_hash(block, out, ROUNDS);
}


static void expand_128(const uint8_t key16[16], const uint8_t n16[16], uint8_t out[64]) {
   uint8_t block[64];
   uint8_t *p = block;
   memcpy(p, A1, 4); p += 4;
   memcpy(p, key16, 16); p += 16;
   memcpy(p, B1, 4); p += 4;
   memcpy(p, n16, 16); p += 16;
   memcpy(p, C1, 4); p += 4;
   memcpy(p, key16, 16); p += 16;
   memcpy(p, D1, 4); p += 4;
   salsa20_hash(block, out, ROUNDS);
}


static void expand_64(const uint8_t key8[8], const uint8_t n16[16], uint8_t out[64]) {
   uint8_t block[64];
   uint8_t *p = block;
   memcpy(p, A2, 4); p += 4;
   memcpy(p, key8, 8); p += 8;
   memcpy(p, key8, 8); p += 8;
   memcpy(p, B2, 4); p += 4;
   memcpy(p, n16, 16); p += 16;
   memcpy(p, C2, 4); p += 4;
   memcpy(p, key8, 8); p += 8;
   memcpy(p, key8, 8); p += 8;
   memcpy(p, D2, 4); p += 4;
   salsa20_hash(block, out, ROUNDS);
}

static void keystream_block(const uint8_t *key, int keylen_bits, const uint8_t nonce8[8], uint64_t counter, uint8_t out[64]) {
   uint8_t n16[16];
   memcpy(n16, nonce8, 8);
   for (int i = 0; i < 8; i++) {
       n16[8 + i] = (uint8_t)((counter >> (8 * i)) & 0xFF);
   }
   if (keylen_bits == 256) {
       expand_256(key, n16, out);
   } else if (keylen_bits == 128) {
       expand_128(key, n16, out);
   } else {
       expand_64(key, n16, out);
   }
}

static void salsa20_crypt(const uint8_t *key, int keylen_bits, const uint8_t nonce8[8], const uint8_t *data, size_t data_len, uint8_t *out) {
   uint64_t counter = 0;
   size_t offset = 0;
   while (offset < data_len) {
       uint8_t ks[64];
       keystream_block(key, keylen_bits, nonce8, counter, ks);
       size_t chunk_len = data_len - offset;
       if (chunk_len > 64) chunk_len = 64;
       for (size_t i = 0; i < chunk_len; i++) {
           out[offset + i] = data[offset + i] ^ ks[i];
       }
       offset += chunk_len;
       counter += 1;
   }
}

static int hexval(char c) {
   if (c >= '0' && c <= '9') return c - '0';
   if (c >= 'a' && c <= 'f') return c - 'a' + 10;
   if (c >= 'A' && c <= 'F') return c - 'A' + 10;
   return -1;
}

static int hex_decode(const char *hex, uint8_t *out, size_t *out_len, size_t max_out) {
   size_t hlen = strlen(hex);
   if (hlen % 2 != 0) return -1;
   size_t n = hlen / 2;
   if (n > max_out) return -1;
   for (size_t i = 0; i < n; i++) {
       int hi = hexval(hex[2 * i]);
       int lo = hexval(hex[2 * i + 1]);
       if (hi < 0 || lo < 0) return -1;
       out[i] = (uint8_t)((hi << 4) | lo);
   }
   *out_len = n;
   return 0;
}

static void hex_encode(const uint8_t *data, size_t len, char *out) {
   static const char *digits = "0123456789abcdef";
   for (size_t i = 0; i < len; i++) {
       out[2 * i] = digits[(data[i] >> 4) & 0xF];
       out[2 * i + 1] = digits[data[i] & 0xF];
   }
   out[2 * len] = '\0';
}


int main(int argc, char **argv) {
   if (argc != 5) {
       fprintf(stderr, "Usage: %s <keylen_bits> <key_hex> <nonce_hex> <text_hex>\n", argv[0]);
       return 1;
   }

   int keylen_bits = atoi(argv[1]);
   if (keylen_bits != 64 && keylen_bits != 128 && keylen_bits != 256) {
       fprintf(stderr, "Error: key length must be 64, 128, or 256 bits.\n");
       return 1;
   }

   uint8_t key[32];
   size_t key_len;
   if (hex_decode(argv[2], key, &key_len, sizeof(key)) != 0) {
       fprintf(stderr, "Error: key must be a valid hex string.\n");
       return 1;
   }
   size_t expected_key_bytes = (size_t)keylen_bits / 8;
   if (key_len != expected_key_bytes) {
       fprintf(stderr, "Error: key must be %zu bytes (%zu hex chars) for a %d-bit key, got %zu bytes.\n",
               expected_key_bytes, expected_key_bytes * 2, keylen_bits, key_len);
       return 1;
   }

   uint8_t nonce[8];
   size_t nonce_len;
   if (hex_decode(argv[3], nonce, &nonce_len, sizeof(nonce)) != 0 || nonce_len != 8) {
       fprintf(stderr, "Error: nonce must be 8 bytes (16 hex chars) of valid hex.\n");
       return 1;
   }

   uint8_t text[1024];
   size_t text_len;
   if (hex_decode(argv[4], text, &text_len, sizeof(text)) != 0) {
       fprintf(stderr, "Error: text must be a valid hex string of at most 1KB.\n");
       return 1;
   }
   uint8_t out[1024];
   salsa20_crypt(key, keylen_bits, nonce, text, text_len, out);
   char hexout[2049];
   hex_encode(out, text_len, hexout);
   printf("%s\n", hexout);


   return 0;
}

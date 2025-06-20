#ifndef HASH_H
#define HASH_H
/*
Usage:

#include <stdio.h>
#include <string.h>

void print_hash_32(const char* str) {
    uint32_t hash = MurmurHash3_x86_32(str, strlen(str), 0);
    printf("MurmurHash3_32(\"%s\") = 0x%08x\n", str, hash);
}

void print_hash_128_x86(const char* str) {
    uint32_t hash[4];
    MurmurHash3_x86_128(str, strlen(str), 0, hash);
    printf("MurmurHash3_x86_128(\"%s\") = 0x%08x%08x%08x%08x\n", str, hash[0], hash[1], hash[2], hash[3]);
}

void print_hash_128_x64(const char* str) {
    uint64_t hash[2];
    MurmurHash3_x64_128(str, strlen(str), 0, hash);
    printf("MurmurHash3_x64_128(\"%s\") = 0x%016lx%016lx\n", str, hash[0], hash[1]);
}

int main() {
    const char* test_strings[] = {"", "a", "hello", "hello world",
                                  "The quick brown fox jumps over the lazy dog"};

    int num_tests = sizeof(test_strings) / sizeof(test_strings[0]);

    for (int i = 0; i < num_tests; i++) {
        print_hash_32(test_strings[i]);
        print_hash_128_x86(test_strings[i]);
        print_hash_128_x64(test_strings[i]);
        printf("\n");
    }

    return 0;
}

*/

#include <endian.h>  // glibc endian support
#include <stddef.h>  // type definitions
#include <stdint.h>  // standard integer support

#ifdef __cplusplus
extern "C" {
#endif

// Rotate left macro
#define ROTL32(x, r) ((x << r) | (x >> (32 - r)))
#define ROTL64(x, r) ((x << r) | (x >> (64 - r)))

// Endian-aware block reading using glibc macros
static inline uint32_t getblock32(const void* p, int i) {
    const uint32_t* blocks = (const uint32_t*)p;
    // Convert from little-endian byte order to host byte order
    return le32toh(blocks[i]);
}

static inline uint64_t getblock64(const void* p, int i) {
    const uint64_t* blocks = (const uint64_t*)p;
    // Convert from little-endian byte order to host byte order
    return le64toh(blocks[i]);
}

// MurmurHash3 32-bit finalizer
static inline uint32_t fmix32(uint32_t h) {
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}

// MurmurHash3 64-bit finalizer
static inline uint64_t fmix64(uint64_t k) {
    k ^= k >> 33;
    k *= 0xff51afd7ed558ccdULL;
    k ^= k >> 33;
    k *= 0xc4ceb9fe1a85ec53ULL;
    k ^= k >> 33;
    return k;
}

//-----------------------------------------------------------------------------
// MurmurHash3_x86_32 - 32-bit hash for x86 platforms
//-----------------------------------------------------------------------------
static inline uint32_t MurmurHash3_x86_32(const void* key, int len, uint32_t seed) {
    const uint8_t* data = (const uint8_t*)key;
    const int nblocks   = len / 4;

    uint32_t h1 = seed;

    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;

    // Body - process 4-byte blocks
    const uint32_t* blocks = (const uint32_t*)(data + nblocks * 4);

    for (int i = -nblocks; i; i++) {
        uint32_t k1 = getblock32(blocks, i);

        k1 *= c1;
        k1 = ROTL32(k1, 15);
        k1 *= c2;

        h1 ^= k1;
        h1 = ROTL32(h1, 13);
        h1 = h1 * 5 + 0xe6546b64;
    }

    // Tail - handle remaining bytes
    const uint8_t* tail = (const uint8_t*)(data + nblocks * 4);
    uint32_t k1         = 0;

    switch (len & 3) {
        case 3:
            k1 ^= tail[2] << 16;
        case 2:
            k1 ^= tail[1] << 8;
        case 1:
            k1 ^= tail[0];
            k1 *= c1;
            k1 = ROTL32(k1, 15);
            k1 *= c2;
            h1 ^= k1;
    }

    // Finalization
    h1 ^= len;
    h1 = fmix32(h1);

    return h1;
}

//-----------------------------------------------------------------------------
// MurmurHash3_x86_128 - 128-bit hash for x86 platforms (returns 4 x 32-bit values)
//-----------------------------------------------------------------------------
static inline void MurmurHash3_x86_128(const void* key, const int len, uint32_t seed, void* out) {
    const uint8_t* data = (const uint8_t*)key;
    const int nblocks   = len / 16;

    uint32_t h1 = seed;
    uint32_t h2 = seed;
    uint32_t h3 = seed;
    uint32_t h4 = seed;

    const uint32_t c1 = 0x239b961b;
    const uint32_t c2 = 0xab0e9789;
    const uint32_t c3 = 0x38b34ae5;
    const uint32_t c4 = 0xa1e38b93;

    // Body - process 16-byte blocks
    const uint32_t* blocks = (const uint32_t*)(data + nblocks * 16);

    for (int i = -nblocks; i; i++) {
        uint32_t k1 = getblock32(blocks, i * 4 + 0);
        uint32_t k2 = getblock32(blocks, i * 4 + 1);
        uint32_t k3 = getblock32(blocks, i * 4 + 2);
        uint32_t k4 = getblock32(blocks, i * 4 + 3);

        k1 *= c1;
        k1 = ROTL32(k1, 15);
        k1 *= c2;
        h1 ^= k1;
        h1 = ROTL32(h1, 19);
        h1 += h2;
        h1 = h1 * 5 + 0x561ccd1b;

        k2 *= c2;
        k2 = ROTL32(k2, 16);
        k2 *= c3;
        h2 ^= k2;
        h2 = ROTL32(h2, 17);
        h2 += h3;
        h2 = h2 * 5 + 0x0bcaa747;

        k3 *= c3;
        k3 = ROTL32(k3, 17);
        k3 *= c4;
        h3 ^= k3;
        h3 = ROTL32(h3, 15);
        h3 += h4;
        h3 = h3 * 5 + 0x96cd1c35;

        k4 *= c4;
        k4 = ROTL32(k4, 18);
        k4 *= c1;
        h4 ^= k4;
        h4 = ROTL32(h4, 13);
        h4 += h1;
        h4 = h4 * 5 + 0x32ac3b17;
    }

    // Tail - handle remaining bytes
    const uint8_t* tail = (const uint8_t*)(data + nblocks * 16);
    uint32_t k1 = 0, k2 = 0, k3 = 0, k4 = 0;

    switch (len & 15) {
        case 15:
            k4 ^= tail[14] << 16;
        case 14:
            k4 ^= tail[13] << 8;
        case 13:
            k4 ^= tail[12] << 0;
            k4 *= c4;
            k4 = ROTL32(k4, 18);
            k4 *= c1;
            h4 ^= k4;

        case 12:
            k3 ^= tail[11] << 24;
        case 11:
            k3 ^= tail[10] << 16;
        case 10:
            k3 ^= tail[9] << 8;
        case 9:
            k3 ^= tail[8] << 0;
            k3 *= c3;
            k3 = ROTL32(k3, 17);
            k3 *= c4;
            h3 ^= k3;

        case 8:
            k2 ^= tail[7] << 24;
        case 7:
            k2 ^= tail[6] << 16;
        case 6:
            k2 ^= tail[5] << 8;
        case 5:
            k2 ^= tail[4] << 0;
            k2 *= c2;
            k2 = ROTL32(k2, 16);
            k2 *= c3;
            h2 ^= k2;

        case 4:
            k1 ^= tail[3] << 24;
        case 3:
            k1 ^= tail[2] << 16;
        case 2:
            k1 ^= tail[1] << 8;
        case 1:
            k1 ^= tail[0] << 0;
            k1 *= c1;
            k1 = ROTL32(k1, 15);
            k1 *= c2;
            h1 ^= k1;
    }

    // Finalization
    h1 ^= len;
    h2 ^= len;
    h3 ^= len;
    h4 ^= len;

    h1 += h2;
    h1 += h3;
    h1 += h4;
    h2 += h1;
    h3 += h1;
    h4 += h1;

    h1 = fmix32(h1);
    h2 = fmix32(h2);
    h3 = fmix32(h3);
    h4 = fmix32(h4);

    h1 += h2;
    h1 += h3;
    h1 += h4;
    h2 += h1;
    h3 += h1;
    h4 += h1;

    ((uint32_t*)out)[0] = h1;
    ((uint32_t*)out)[1] = h2;
    ((uint32_t*)out)[2] = h3;
    ((uint32_t*)out)[3] = h4;
}

//-----------------------------------------------------------------------------
// MurmurHash3_x64_128 - 128-bit hash for x64 platforms (returns 2 x 64-bit values)
//-----------------------------------------------------------------------------
static inline void MurmurHash3_x64_128(const void* key, const int len, const uint32_t seed, void* out) {
    const uint8_t* data = (const uint8_t*)key;
    const int nblocks   = len / 16;

    uint64_t h1 = seed;
    uint64_t h2 = seed;

    const uint64_t c1 = 0x87c37b91114253d5ULL;
    const uint64_t c2 = 0x4cf5ad432745937fULL;

    // Body - process 16-byte blocks
    const uint64_t* blocks = (const uint64_t*)(data);

    for (int i = 0; i < nblocks; i++) {
        uint64_t k1 = getblock64(blocks, i * 2 + 0);
        uint64_t k2 = getblock64(blocks, i * 2 + 1);

        k1 *= c1;
        k1 = ROTL64(k1, 31);
        k1 *= c2;
        h1 ^= k1;
        h1 = ROTL64(h1, 27);
        h1 += h2;
        h1 = h1 * 5 + 0x52dce729;

        k2 *= c2;
        k2 = ROTL64(k2, 33);
        k2 *= c1;
        h2 ^= k2;
        h2 = ROTL64(h2, 31);
        h2 += h1;
        h2 = h2 * 5 + 0x38495ab5;
    }

    // Tail - handle remaining bytes
    const uint8_t* tail = (const uint8_t*)(data + nblocks * 16);
    uint64_t k1 = 0, k2 = 0;

    switch (len & 15) {
        case 15:
            k2 ^= ((uint64_t)tail[14]) << 48;
        case 14:
            k2 ^= ((uint64_t)tail[13]) << 40;
        case 13:
            k2 ^= ((uint64_t)tail[12]) << 32;
        case 12:
            k2 ^= ((uint64_t)tail[11]) << 24;
        case 11:
            k2 ^= ((uint64_t)tail[10]) << 16;
        case 10:
            k2 ^= ((uint64_t)tail[9]) << 8;
        case 9:
            k2 ^= ((uint64_t)tail[8]) << 0;
            k2 *= c2;
            k2 = ROTL64(k2, 33);
            k2 *= c1;
            h2 ^= k2;

        case 8:
            k1 ^= ((uint64_t)tail[7]) << 56;
        case 7:
            k1 ^= ((uint64_t)tail[6]) << 48;
        case 6:
            k1 ^= ((uint64_t)tail[5]) << 40;
        case 5:
            k1 ^= ((uint64_t)tail[4]) << 32;
        case 4:
            k1 ^= ((uint64_t)tail[3]) << 24;
        case 3:
            k1 ^= ((uint64_t)tail[2]) << 16;
        case 2:
            k1 ^= ((uint64_t)tail[1]) << 8;
        case 1:
            k1 ^= ((uint64_t)tail[0]) << 0;
            k1 *= c1;
            k1 = ROTL64(k1, 31);
            k1 *= c2;
            h1 ^= k1;
    }

    // Finalization
    h1 ^= len;
    h2 ^= len;

    h1 += h2;
    h2 += h1;

    h1 = fmix64(h1);
    h2 = fmix64(h2);

    h1 += h2;
    h2 += h1;

    ((uint64_t*)out)[0] = h1;
    ((uint64_t*)out)[1] = h2;
}

// Use 32-bit MurmurHash3 finalizer for regular int, uint32_t
static inline uint32_t hash_int32(uint32_t x) {
    x ^= x >> 16;
    x *= 0x85ebca6b;
    x ^= x >> 13;
    x *= 0xc2b2ae35;
    x ^= x >> 16;
    return x;
}

// Use the 64-bit MurmurHash3 finalizer for size_t, pointers, and 64-bit integers
static inline uint64_t hash_int64(uint64_t x) {
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
    return x;
}

// Hash size_t on 64 bit platforms.
static inline size_t hash_size_t(size_t x) {
    return (size_t)hash_int64(x);
}

// FNV-1a hash function for C strings
static inline uint64_t fnv1a_hash(const char* s) {
    uint32_t h = 0x811C9DC5;
    while (*s) {
        h = (h ^ (uint8_t)(*s++)) * 0x01000193;
    }
    return h;
}

#ifdef __cplusplus
}
#endif

#endif /* HASH_H */

#ifndef HASHMAP_H
#define HASHMAP_H

/*
MIT License

Copyright (c) 2025 Dr. Nathan Abiira

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Macro to get containing structure from member pointer.
// This is expected to be used with HNode pointers.
#define container_of(ptr, type, member) ((type*)((char*)(ptr) - offsetof(type, member)))

#define NEXT_POWER_OF_TWO(n)                                                                                 \
    ((n) == 0 ? 1                                                                                            \
              : (((size_t)(n) - 1) | (((size_t)(n) - 1) >> 1) | (((size_t)(n) - 1) >> 2) |                   \
                 (((size_t)(n) - 1) >> 4) | (((size_t)(n) - 1) >> 8) | (((size_t)(n) - 1) >> 16) |           \
                 (((size_t)(n) - 1) >> 32)) +                                                                \
                    1)

// // Helper function to find next power of two from n.
// static inline size_t next_power_of_two(size_t n) {
//     if (n == 0)
//         return 1;

//     // clever trick, such that if n is a power of 2,
//     // it is not pushed to the next power.
//     n--;

//     n |= n >> 1;
//     n |= n >> 2;
//     n |= n >> 4;
//     n |= n >> 8;
//     n |= n >> 16;
//     n |= n >> 32;
//     return n + 1;
// }

// Hash table node that should be embedded into payload structures
// Uses intrusive linking to avoid separate allocations
typedef struct HNode {
    struct HNode* next;  // Next node in collision chain
    uint64_t hcode;      // Precomputed hash code
} HNode;

// A fixed-size hash table with chaining
typedef struct HTab {
    HNode** tab;  // Array of slot pointers (each points to a chain)
    size_t mask;  // Bitmask for hash (size-1 where size is power of 2)
    size_t size;  // Number of items in table
} HTab;

// The main hash map interface using two tables for progressive rehashing
typedef struct HMap {
    HTab newer;          // Active table (new inserts go here)
    HTab older;          // Table being migrated from
    size_t migrate_pos;  // Next position to migrate from older table
} HMap;

// Initialize map with this given capacity.
void hm_init(HMap* hmap, size_t capacity);

// Find a node in the hash map.
HNode* hm_lookup(HMap* hmap, HNode* key, bool (*eq)(HNode*, HNode*));

// Insert a node into the hash map
void hm_insert(HMap* hmap, HNode* node);

// Remove a node from the hash map.
HNode* hm_delete(HMap* hmap, HNode* key, bool (*eq)(HNode*, HNode*));

// Clear the entire hash map and free underlying memory.
void hm_clear(HMap* hmap);

// Returns the number of elements in the hash map.
size_t hm_size(HMap* hmap);

// ForEach macro
// node is the loop variable of type HNode*
// hmap_ptr is the pointer to HMap.
// newer table first (__phase == 0)
// then older table  (__phase == 1)
// t ensures __tab->tab is non-NULL and __i is within bounds (<= mask)
#define hm_foreach(hmap_ptr, node)                                                                           \
    for (int __phase = 0; __phase < 2; __phase++)                                                            \
        for (HTab* __tab = (__phase == 0 ? &(hmap_ptr)->newer : &(hmap_ptr)->older); __tab && __tab->tab;    \
             __tab       = NULL)                                                                             \
            for (size_t __i = 0; __i <= __tab->mask; __i++)                                            \
                for (HNode* node = __tab->tab[__i]; node != NULL; node = node->next)

// --------------------------------------------------------------------------------
// Implementation of common hash functions for simple types
// --------------------------------------------------------------------------------

// FNV-1a hash function for C strings
static inline uint64_t hm_strhash(const char* s) {
    uint32_t h = 0x811C9DC5;
    while (*s) {
        h = (h ^ (uint8_t)(*s++)) * 0x01000193;
    }
    return h;
}

// Jenkins One-at-a-Time Hash for C Null-terminated strings.
static inline uint64_t hm_jenkins_hash(const char* s) {
    uint32_t h = 0;
    while (*s) {
        h += (uint8_t)(*s++);
        h += (h << 10);
        h ^= (h >> 6);
    }
    h += (h << 3);
    h ^= (h >> 11);
    h += (h << 15);
    return h;
}

// Jenkins One-at-a-Time Hash for abitrary bytes in key
// of given length.
// See https://en.wikipedia.org/wiki/Jenkins_hash_function
static inline uint32_t hm_jenkins_hash2(const uint8_t* key, size_t length) {
    uint32_t hash = 0;
    for (size_t i = 0; i < length; ++i) {
        hash += key[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

// Use 32-bit MurmurHash3 finalizer for regular int, uint32_t
static inline uint32_t hm_hash_int32(uint32_t x) {
    x ^= x >> 16;
    x *= 0x85ebca6b;
    x ^= x >> 13;
    x *= 0xc2b2ae35;
    x ^= x >> 16;
    return x;
}

// Use the 64-bit MurmurHash3 finalizer for size_t, pointers, and 64-bit integers
static inline uint64_t hm_hash_int64(uint64_t x) {
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
    return x;
}

// Hash size_t on 64 bit platforms.
static inline size_t hm_hash_size_t(size_t x) {
    return (size_t)hm_hash_int64(x);
}

#endif  // HASHMAP_H

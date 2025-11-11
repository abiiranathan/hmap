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

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "hmap.h"

/** Magic number to detect initialized HMap structures. */
#define HMAP_MAGIC 0x484D4150  // "HMAP" in hex

/** Minimum initial capacity for hash tables. */
#define MIN_CAPACITY 4

/**
 * Checks if an HMap appears to be properly initialized.
 * @param hmap The hash map to check.
 * @return true if initialized (magic number matches), false otherwise.
 */
static inline bool hm_is_initialized(const HMap* hmap) {
    return hmap && hmap->magic == HMAP_MAGIC;
}

/**
 * Ensures the hash map is initialized, auto-initializing if necessary.
 * @param hmap The hash map to validate.
 * @return true on success, false if hmap is NULL.
 */
static bool hm_ensure_initialized(HMap* hmap) {
    if (hmap == nullptr) {
        return false;
    }

    if (!hm_is_initialized(hmap)) {
        // Auto-initialize with zero state
        memset(hmap, 0, sizeof(*hmap));
        hmap->magic = HMAP_MAGIC;
    }

    return true;
}

/**
 * Initialize a hash table with given size, n (rounded to power of 2).
 * @param htab The table to initialize. Must not be NULL.
 * @param n Desired capacity (will be rounded up to power of 2).
 * @return true on success, false on allocation failure.
 */
static bool hm_table_init(HTab* htab, size_t n) {
    assert(htab != nullptr);

    // Enforce minimum capacity
    if (n < MIN_CAPACITY) {
        n = MIN_CAPACITY;
    }

    // Round up to power of 2 if needed
    if ((n & (n - 1)) != 0) {
        n = NEXT_POWER_OF_TWO(n);
    }

    // Allocate slot array
    HNode** tab = (HNode**)calloc(n, sizeof(HNode*));
    if (tab == nullptr) {
        return false;  // Allocation failure
    }

    htab->tab  = tab;
    htab->mask = n - 1;
    htab->size = 0;

    return true;
}

/**
 * Insert a node into a hash table (internal).
 * @param htab The table to insert into. Must be initialized.
 * @param node The node to insert. Must not be NULL.
 */
static void h_insert(HTab* htab, HNode* node) {
    assert(htab != nullptr && htab->tab != nullptr);
    assert(node != nullptr);

    size_t pos     = node->hcode & htab->mask;
    HNode* next    = htab->tab[pos];
    node->next     = next;
    htab->tab[pos] = node;
    htab->size++;
}

/**
 * Find a node and return pointer to the slot that points to it.
 * @param htab The table to search. May be uninitialized.
 * @param key The key node to search for. Must not be NULL.
 * @param eq Equality comparator function. Must not be NULL.
 * @return Pointer to the slot containing the node, or NULL if not found.
 */
static HNode** h_lookup(HTab* htab, HNode* key, bool (*eq)(HNode*, HNode*)) {
    assert(key != nullptr && eq != nullptr);

    if (!htab->tab) {
        return nullptr;
    }

    size_t pos   = key->hcode & htab->mask;
    HNode** from = &htab->tab[pos];

    // Walk collision chain
    for (HNode* cur; (cur = *from) != nullptr; from = &cur->next) {
        if (cur->hcode == key->hcode && eq(cur, key)) {
            return from;
        }
    }

    return nullptr;
}

/**
 * Remove a node from its chain.
 * @param htab The table containing the node.
 * @param from Pointer to the slot pointing to the node.
 * @return The detached node.
 */
static HNode* h_detach(HTab* htab, HNode** from) {
    assert(htab != nullptr && from != nullptr && *from != nullptr);

    HNode* node = *from;
    *from       = node->next;
    htab->size--;

    return node;
}

/** Amount of work to do per progressive rehashing step. */
static const size_t k_rehashing_work = 128;

/**
 * Migrate a limited number of items from older to newer table.
 * @param hmap The hash map performing progressive rehashing.
 */
static void hm_help_rehashing(HMap* hmap) {
    assert(hmap != nullptr);

    if (!hmap->older.tab) {
        return;  // No rehashing in progress
    }

    size_t nwork       = 0;
    size_t old_buckets = hmap->older.mask + 1;

    // Do limited work per call
    while (nwork < k_rehashing_work && hmap->older.size > 0) {
        // Bounds check on migrate_pos
        if (hmap->migrate_pos >= old_buckets) {
            break;
        }

        HNode** from = &hmap->older.tab[hmap->migrate_pos];
        if (!*from) {
            hmap->migrate_pos++;
            continue;
        }

        // Move one chain item to newer table
        h_insert(&hmap->newer, h_detach(&hmap->older, from));
        nwork++;
    }

    // Clean up when migration complete
    if (hmap->older.size == 0 && hmap->older.tab) {
        free((void*)hmap->older.tab);
        memset(&hmap->older, 0, sizeof(hmap->older));
    }
}

/**
 * Start rehashing by making newer table the older one and allocating bigger table.
 * @param hmap The hash map to trigger rehashing on.
 * @return true on success, false on allocation failure.
 */
static bool hm_trigger_rehashing(HMap* hmap) {
    assert(hmap != nullptr);
    assert(hmap->older.tab == nullptr);  // Shouldn't be rehashing already

    size_t new_capacity = (hmap->newer.mask + 1) * 2;

    // Temporary table for new allocation
    HTab new_table = {0};
    if (!hm_table_init(&new_table, new_capacity)) {
        return false;  // Allocation failed, keep current table
    }

    hmap->older       = hmap->newer;  // Current becomes older
    hmap->newer       = new_table;    // New table becomes newer
    hmap->migrate_pos = 0;

    return true;
}

/** Maximum load factor (items per bucket) before triggering rehash. */
static const size_t k_max_load_factor = 8;

/**
 * Find a node in the hash map.
 * @param hmap The hash map to search. Auto-initializes if needed.
 * @param key The key node to search for. Must not be NULL.
 * @param eq Equality comparator. Must not be NULL.
 * @return The found node, or NULL if not found or hmap is NULL.
 */
HNode* hm_lookup(HMap* hmap, HNode* key, bool (*eq)(HNode*, HNode*)) {
    if (!hm_ensure_initialized(hmap) || key == nullptr || eq == nullptr) {
        return nullptr;
    }

    hm_help_rehashing(hmap);

    // Search newer then older table
    HNode** from = h_lookup(&hmap->newer, key, eq);
    if (!from) {
        from = h_lookup(&hmap->older, key, eq);
    }

    return from ? *from : nullptr;
}

/**
 * Insert a node into the hash map.
 * @param hmap The hash map. Auto-initializes if needed.
 * @param node The node to insert. Must not be NULL.
 * @return true on success, false on failure (NULL inputs or allocation error).
 */
bool hm_insert(HMap* hmap, HNode* node) {
    if (!hm_ensure_initialized(hmap) || node == nullptr) {
        return false;
    }

    // Lazy initialization of first table
    if (!hmap->newer.tab) {
        if (!hm_table_init(&hmap->newer, MIN_CAPACITY)) {
            return false;  // Allocation failed
        }
    }

    h_insert(&hmap->newer, node);

    // Check if we need to start rehashing
    if (!hmap->older.tab) {
        size_t threshold = (hmap->newer.mask + 1) * k_max_load_factor;
        if (hmap->newer.size >= threshold) {
            // Trigger rehashing, but don't fail insertion if it fails
            // (growth is a best-effort optimization)
            hm_trigger_rehashing(hmap);
        }
    }

    hm_help_rehashing(hmap);

    return true;
}

/**
 * Remove a node from the hash map.
 * @param hmap The hash map. Auto-initializes if needed.
 * @param key The key node to remove. Must not be NULL.
 * @param eq Equality comparator. Must not be NULL.
 * @return The removed node, or NULL if not found or invalid inputs.
 */
HNode* hm_delete(HMap* hmap, HNode* key, bool (*eq)(HNode*, HNode*)) {
    if (!hm_ensure_initialized(hmap) || key == nullptr || eq == nullptr) {
        return nullptr;
    }

    hm_help_rehashing(hmap);

    // Try newer then older table
    HNode** from = h_lookup(&hmap->newer, key, eq);
    if (from) {
        return h_detach(&hmap->newer, from);
    }

    from = h_lookup(&hmap->older, key, eq);
    if (from) {
        return h_detach(&hmap->older, from);
    }

    return nullptr;
}

/**
 * Clear the entire hash map and free underlying memory.
 * Safe to call on uninitialized or NULL maps.
 * @param hmap The hash map to clear. May be NULL.
 */
void hm_clear(HMap* hmap) {
    if (hmap == nullptr) {
        return;
    }

    free((void*)hmap->newer.tab);
    free((void*)hmap->older.tab);
    memset(hmap, 0, sizeof(*hmap));
    hmap->magic = HMAP_MAGIC;  // Keep initialized marker
}

/**
 * Get the total number of elements in the hash map.
 * @param hmap The hash map. Auto-initializes if needed.
 * @return Total element count, or 0 if hmap is NULL.
 */
size_t hm_size(const HMap* hmap) {
    if (!hm_is_initialized(hmap)) {
        return 0;
    }

    return hmap->newer.size + hmap->older.size;
}

/**
 * Resize the hash map to a specific capacity.
 * @param hmap The hash map. Auto-initializes if needed.
 * @param new_capacity Desired capacity (must be power of 2).
 * @return true on success, false on allocation failure or invalid inputs.
 */
bool hm_resize(HMap* hmap, size_t new_capacity) {
    if (!hm_ensure_initialized(hmap)) {
        return false;
    }

    // Validate power of 2
    if (new_capacity == 0 || (new_capacity & (new_capacity - 1)) != 0) {
        return false;
    }

    size_t current_size = hmap->newer.size + hmap->older.size;

    // Don't resize smaller than current elements
    if (new_capacity < current_size) {
        new_capacity = NEXT_POWER_OF_TWO(current_size);
        if (new_capacity < MIN_CAPACITY) {
            new_capacity = MIN_CAPACITY;
        }
    }

    // If already at correct capacity, do nothing
    if (hmap->newer.tab && (hmap->newer.mask + 1) == new_capacity && !hmap->older.tab) {
        return true;
    }

    // If currently rehashing, finish it first
    while (hmap->older.size > 0) {
        hm_help_rehashing(hmap);
    }

    // Check again after completing migration
    if (hmap->newer.tab && (hmap->newer.mask + 1) == new_capacity) {
        return true;
    }

    // Trigger progressive rehashing to new size
    HTab new_table = {0};
    if (!hm_table_init(&new_table, new_capacity)) {
        return false;  // Allocation failed
    }

    hmap->older       = hmap->newer;
    hmap->newer       = new_table;
    hmap->migrate_pos = 0;

    // Do some initial migration work
    hm_help_rehashing(hmap);

    return true;
}

/**
 * Initialize hash map with given capacity.
 * @param hmap The hash map to initialize. Must not be NULL.
 * @param capacity Initial capacity (will be rounded to power of 2).
 * @return true on success, false on allocation failure.
 */
bool hm_init(HMap* hmap, size_t capacity) {
    if (hmap == nullptr) {
        return false;
    }

    // Zero out entire structure
    memset(hmap, 0, sizeof(*hmap));
    hmap->magic = HMAP_MAGIC;

    // Initialize newer table
    if (capacity > 0) {
        return hm_table_init(&hmap->newer, capacity);
    }

    return true;  // Success with lazy initialization
}

/**
 * Destroy hash map and free all memory.
 * Alias for hm_clear() for API clarity.
 * @param hmap The hash map to destroy. May be NULL.
 */
void hm_destroy(HMap* hmap) {
    hm_clear(hmap);
}

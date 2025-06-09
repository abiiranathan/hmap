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

#include "hmap.h"

// Initialize a hash table with given size, n (must be power of 2)
static void hm_init(HTab* htab, size_t n) {
    assert(n > 0 && ((n - 1) & n) == 0);              // Verify power of 2
    htab->tab  = (HNode**)calloc(n, sizeof(HNode*));  // Allocate slot array
    htab->mask = n - 1;                               // Size mask (for fast modulo)
    htab->size = 0;                                   // Empty table
}

// Insert a node into a hash table
static void h_insert(HTab* htab, HNode* node) {
    size_t pos     = node->hcode & htab->mask;  // Compute slot index
    HNode* next    = htab->tab[pos];            // Get current chain head
    node->next     = next;                      // Link new node
    htab->tab[pos] = node;                      // Update chain head
    htab->size++;                               // Increase count
}

// Find a node and return pointer to the slot that points to it
static HNode** h_lookup(HTab* htab, HNode* key, bool (*eq)(HNode*, HNode*)) {
    if (!htab->tab) {
        return NULL;
    }

    size_t pos   = key->hcode & htab->mask;  // Compute slot
    HNode** from = &htab->tab[pos];          // Pointer to chain head

    // Walk collision chain
    for (HNode* cur; (cur = *from) != NULL; from = &cur->next) {
        // compare hash code first to eliminate most non-matches.
        if (cur->hcode == key->hcode && eq(cur, key)) {
            return from;  // Found match
        }
    }
    return NULL;  // Not found
}

// Remove a node from its chain
static HNode* h_detach(HTab* htab, HNode** from) {
    HNode* node = *from;       // Get target node
    *from       = node->next;  // Update pointer to skip node
    htab->size--;              // Decrease count
    return node;
}

const size_t k_rehashing_work = 128;  // constant work

// Migrate a limited number of items from older to newer table
static void hm_help_rehashing(HMap* hmap) {
    size_t nwork = 0;

    // Do limited work per call
    while (nwork < k_rehashing_work && hmap->older.size > 0) {
        HNode** from = &hmap->older.tab[hmap->migrate_pos];
        if (!*from) {
            hmap->migrate_pos++;  // Skip empty slots
            continue;
        }

        // Move one chain item to newer table
        h_insert(&hmap->newer, h_detach(&hmap->older, from));
        nwork++;
    }

    // Clean up if done
    if (hmap->older.size == 0 && hmap->older.tab) {
        free((void*)hmap->older.tab);
        hmap->older = (HTab){};
    }
}

// moves some keys to the newer table. O(1)
// Start rehashing by making newer table the older one and allocating bigger table
static void hm_trigger_rehashing(HMap* hmap) {
    assert(!hmap->older.tab);  // Shouldn't be rehashing already

    hmap->older = hmap->newer;                          // Current table becomes older
    hm_init(&hmap->newer, (hmap->newer.mask + 1) * 2);  // Double size
    hmap->migrate_pos = 0;                              // Start migrating from slot 0
}

// Find a node in the hash map
HNode* hm_lookup(HMap* hmap, HNode* key, bool (*eq)(HNode*, HNode*)) {
    hm_help_rehashing(hmap);  // Help with ongoing rehashing

    // Search newer then older table
    HNode** from = h_lookup(&hmap->newer, key, eq);
    if (!from) {
        from = h_lookup(&hmap->older, key, eq);
    }

    return from ? *from : NULL;
}

// Max Load factor. For chainable hashtable, this should be > 1
// as each slot is intended to hold multiple items.
const size_t k_max_load_factor = 8;

// Insert a node into the hash map
void hm_insert(HMap* hmap, HNode* node) {
    if (!hmap->newer.tab) {
        hm_init(&hmap->newer, 4);  // Initial size 4 if empty
    }

    h_insert(&hmap->newer, node);  // Always insert to newer table

    // Check if we need to start rehashing
    if (!hmap->older.tab) {
        size_t threshold = (hmap->newer.mask + 1) * k_max_load_factor;
        if (hmap->newer.size >= threshold) {
            hm_trigger_rehashing(hmap);
        }
    }

    hm_help_rehashing(hmap);  // Do some rehashing work
}

// Remove a node from the hash map
HNode* hm_delete(HMap* hmap, HNode* key, bool (*eq)(HNode*, HNode*)) {
    hm_help_rehashing(hmap);  // Help with ongoing rehashing

    // Try newer then older table
    HNode** from = h_lookup(&hmap->newer, key, eq);
    if (from) {
        return h_detach(&hmap->newer, from);
    }
    from = h_lookup(&hmap->older, key, eq);
    if (from) {
        return h_detach(&hmap->older, from);
    }
    return NULL;
}

// Clear the entire hash map and free underlying memory.
void hm_clear(HMap* hmap) {
    free((void*)hmap->newer.tab);
    free((void*)hmap->older.tab);
    *hmap = (HMap){};
}

// Get the total number of elements in the hash map
size_t hm_size(HMap* hmap) {
    return hmap->newer.size + hmap->older.size;
}

// Helper function to find next power of two
static inline size_t next_power_of_two(size_t n) {
    if (n == 0)
        return 1;
    n--;

    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    return n + 1;
}

// Optimized resize that leverages existing progressive rehashing
void hm_resize(HMap* hmap, size_t new_capacity) {
    assert(new_capacity > 0 && ((new_capacity - 1) & new_capacity) == 0);

    // Calculate total current size
    size_t current_size = hmap->newer.size + hmap->older.size;

    // Don't resize smaller than current elements
    if (new_capacity < current_size) {
        new_capacity = next_power_of_two(current_size);
    }

    // If already at correct capacity, do nothing
    if (hmap->newer.tab && (hmap->newer.mask + 1) == new_capacity && !hmap->older.tab) {
        return;
    }

    // If currently rehashing, finish it first
    while (hmap->older.size > 0) {
        hm_help_rehashing(hmap);
    }

    // Now we have only newer table with all elements
    // Check if we need to resize
    if ((hmap->newer.mask + 1) == new_capacity) {
        return;  // Already correct size
    }

    // Trigger progressive rehashing to new size
    hmap->older = hmap->newer;            // Current becomes older
    hm_init(&hmap->newer, new_capacity);  // Create new table
    hmap->migrate_pos = 0;                // Start migration

    // Do some initial migration work (optional - keeps it non-blocking)
    hm_help_rehashing(hmap);
}

// If you want immediate resize (blocking but simpler)
void hm_resize_immediate(HMap* hmap, size_t new_capacity) {
    assert(new_capacity > 0 && ((new_capacity - 1) & new_capacity) == 0);

    size_t current_size = hmap->newer.size + hmap->older.size;

    if (new_capacity < current_size) {
        new_capacity = next_power_of_two(current_size);
    }

    if (hmap->newer.tab && (hmap->newer.mask + 1) == new_capacity && !hmap->older.tab) {
        return;
    }

    // Force complete any ongoing rehashing first
    while (hmap->older.size > 0) {
        hm_help_rehashing(hmap);
    }

    // If already correct size after completing rehashing
    if ((hmap->newer.mask + 1) == new_capacity) {
        return;
    }

    // Do immediate full migration (your current approach)
    HTab new_tab;
    hm_init(&new_tab, new_capacity);

    // Migrate all nodes from newer table
    for (size_t i = 0; i <= hmap->newer.mask; i++) {
        HNode* node = hmap->newer.tab[i];
        while (node) {
            HNode* next = node->next;
            h_insert(&new_tab, node);
            node = next;
        }
    }

    free(hmap->newer.tab);
    hmap->newer       = new_tab;
    hmap->older       = (HTab){};
    hmap->migrate_pos = 0;
}

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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hmap.h"

// Item type with embedded HNode
typedef struct Item {
    HNode node;       // Embedded hash node
    const char* key;  // Key string (assume valid pointer lifetime)
    int value;        // Associated value
} Item;

// String equality comparator for HNode keys
bool item_eq(HNode* a, HNode* b) {
    Item* item_a = container_of(a, Item, node);
    Item* item_b = container_of(b, Item, node);
    return strcmp(item_a->key, item_b->key) == 0;
}

int main() {
    HMap hmap = {};

    // Test data
    const char* keys[] = {"apple", "banana", "cherry", "date", "elderberry"};
    Item items[5];

    // Insert items into hashmap
    for (int i = 0; i < 5; i++) {
        items[i].key        = keys[i];  // Valid lifetime for this main function
        items[i].value      = i * 10;   // Values don't have to be void pointers
        items[i].node.hcode = hm_strhash(items[i].key);
        hm_insert(&hmap, &items[i].node);
    }

    // Lookup test
    for (int i = 0; i < 5; i++) {
        const char* key = keys[i];
        Item key_item   = {
              .key        = key,
              .node.hcode = hm_strhash(key),
              .node.next  = NULL,
        };

        HNode* found = hm_lookup(&hmap, &key_item.node, item_eq);
        if (found) {
            Item* item = container_of(found, Item, node);
            printf("Found: %s = %d\n", item->key, item->value);
        } else {
            printf("Not found: %s\n", key);
        }
    }

    // Test deletion
    Item del_item = {
        .key        = "banana",
        .node.hcode = hm_strhash("banana"),
        .node.next  = NULL,
    };

    HNode* deleted = hm_delete(&hmap, &del_item.node, item_eq);
    if (deleted) {
        Item* item = container_of(deleted, Item, node);
        printf("Deleted: %s\n", item->key);
    }

    // Confirm deletion
    if (!hm_lookup(&hmap, &del_item.node, item_eq)) {
        printf("Confirmed banana is deleted\n");
    }

    // Print final size
    printf("Final size: %zu\n", hm_size(&hmap));
    puts("\n==========Testing foreach macro===========");

    hm_foreach(&hmap, node) {
        Item* item = container_of(node, Item, node);
        printf("Got Item: %s: %d, i=%lu\n", item->key, item->value, __i);
    }

    // Clean up
    hm_clear(&hmap);

    return 0;
}

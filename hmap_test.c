#include "hmap.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Item type with embedded HNode
typedef struct Item {
    HNode node;
    const char* key;
    int value;
} Item;

// Equality function
bool item_eq(HNode* a, HNode* b) {
    Item* ia = container_of(a, Item, node);
    Item* ib = container_of(b, Item, node);
    return strcmp(ia->key, ib->key) == 0;
}

void test_insert_and_lookup() {
    puts("== test_insert_and_lookup ==");
    HMap map = {};

    const char* keys[] = {"a", "b", "c", "d", "e"};
    Item items[5];

    for (int i = 0; i < 5; i++) {
        items[i] = (Item){
            .key   = keys[i],
            .value = i * 100,
        };
        items[i].node.hcode = hm_strhash(items[i].key);
        hm_insert(&map, &items[i].node);
    }

    for (int i = 0; i < 5; i++) {
        Item probe   = {.key = keys[i], .node = {.hcode = hm_strhash(keys[i])}};
        HNode* found = hm_lookup(&map, &probe.node, item_eq);
        assert(found);
        Item* found_item = container_of(found, Item, node);
        assert(found_item->value == i * 100);
    }

    assert(hm_size(&map) == 5);
    hm_clear(&map);
    puts("PASS");
}

void test_deletion() {
    puts("== test_deletion ==");
    HMap map  = {};
    Item item = {
        .key   = "delete_me",
        .value = 123,
    };
    item.node.hcode = hm_strhash(item.key);
    hm_insert(&map, &item.node);

    Item probe = {.key = "delete_me", .node = {.hcode = hm_strhash("delete_me")}};

    assert(hm_lookup(&map, &probe.node, item_eq));
    HNode* deleted = hm_delete(&map, &probe.node, item_eq);
    assert(deleted != NULL);
    assert(hm_lookup(&map, &probe.node, item_eq) == NULL);
    assert(hm_size(&map) == 0);
    hm_clear(&map);
    puts("PASS");
}

void test_collision_handling() {
    puts("== test_collision_handling ==");
    HMap map = {};

    // Force collisions by overriding the hcode to same value
    Item a = {.key = "apple", .value = 1, .node.hcode = 42};
    Item b = {.key = "banana", .value = 2, .node.hcode = 42};

    hm_insert(&map, &a.node);
    hm_insert(&map, &b.node);

    Item probe_a = {.key = "apple", .node = {.hcode = 42}};
    Item probe_b = {.key = "banana", .node = {.hcode = 42}};

    assert(hm_lookup(&map, &probe_a.node, item_eq));
    assert(hm_lookup(&map, &probe_b.node, item_eq));
    assert(hm_size(&map) == 2);

    hm_clear(&map);
    puts("PASS");
}

void test_iteration() {
    puts("== test_iteration ==");
    HMap map      = {};
    Item items[3] = {
        {.key = "k1", .value = 10},
        {.key = "k2", .value = 20},
        {.key = "k3", .value = 30},
    };

    for (int i = 0; i < 3; i++) {
        items[i].node.hcode = hm_strhash(items[i].key);
        hm_insert(&map, &items[i].node);
    }

    size_t count = 0;
    int total    = 0;

    hm_foreach(&map, node) {
        Item* item = container_of(node, Item, node);
        total += item->value;
        count++;
    }

    assert(count == 3);
    assert(total == 60);
    puts("PASS");
    hm_clear(&map);
}

void test_clear_and_reuse() {
    puts("== test_clear_and_reuse ==");
    HMap map = {};

    Item a       = {.key = "a", .value = 1};
    a.node.hcode = hm_strhash(a.key);
    hm_insert(&map, &a.node);
    assert(hm_size(&map) == 1);

    hm_clear(&map);
    assert(hm_size(&map) == 0);

    // Reuse map
    Item b       = {.key = "b", .value = 2};
    b.node.hcode = hm_strhash(b.key);
    hm_insert(&map, &b.node);
    assert(hm_size(&map) == 1);

    hm_clear(&map);
    puts("PASS");
}

void test_reinsertion() {
    puts("== test_reinsertion ==");
    HMap map = {};

    Item item       = {.key = "hello", .value = 42};
    item.node.hcode = hm_strhash(item.key);
    hm_insert(&map, &item.node);

    Item probe   = {.key = "hello", .node = {.hcode = hm_strhash("hello")}};
    HNode* found = hm_lookup(&map, &probe.node, item_eq);
    assert(found);
    assert(container_of(found, Item, node)->value == 42);

    // Delete
    hm_delete(&map, &probe.node, item_eq);
    assert(hm_lookup(&map, &probe.node, item_eq) == NULL);
    assert(hm_size(&map) == 0);

    // Reinsert
    item.node.next = NULL;  // Safe reset
    hm_insert(&map, &item.node);
    found = hm_lookup(&map, &probe.node, item_eq);
    assert(found);
    assert(container_of(found, Item, node)->value == 42);

    hm_clear(&map);
    puts("PASS");
}

void test_resize_basic() {
    puts("== test_resize_basic ==");
    HMap map = {};

    // Insert some items
    Item items[4];
    const char* keys[] = {"a", "b", "c", "d"};

    for (int i = 0; i < 4; i++) {
        items[i]            = (Item){.key = keys[i], .value = i * 10};
        items[i].node.hcode = hm_strhash(items[i].key);
        hm_insert(&map, &items[i].node);
    }

    assert(hm_size(&map) == 4);

    // Resize to larger capacity
    hm_resize(&map, 16);

    // All items should still be findable
    for (int i = 0; i < 4; i++) {
        Item probe   = {.key = keys[i], .node = {.hcode = hm_strhash(keys[i])}};
        HNode* found = hm_lookup(&map, &probe.node, item_eq);
        assert(found);
        Item* found_item = container_of(found, Item, node);
        assert(found_item->value == i * 10);
    }

    assert(hm_size(&map) == 4);
    hm_clear(&map);
    puts("PASS");
}

void test_resize_smaller() {
    puts("== test_resize_smaller ==");
    HMap map = {};

    // Insert many items to force larger table
    Item items[20];
    char keys[20][10];

    for (int i = 0; i < 20; i++) {
        snprintf(keys[i], sizeof(keys[i]), "key%d", i);
        items[i]            = (Item){.key = keys[i], .value = i};
        items[i].node.hcode = hm_strhash(items[i].key);
        hm_insert(&map, &items[i].node);
    }

    assert(hm_size(&map) == 20);

    // Try to resize smaller than current elements (should adjust to fit)
    hm_resize(&map, 8);  // Should become next power of 2 >= 20 = 32

    // All items should still be accessible
    for (int i = 0; i < 20; i++) {
        Item probe   = {.key = keys[i], .node = {.hcode = hm_strhash(keys[i])}};
        HNode* found = hm_lookup(&map, &probe.node, item_eq);
        assert(found);
        assert(container_of(found, Item, node)->value == i);
    }

    assert(hm_size(&map) == 20);
    hm_clear(&map);
    puts("PASS");
}

void test_resize_during_rehashing() {
    puts("== test_resize_during_rehashing ==");
    HMap map = {};

    // Insert enough items to trigger rehashing
    Item items[50];
    char keys[50][10];

    for (int i = 0; i < 50; i++) {
        snprintf(keys[i], sizeof(keys[i]), "k%d", i);
        items[i]            = (Item){.key = keys[i], .value = i * 2};
        items[i].node.hcode = hm_strhash(items[i].key);
        hm_insert(&map, &items[i].node);

        // Stop partway to ensure we're in middle of rehashing
        if (i == 35 && map.older.tab != NULL) {
            break;  // We have ongoing rehashing
        }
    }

    size_t initial_size = hm_size(&map);

    // Resize while rehashing is in progress
    hm_resize(&map, 128);

    // All inserted items should still be accessible
    for (int i = 0; i <= 35; i++) {
        Item probe   = {.key = keys[i], .node = {.hcode = hm_strhash(keys[i])}};
        HNode* found = hm_lookup(&map, &probe.node, item_eq);
        assert(found);
        assert(container_of(found, Item, node)->value == i * 2);
    }

    assert(hm_size(&map) == initial_size);
    hm_clear(&map);
    puts("PASS");
}

void test_resize_empty_map() {
    puts("== test_resize_empty_map ==");
    HMap map = {};

    // Resize empty map
    hm_resize(&map, 16);
    assert(hm_size(&map) == 0);

    // Should be able to insert after resize
    Item item       = {.key = "test", .value = 42};
    item.node.hcode = hm_strhash(item.key);
    hm_insert(&map, &item.node);

    Item probe   = {.key = "test", .node = {.hcode = hm_strhash("test")}};
    HNode* found = hm_lookup(&map, &probe.node, item_eq);
    assert(found);
    assert(container_of(found, Item, node)->value == 42);

    hm_clear(&map);
    puts("PASS");
}

void test_resize_with_collisions() {
    puts("== test_resize_with_collisions ==");
    HMap map = {};

    // Create items with forced collisions
    Item items[6];
    const char* keys[] = {"apple", "banana", "cherry", "date", "elderberry", "fig"};

    for (int i = 0; i < 6; i++) {
        items[i] = (Item){.key = keys[i], .value = i * 5};
        // Force all to same hash code to create long collision chain
        items[i].node.hcode = 12345;
        hm_insert(&map, &items[i].node);
    }

    assert(hm_size(&map) == 6);

    // Resize should preserve collision chain integrity
    hm_resize(&map, 32);

    // All items should still be findable despite collisions
    for (int i = 0; i < 6; i++) {
        Item probe   = {.key = keys[i], .node = {.hcode = 12345}};
        HNode* found = hm_lookup(&map, &probe.node, item_eq);
        assert(found);
        assert(container_of(found, Item, node)->value == i * 5);
    }

    assert(hm_size(&map) == 6);
    hm_clear(&map);
    puts("PASS");
}

void test_resize_then_operations() {
    puts("== test_resize_then_operations ==");
    HMap map = {};

    // Insert initial items
    Item items[5];
    const char* keys[] = {"one", "two", "three", "four", "five"};

    for (int i = 0; i < 5; i++) {
        items[i]            = (Item){.key = keys[i], .value = i};
        items[i].node.hcode = hm_strhash(items[i].key);
        hm_insert(&map, &items[i].node);
    }

    // Resize
    hm_resize(&map, 64);

    // Test operations after resize

    // 1. Lookup existing
    Item probe   = {.key = "three", .node = {.hcode = hm_strhash("three")}};
    HNode* found = hm_lookup(&map, &probe.node, item_eq);
    assert(found && container_of(found, Item, node)->value == 2);

    // 2. Delete existing
    HNode* deleted = hm_delete(&map, &probe.node, item_eq);
    assert(deleted);
    assert(hm_size(&map) == 4);

    // 3. Insert new
    Item new_item       = {.key = "six", .value = 6};
    new_item.node.hcode = hm_strhash(new_item.key);
    hm_insert(&map, &new_item.node);
    assert(hm_size(&map) == 5);

    // 4. Verify new item findable
    Item new_probe = {.key = "six", .node = {.hcode = hm_strhash("six")}};
    found          = hm_lookup(&map, &new_probe.node, item_eq);
    assert(found && container_of(found, Item, node)->value == 6);

    hm_clear(&map);
    puts("PASS");
}

void test_resize_immediate() {
    puts("== test_resize_immediate ==");
    HMap map = {};

    // Insert initial items
    Item items[5];
    const char* keys[] = {"one", "two", "three", "four", "five"};

    for (int i = 0; i < 5; i++) {
        items[i]            = (Item){.key = keys[i], .value = i};
        items[i].node.hcode = hm_strhash(items[i].key);
        hm_insert(&map, &items[i].node);
    }

    // Resize
    hm_resize_immediate(&map, 64);

    // Test operations after resize

    // 1. Lookup existing
    Item probe   = {.key = "three", .node = {.hcode = hm_strhash("three")}};
    HNode* found = hm_lookup(&map, &probe.node, item_eq);
    assert(found && container_of(found, Item, node)->value == 2);

    // 2. Delete existing
    HNode* deleted = hm_delete(&map, &probe.node, item_eq);
    assert(deleted);
    assert(hm_size(&map) == 4);

    // 3. Insert new
    Item new_item       = {.key = "six", .value = 6};
    new_item.node.hcode = hm_strhash(new_item.key);
    hm_insert(&map, &new_item.node);
    assert(hm_size(&map) == 5);

    // 4. Verify new item findable
    Item new_probe = {.key = "six", .node = {.hcode = hm_strhash("six")}};
    found          = hm_lookup(&map, &new_probe.node, item_eq);
    assert(found && container_of(found, Item, node)->value == 6);

    hm_clear(&map);
    puts("PASS");
}

int main() {
    test_insert_and_lookup();
    test_deletion();
    test_collision_handling();
    test_iteration();
    test_clear_and_reuse();
    test_reinsertion();

    // Resizing
    test_resize_basic();
    test_resize_smaller();
    test_resize_during_rehashing();
    test_resize_empty_map();
    test_resize_with_collisions();
    test_resize_immediate();

    puts("\nAll tests passed.");
    return 0;
}

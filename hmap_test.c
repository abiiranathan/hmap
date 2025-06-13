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

int main() {
    test_insert_and_lookup();
    test_deletion();
    test_collision_handling();
    test_iteration();
    test_clear_and_reuse();
    test_reinsertion();

    puts("\nAll tests passed.");
    return 0;
}

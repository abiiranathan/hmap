#include <hmap.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Entity pool - single allocation for 10K entities
#define MAX_ENTITIES 10000

typedef struct Entity {
    uint32_t id;
    char name[32];
    float x, y;
    HNode hnode;
} Entity;

static Entity entity_pool[MAX_ENTITIES];
static size_t entity_count = 0;

// Hash function
static uint64_t hash_u32(uint32_t x) {
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}

// Equality function
static bool entity_eq(HNode* a, HNode* b) {
    Entity* ea = container_of(a, Entity, hnode);
    Entity* eb = container_of(b, Entity, hnode);
    return ea->id == eb->id;
}

// Add entity from pool.
Entity* spawn_entity(HMap* map, uint32_t id, const char* name, float x, float y) {
    if (entity_count >= MAX_ENTITIES) {
        return nullptr;  // Pool exhausted
    }

    Entity* e = &entity_pool[entity_count++];
    e->id     = id;
    strncpy(e->name, name, sizeof(e->name) - 1);
    e->x           = x;
    e->y           = y;
    e->hnode.hcode = hash_u32(id);
    hm_insert(map, &e->hnode);
    return e;
}

// Find entity - uses stack for search key
Entity* find_entity(HMap* map, uint32_t id) {
    // Temporary search key on stack
    Entity search = {.id = id, .hnode.hcode = hash_u32(id)};

    HNode* node = hm_lookup(map, &search.hnode, entity_eq);
    return node ? container_of(node, Entity, hnode) : nullptr;
}

int main(void) {
    HMap entity_map = {0};
    hm_init(&entity_map, 1024);

    // Spawn 1000 entities - ZERO malloc calls!
    for (uint32_t i = 0; i < 1000; i++) {
        char name[32];
        snprintf(name, 32, "Entity_%u", i);
        spawn_entity(&entity_map, i, name, i * 1.5f, i * 2.0f);
    }

    printf("Spawned 1000 entities with ZERO heap allocations!\n");
    printf("Map size: %zu\n", hm_size(&entity_map));

    // Look up using stack-allocated search key
    Entity* found = find_entity(&entity_map, 42);
    if (found) {
        printf("Found: %s at (%.1f, %.1f)\n", found->name, found->x, found->y);
    }

    hm_clear(&entity_map);
    return 0;
}

# 🔗 C HashMap Library

**Efficient, embeddable, and minimal hash map implementation in pure C.**

![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)

This library provides a fast and memory-efficient hash map (dictionary) for C projects. Designed for high performance with separate chaining and incremental rehashing, it supports generic key-value storage via embedded node structs.

---

## ✨ Features

* 🧠 **Amortized O(1)** for insertion, deletion, and lookup
* 🔁 **Incremental rehashing** — avoids large pauses during growth
* 🔩 **Generic support** for any key/value via embedded nodes
* 🧺 **Separate chaining** with linked lists to handle collisions
* ⚡ **Power-of-two** buckets for fast hash masking
* 🧼 **Memory cleanup** with `hm_clear()`
* 📏 **Load-factor based resizing** (threshold: 8)
* 🚫 **Not thread-safe** — suitable for single-threaded use

---

## 📦 Installation

1. Just add `hmap.h` and `hmap.c` to your project:

```
your_project/
├── hmap.h
├── hmap.c
└── main.c
```

Then include it:

```c
#include "hmap.h"
```

Compile normally:

```bash
gcc main.c hmap.c -o your_program
```
2. Use `cmake`


Clone the repo to the root of your project.

In your `CMakeLists.txt`, add
```txt
add_add_subdirectory(hmap)

target_link_libraries(your_lib/executable PRIVATE hmap::hmap)
```

3. Clone the repository and install the library system-wide.
```bash
git clone https://github.com/abiiranathan/hmap.git
cd hmap
sudo ninja install

# if you don't have ninja installed,
# sudo cmake --install build --prefix /usr/local

# Link with hmap
gcc myapp.c -lhmap

```

When installed globally, link with cmake:

```txt
find_package(hmap REQUIRED)

target_link_libraries(youapp PRIVATE hmap::hmap)
```

4. Once installed, you can also use pkg-config to link with it.

```bash
gcc main.c `pkg-config --cflags --libs hmap`
```

---

## 🚀 Quick Start

### 1. Define your item structure:

```c
typedef struct {
    HNode node;       // Required for hashmap functionality
    const char* key;  // Your key (e.g. string)
    int value;        // Your value
} Item;
```

### 2. Define an equality function:

```c
bool item_eq(HNode* a, HNode* b) {
    Item* ia = container_of(a, Item, node);
    Item* ib = container_of(b, Item, node);
    return strcmp(ia->key, ib->key) == 0;
}
```

### 3. Insert, lookup, delete:

```c
Item item = { .key = "apple", .value = 42 };
item.node.hcode = hm_strhash(item.key);
hm_insert(&map, &item.node);

Item key_item = { .key = "apple", .node.hcode = hm_strhash("apple") };
HNode* found = hm_lookup(&map, &key_item.node, item_eq);
if (found) {
    Item* result = container_of(found, Item, node);
    printf("%s => %d\n", result->key, result->value);
}

hm_delete(&map, &key_item.node, item_eq);
hm_clear(&map);
```

---

## 🧪 Testing

See `hmap_test.c` for a full test suite, including:

```bash
make test
```

or 
```bash
cd build
ctest --output-on-failure
```

---

## 🛠️ Hashing

This library provides a built-in `hm_strhash()` (FNV-1a) for string keys. You can plug in your own hashing function if needed or include [hash.h](./hash.h) that contains various hash functions.

---

## 🧩 Internals

* `HNode`: Base node embedded in your structs.
* `HTab`: Internal hash table used for buckets.
* `HMap`: Public hashmap structure you interact with.

All operations (insert, delete, lookup, size, foreach) are exposed through `hmap.h`.

---

## 🧼 Clean-Up

Don't forget to call:

```c
hm_clear(&map);
```

...to release memory when you're done.

---

## 🪪 License

MIT License © 2025 [Dr. Nathan Abiira](https://github.com/abiiranathan)

---

## 🔗 Contributions

PRs and issues welcome. Please fork the repo and submit pull requests with clear descriptions and tests.

---
/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023, Nikita Romaniuk
 */

#pragma once

#include <stdbool.h>

struct hashmap_entry {
	bool exists;
	const char* key;
	void* value;
};

struct hashmap {
	struct hashmap_entry* table;
	int capacity;
	int size;
};

unsigned long djb2(const char* key);

void hashmap_create(struct hashmap* self);
void hashmap_free(struct hashmap* self);
void hashmap_resize(struct hashmap* self);
void* hashmap_get(struct hashmap* self, const char* key);
void hashmap_insert(struct hashmap* self, const char* key, void* value);

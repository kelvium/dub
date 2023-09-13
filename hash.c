/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023, Nikita Romaniuk
 */

#include "hash.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned long djb2(const char* key)
{
	unsigned long hash = 0;
	int c;
	while ((c = *key++))
		hash = ((hash << 5) + hash) + c;
	return hash;
}

void hashmap_create(struct hashmap* self)
{
	self->capacity = 2048;
	self->size = 0;
	self->table = calloc(self->capacity, sizeof(struct hashmap_entry));
	if (!self->table) {
		fprintf(stderr, "couldn't allocate hashmap\n");
		abort();
	}
}

void hashmap_free(struct hashmap* self)
{
	for (int i = 0; i < self->capacity; ++i) {
		struct hashmap_entry* entry = self->table + i;
		if (entry->exists) {
			free((void*) entry->key);
		}
	}
	free(self->table);
}

void hashmap_resize(struct hashmap* self)
{
	int new_capacity = self->capacity * 2;
	struct hashmap_entry* new_table = calloc(new_capacity, sizeof(struct hashmap_entry));
	for (int i = 0; i < self->capacity; ++i) {
		struct hashmap_entry* entry = self->table + i;
		if (entry->exists) {
			int index = djb2(entry->key) % new_capacity;
			while (new_table[index].exists)
				index = (index + 1) % new_capacity;
			new_table[index] = *entry;
		}
	}
	free(self->table);
	self->capacity = new_capacity;
	self->table = new_table;
}

void hashmap_insert(struct hashmap* self, const char* key, void* value)
{
	if (self->size == self->capacity)
		hashmap_resize(self);

	int index = djb2(key) % self->capacity;
	while (self->table[index].exists) {
		if (strcmp(self->table[index].key, key) == 0) {
			self->table[index].value = value;
			return;
		}
		index = (index + 1) % self->capacity;
	}

	struct hashmap_entry entry;
	entry.exists = true;
	entry.key = strdup(key); // it's just easier to deal with later
	entry.value = value;
	self->table[index] = entry;
	++self->size;
}

void* hashmap_get(struct hashmap* self, const char* key)
{
	int index = djb2(key) % self->capacity;
	while (self->table[index].exists) {
		if (strcmp(self->table[index].key, key) == 0)
			return self->table[index].value;
		index = (index + 1) % self->capacity;
	}
	return NULL;
}

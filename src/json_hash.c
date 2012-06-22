/*
  This file is part of YACJP.

  YacJP is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, version 3 of the License.

  YacJP is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with YacJP.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <string.h>

#include "hash.h"

/* original hashing algorithm from Python, implementation and bugs are mine */

#define PERTURB_SHIFT 5

typedef struct json_hash_entry {
     const void *key;
     void *value;
} json_hash_entry_t;

struct json_hash_impl {
     hash_t fn;
     json_memory_t memory;
     hash_keys_t   keys;

     int capacity;
     int count;
     json_hash_entry_t *entries;
};

static unsigned int string_hash(const char *key) {
     unsigned int result = 0;
     while (*key) {
          result = result * 13 + *key;
          key++;
     }
     return result;
}

__PUBLIC__ hash_keys_t hash_strings = {
     (hash_fn)string_hash,
     (compare_fn)strcmp,
     (clone_fn)strdup,
     (free_fn)free,
};

static int index_of(json_hash_entry_t *entries, hash_keys_t keys, int capacity, const void *key) {
     compare_fn cmp = keys.compare;

     unsigned int h = keys.hash(key);
     unsigned int j = h;
     unsigned int perturb = h;

     int result = j % capacity;
     const void *k = entries[result].key;
     int found = k ? !cmp(key, k) : 1;

     while (!found) {
          j = (5 * j) + 1 + perturb;
          perturb >>= PERTURB_SHIFT;
          result = j % capacity;
          k = entries[result].key;
          found = k ? !cmp(key, k) : 1;
     }

     if (!k) result = -result - 1;

     return result;
}

static void grow(struct json_hash_impl *this) {
     json_hash_entry_t *new_entries;
     int new_capacity;
     json_hash_entry_t field;
     int i, index;
     if (this->capacity == 0) {
          new_capacity = 4;
          new_entries = (json_hash_entry_t *)this->memory.malloc(new_capacity * sizeof(json_hash_entry_t));
          memset(new_entries, 0, new_capacity * sizeof(json_hash_entry_t));
     }
     else {
          new_capacity = this->capacity * 2;
          new_entries = (json_hash_entry_t *)this->memory.malloc(new_capacity * sizeof(json_hash_entry_t));
          memset(new_entries, 0, new_capacity * sizeof(json_hash_entry_t));
          for (i = 0; i < this->capacity; i++) {
               field = this->entries[i];
               if (field.key) {
                    index = -index_of(new_entries, this->keys, new_capacity, field.key) - 1;
                    new_entries[index] = field;
               }
          }
          this->memory.free(this->entries);
     }
     this->capacity = new_capacity;
     this->entries = new_entries;
}

static unsigned int count(struct json_hash_impl *this) {
     return this->count;
}

static void iterate(struct json_hash_impl *this, hash_iterator_fn iterator, void *data) {
     int i, index = 0;
     json_hash_entry_t entry;
     for (i = 0; index < this->count; i++) {
          entry = this->entries[i];
          if (entry.key) {
               iterator(this, index++, entry.key, entry.value, data);
          }
     }
}

static void *get(struct json_hash_impl *this, const void *key) {
     void *result = NULL;
     int index;
     if (this->capacity) {
          index = index_of(this->entries, this->keys, this->capacity, key);
          if (index >= 0) {
               result = this->entries[index].value;
          }
     }
     return result;
}

static void *set(struct json_hash_impl *this, const void *key, void *value) {
     void *result = NULL;
     int index;
     if (this->capacity == 0) {
          grow(this);
     }
     index = index_of(this->entries, this->keys, this->capacity, key);
     if (index >= 0) {
          result = this->entries[index].value;
     }
     else {
          if (this->count * 3 >= this->capacity * 2) {
               grow(this);
               index = index_of(this->entries, this->keys, this->capacity, key);
          }
          index = -index - 1;
          this->entries[index].key = this->keys.clone(key);
          this->count++;
     }
     this->entries[index].value = value;
     return result;
}

static void *del(struct json_hash_impl *this, const void *key) {
     void *result = NULL;
     int index = index_of(this->entries, this->keys, this->capacity, key);
     if (index >= 0) {
          result = this->entries[index].value;
          this->keys.free(this->entries[index].key);
          this->entries[index].key   = NULL;
          this->entries[index].value = NULL;
          this->count--;
     }
     return result;
}

static void free_(struct json_hash_impl *this) {
     if (this->entries) this->memory.free(this->entries);
     this->memory.free(this);
}

__PUBLIC__ hash_t *new_hash(json_memory_t memory, hash_keys_t keys) {
     struct json_hash_impl *result = (struct json_hash_impl *)memory.malloc(sizeof(struct json_hash_impl));
     if (!result) return NULL;
     result->fn.free      = (hash_free_fn   )free_  ;
     result->fn.count     = (hash_count_fn  )count  ;
     result->fn.iterate   = (hash_iterate_fn)iterate;
     result->fn.get       = (hash_get_fn    )get    ;
     result->fn.set       = (hash_set_fn    )set    ;
     result->fn.del       = (hash_del_fn    )del    ;
     result->memory       = memory;
     result->keys         = keys;
     result->capacity     = 0;
     result->count        = 0;
     result->entries       = NULL;
     return (hash_t*)result;
}

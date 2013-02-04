#ifndef __SMALL_HASH__H_
#define __SMALL_HASH__H_

#include <stddef.h>             /* offsetof */
#include <stdint.h>             /* uint32_t */
#include <stdbool.h>

typedef struct small_hash__table small_hash__table;

typedef struct small_hash__node small_hash__node;
#define SMALL_HASH__NODE__EMPTY  SMALL_HASH__NODE__EMPTY_INTERNAL

typedef struct small_hash__anchor small_hash__anchor;

typedef uint32_t small_hash__hash;

#include "utils.h"

struct small_hash__funcs {
    bool (*match_key)(void *user_arg, const void *key, small_hash__node *);
};
#define SMALL_HASH__FUNCS(prefix) \
    (struct small_hash__funcs)    \
    { & prefix ## match                 \
    }

void small_hash__table__init_static(
    small_hash__table *,
    struct small_hash__funcs *user_funcs, void *user_arg,
    unsigned anchors_count, small_hash__anchor *anchors);
#define SMALL_HASH__TABLE__INIT_STATIC(table, user_funcs, user_arg, anchors_array) \
    small_hash__table__init_static((table), (user_funcs), (user_arg),   \
                                   ARRAY_LEN(anchors_array), (anchors_array))

void small_hash__table__init_dynamic(
    small_hash__table *,
    struct small_hash__funcs *user_funcs, void *user_arg,
    unsigned anchors_count);

void small_hash__table__free(small_hash__table *);

void small_hash__table__add(
    small_hash__table *,
    small_hash__hash, small_hash__node *);
void small_hash__table__del(
    small_hash__table *,
    small_hash__hash, small_hash__node *);
small_hash__node *small_hash__table__find(
    small_hash__table *,
    small_hash__hash, const void *key);

#include "small_hash_internals.h"

#endif

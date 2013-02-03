#include "small_hash.h"
#include <stdio.h>              /* printf */
#include <stdlib.h>             /* malloc, abort */

#define PAIR_COUNT (10000000)
#define HASH_SIZE (1ULL<<16)

struct pair {
    small_hash__node node;
    unsigned key, val;
};

#define pair_of(nodeptr) container_of(nodeptr, struct pair, node)

static bool prefix__match(void *user_arg, const void *key, small_hash__node *node) {
    return pair_of(node)->key == (uintptr_t)key;
}

int main() {
    small_hash__anchor *anchors = malloc(HASH_SIZE * sizeof *anchors);
    small_hash__table table;
    struct small_hash__funcs funcs = SMALL_HASH__FUNCS(prefix__);
    SMALL_HASH__TABLE__INIT_STATIC(&table, &funcs, NULL, anchors);

    struct pair *pairs = malloc(PAIR_COUNT * sizeof *pairs);
    unsigned i;
    for(i = 0; i < PAIR_COUNT; i++) {
        pairs[i] = (struct pair){ SMALL_HASH__NODE__EMPTY, i, i };
        small_hash__table__add(&table, i, &pairs[i].node);
    }

    struct pair *res = pair_of(small_hash__table__find(&table, 100, (void*)(uintptr_t)100));

    printf("%d", res->val);
    return 0;
}

#include "small_hash.h"
#include <stdio.h>              /* printf */
#include <stdlib.h>             /* malloc, abort */

#define HASH_SIZE (1ULL<<16)

struct pair {
    small_hash__node node;
    unsigned key, val;
};

#define pair_of(nodeptr) container_of(nodeptr, struct pair, node)

static bool prefix__match(void *user_arg, const void *key, small_hash__node *node) {
    return pair_of(node)->key == (uintptr_t)key;
}

static void usage(char *progname) { fprintf(stderr, "Usage: %s <count>\n", progname); }

int main(int argc, char *argv[]) {
    if(argc != 2) { usage(argv[0]); return -1; }
    long pair_count = atol(argv[1]);
    if(pair_count <= 0) { usage(argv[0]); return -1; }

    small_hash__table table;
    struct small_hash__funcs funcs = SMALL_HASH__FUNCS(prefix__);
    small_hash__table__init_dynamic(&table, &funcs, NULL, HASH_SIZE);

    struct pair *pairs = malloc(pair_count * sizeof *pairs);
    unsigned i;
    for(i = 0; i < pair_count; i++) {
        pairs[i] = (struct pair){ SMALL_HASH__NODE__EMPTY, i, i };
        small_hash__table__add(&table, i, &pairs[i].node);
    }

    struct pair *res = pair_of(small_hash__table__find(&table, 100, (void*)(uintptr_t)100));

    printf("%d\n", res->val);
    return 0;
}

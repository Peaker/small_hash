#include "small_hash.h"
#include "get_time_micros.h"
#include <stdio.h>              /* printf */
#include <stdlib.h>             /* malloc, abort */
#include <inttypes.h>           /* PRI* */

#define HASH_SIZE (1ULL<<16)

struct pair {
    small_hash__node node;
    unsigned key, val;
};

#define pair_of(nodeptr) container_of(nodeptr, struct pair, node)

static bool prefix__match(void *user_arg, const void *key, small_hash__node *node) {
    return pair_of(node)->key == (uintptr_t)key;
}

static small_hash__hash prefix__get_hash(void *user_arg, small_hash__node *node) {
    return pair_of(node)->key;
}

static void usage(char *progname) { fprintf(stderr, "Usage: %s <count>\n", progname); }

static void destroy(void *arg, small_hash__node *node) {}

int main(int argc, char *argv[]) {
    if(argc != 2) { usage(argv[0]); return -1; }
    long pair_count = atol(argv[1]);
    if(pair_count <= 0) { usage(argv[0]); return -1; }

    small_hash__table table;
    struct small_hash__funcs funcs = SMALL_HASH__FUNCS(prefix__);
    small_hash__table__init_dynamic(&table, &funcs, NULL, 128);

    uint64_t t0 = get_time_micros();

    struct pair *pairs = malloc(pair_count * sizeof *pairs);
    unsigned i;
    for(i = 0; i < pair_count; i++) {
        pairs[i] = (struct pair){ SMALL_HASH__NODE__EMPTY, i, i };
        small_hash__table__add(&table, i, &pairs[i].node);
    }

    uint64_t t1 = get_time_micros();

    for(i = 0; i < pair_count; i++) {
        unsigned randnum = i*0x4A31 % 0x80000;
        struct pair *res = pair_of(
            small_hash__table__find(
                &table, randnum, (void*)(uintptr_t)randnum));
        if(res && res->val == -1U) printf("%d\n", res->val);
    }

    uint64_t t2 = get_time_micros();

    for(i = 0; i < pair_count; i++) {
        small_hash__table__del(&table, i, &pairs[i].node);
    }
    small_hash__table__fini_destroy(&table, destroy, NULL);

    uint64_t t3 = get_time_micros();
    printf("insertions: %" PRIu64 "\n"
           "lookups   : %" PRIu64 "\n"
           "deletions : %" PRIu64 "\n"
           "total     : %" PRIu64 "\n",
           t1-t0, t2-t1, t3-t2, t3-t0);

    return 0;
}

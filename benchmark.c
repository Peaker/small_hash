#include "small_hash.h"
#include <stdio.h>              /* printf */
#include <stdlib.h>             /* malloc, abort */
#include <sys/time.h>           /* gettimeofday */
#include <inttypes.h>           /* PRI* */

#define PAIR_COUNT (10000000)
#define HASH_SIZE (1ULL<<16)

struct pair {
    small_hash__node node;
    unsigned key, val;
};

#define pair_of(nodeptr)                           \
    container_of(nodeptr, struct pair, node)

static bool prefix__match(void *user_arg, const void *key, small_hash__node *node) {
    return pair_of(node)->key == (uintptr_t)key;
}

static uint64_t micros() {
    struct timeval tv;
    int rc = gettimeofday(&tv, NULL);
    if(0 != rc) {
        perror("gettimeofday");
        abort();
    }
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

int main() {
    uint64_t t0 = micros();

    small_hash__anchor *anchors = malloc(HASH_SIZE * sizeof *anchors);
    small_hash__table table;
    struct small_hash__funcs funcs = SMALL_HASH__FUNCS(prefix__);
    SMALL_HASH__TABLE__INIT_STATIC(&table, &funcs, NULL, anchors);

    uint64_t t1 = micros();

    struct pair *pairs = malloc(PAIR_COUNT * sizeof *pairs);
    unsigned i;
    for(i = 0; i < PAIR_COUNT; i++) {
        pairs[i] = (struct pair){ SMALL_HASH__NODE__EMPTY, i, i };
        small_hash__table__add(&table, i, &pairs[i].node);
    }
    uint64_t t2 = micros();

    struct pair *res = pair_of(small_hash__table__find(&table, 100, (void*)(uintptr_t)100));

    uint64_t t3 = micros();
    printf("%d, %" PRIu64", %" PRIu64 ", %" PRIu64 "\n",
           res->val, t1-t0, t2-t1, t3-t2);
    return 0;
}

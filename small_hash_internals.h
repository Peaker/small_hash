#ifndef __SMALL_HASH__INTERNALS_H_
#define __SMALL_HASH__INTERNALS_H_

struct small_hash__table {
    struct small_hash__funcs *user_funcs;
    void *user_arg;

    bool is_dynamic;
    unsigned anchors_count;
    small_hash__anchor *anchors;
};

struct small_hash__node {
    small_hash__node *prev, *next;
};

#define SMALL_HASH__NODE__EMPTY_INTERNAL   (struct small_hash__node){ NULL, NULL }


struct small_hash__anchor {
    small_hash__node *first;
};

#endif

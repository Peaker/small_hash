#ifndef __SMALL_HASH__INTERNALS_H_
#define __SMALL_HASH__INTERNALS_H_

struct small_hash__table {
    struct small_hash__funcs *user_funcs;
    void *user_arg;

    unsigned count;

    bool is_dynamic;
    /* If dynamic: */
    unsigned min_anchors_count;
    unsigned low_watermark;
    unsigned high_watermark;
    unsigned expensive_lookup_count;
    unsigned lookup_count;

    unsigned anchors_count;
    small_hash__anchor *anchors;

    unsigned prevent_resizes_count;
};

struct small_hash__node {
    small_hash__node *prev, *next;
};

struct small_hash__iter {
    unsigned next_anchor_index;
    small_hash__node *next;
};

#define SMALL_HASH__NODE__EMPTY_INTERNAL   (struct small_hash__node){ NULL, NULL }


struct small_hash__anchor {
    small_hash__node *first;
};

#endif

#include "small_hash.h"
#include "utils.h"
#include <string.h>             /* memset */
#include <stdlib.h>             /* free */
#include <assert.h>

/* Having 1 anchor for each element (on average) is typically not a
 * big deal, it is equivalent to enlarging the elements by a single
 * ptr. We already pay 2 ptrs per element, so it adds 33% to the
 * overhead we add. */
#define DESIRED_COUNT_PER_ANCHOR 1

/* If we grow/shrink to having 5 or 0.20 items per anchor, we rehash. */
#define FACTOR  6

static void init_internal(
    small_hash__table *table,
    struct small_hash__funcs *user_funcs, void *user_arg,
    unsigned anchors_count,
    small_hash__anchor anchors[],
    bool is_dynamic)
{
    table->user_funcs = user_funcs;
    table->user_arg = user_arg;
    table->is_dynamic = is_dynamic;
    table->anchors_count = anchors_count;
    table->anchors = anchors;
    table->count = 0;
}

void small_hash__table__init_static(
    small_hash__table *table,
    struct small_hash__funcs *user_funcs, void *user_arg,
    unsigned anchors_count,
    small_hash__anchor anchors[])
{
    memset(anchors, 0, anchors_count * sizeof *anchors);
    init_internal(table, user_funcs, user_arg, anchors_count, anchors, false);
}

static void set_watermarks(small_hash__table *table)
{
    unsigned desired_count = table->anchors_count * DESIRED_COUNT_PER_ANCHOR;
    table->low_watermark = desired_count / FACTOR;
    table->high_watermark = desired_count * FACTOR;
}

static small_hash__anchor *alloc_anchors(unsigned count)
{
    small_hash__anchor *anchors = malloc(count * sizeof *anchors);
    memset(anchors, 0, count * sizeof *anchors);
    return anchors;
}

void small_hash__table__init_dynamic(
    small_hash__table *table,
    struct small_hash__funcs *user_funcs, void *user_arg,
    unsigned min_anchors_count)
{
    init_internal(table, user_funcs, user_arg,
                  min_anchors_count, alloc_anchors(min_anchors_count), true);
    table->min_anchors_count = min_anchors_count;
    set_watermarks(table);
}

void small_hash__table__free(small_hash__table *table)
{
    if(table->is_dynamic) {
        free(table->anchors);
        table->anchors = NULL;
    }
}

static small_hash__anchor *anchor_of_hash(small_hash__table *table, small_hash__hash hash)
{
    return &table->anchors[hash % table->anchors_count];
}

static void insert_to_anchor(small_hash__anchor *anchor, small_hash__node *node)
{
    node->prev = NULL;
    node->next = anchor->first;
    if(node->next) {
        node->next->prev = node;
    }
    anchor->first = node;
}

static void rehash(small_hash__table *table, unsigned new_anchors_count)
{
    assert(table->is_dynamic);
    small_hash__anchor *new_anchors = alloc_anchors(new_anchors_count);
    unsigned i;
    for(i = 0; i < table->anchors_count; i++) {
        small_hash__node *node, *next;
        for(node = table->anchors[i].first; node; node=next) {
            next = node->next;
            small_hash__hash hash =
                table->user_funcs->get_hash(table->user_arg, node);
            small_hash__anchor *anchor = &new_anchors[hash % new_anchors_count];
            insert_to_anchor(anchor, node);
        }
    }
    free(table->anchors);
    table->anchors = new_anchors;
    table->anchors_count = new_anchors_count;
    set_watermarks(table);
}

static void maybe_enlarge(small_hash__table *table)
{
    if(!table->is_dynamic) return;
    if(table->count < table->high_watermark) return;
    rehash(table, table->anchors_count * FACTOR);
}

static void maybe_shrink(small_hash__table *table)
{
    if(!table->is_dynamic) return;
    if(table->count >= table->low_watermark) return;
    if(table->anchors_count <= table->min_anchors_count) return;
    rehash(table, max(table->anchors_count / FACTOR, table->min_anchors_count));
}

void small_hash__table__add(
    small_hash__table *table,
    small_hash__hash hash, small_hash__node *node)
{
    insert_to_anchor(anchor_of_hash(table, hash), node);
    table->count++;
    maybe_enlarge(table);
}

void small_hash__table__del(
    small_hash__table *table,
    small_hash__hash hash, small_hash__node *node)
{
    /* Remove link from prev to us: */
    if(node->prev) {
        node->prev->next = node->next;
    } else {
        /* anchor is prev */
        small_hash__anchor *anchor = anchor_of_hash(table, hash);
        anchor->first = node->next;
    }
    /* Remove link from next to us: */
    if(node->next) {
        node->next->prev = node->prev;
    }

    table->count--;
    maybe_shrink(table);
}

small_hash__node *small_hash__table__find(
    small_hash__table *table,
    small_hash__hash hash, const void *key)
{
    small_hash__anchor *anchor = anchor_of_hash(table, hash);
    small_hash__node *node;
    for(node = anchor->first; node; node = node->next) {
        if(table->user_funcs->match_key(table->user_arg, key, node)) {
            return node;
        }
    }
    return NULL;
}

#include "small_hash.h"
#include <string.h>             /* memset */
#include <stdlib.h>             /* free */

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
    memset(anchors, 0, anchors_count * sizeof(small_hash__anchor));
}

void small_hash__table__init_static(
    small_hash__table *table,
    struct small_hash__funcs *user_funcs, void *user_arg,
    unsigned anchors_count,
    small_hash__anchor anchors[])
{
    init_internal(table, user_funcs, user_arg, anchors_count, anchors, false);
}

void small_hash__table__init_dynamic(
    small_hash__table *table,
    struct small_hash__funcs *user_funcs, void *user_arg,
    unsigned anchors_count)
{
    small_hash__anchor *anchors =
        malloc(anchors_count * sizeof *anchors);
    init_internal(table, user_funcs, user_arg, anchors_count, anchors, true);
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

void small_hash__table__add(
    small_hash__table *table,
    small_hash__hash hash, small_hash__node *node)
{
    small_hash__anchor *anchor = anchor_of_hash(table, hash);
    node->prev = NULL;
    node->next = anchor->first;
    if(node->next) {
        node->next->prev = node;
    }
    anchor->first = node;
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

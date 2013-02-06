#include "small_hash.h"
#include "utils.h"
#include <string.h>             /* memset */
#include <stdlib.h>             /* free */
#include <assert.h>
#include "get_time_micros.h"

/* Having 1 anchor for each element (on average) is typically not a
 * big deal, it is equivalent to enlarging the elements by a single
 * ptr. We already pay 2 ptrs per element, so it adds 33% to the
 * overhead we add. */
#define DESIRED_COUNT_PER_ANCHOR 5

#define SHRINK_FACTOR            4
#define SHRINK_WATERMARK_FACTOR  4

/* Allow expanding the hash table even if it only grew twice too
 * large. The actual trigger is many expensive lookups. */
#define MIN_EXPAND_WATERMARK_FACTOR  2

#define EXPENSIVE_LOOKUP_THRESHOLD   7
#define ENLARGE_DUE_TO_EXPENSIVE_LOOKUP_AFTER  2
#define BETWEEN_LOOKUP_REPORT_COUNT  10

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
    table->expensive_lookup_count = 0;
    table->lookup_count = 0;
    table->total_rehash_cost = 0;
}

void small_hash__table__init_static(
    small_hash__table *table,
    struct small_hash__funcs *user_funcs, void *user_arg,
    unsigned anchors_count,
    small_hash__anchor anchors[])
{
    memset(anchors, 0, anchors_count * sizeof *anchors);
    init_internal(table, user_funcs, user_arg, anchors_count, anchors, false);
    table->low_watermark = 0;
    table->high_watermark = 0;
    table->expensive_lookup_count = 0;
}

static void set_watermarks(small_hash__table *table)
{
    unsigned desired_count = table->anchors_count * DESIRED_COUNT_PER_ANCHOR;
    table->low_watermark = desired_count / SHRINK_WATERMARK_FACTOR;
    table->high_watermark = desired_count * MIN_EXPAND_WATERMARK_FACTOR;
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
    uint64_t before = get_time_micros();
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
    uint64_t cost = get_time_micros() - before;
    table->total_rehash_cost += cost;
}

static void maybe_shrink(small_hash__table *table)
{
    if(!table->is_dynamic) return;
    if(table->count >= table->low_watermark) return;
    if(table->anchors_count <= table->min_anchors_count) return;
    rehash(table, max(table->anchors_count / SHRINK_FACTOR, table->min_anchors_count));
}

void small_hash__table__add(
    small_hash__table *table,
    small_hash__hash hash, small_hash__node *node)
{
    insert_to_anchor(anchor_of_hash(table, hash), node);
    table->count++;
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

static inline void report_lookup(small_hash__table *table, unsigned lookup_cost)
{
    if(!table->is_dynamic) return;
    if(table->count <= table->high_watermark) return;
    if(lookup_cost < EXPENSIVE_LOOKUP_THRESHOLD) return;

    table->expensive_lookup_count++;
    if(table->expensive_lookup_count < ENLARGE_DUE_TO_EXPENSIVE_LOOKUP_AFTER) return;

    unsigned new_anchors_count = table->count / DESIRED_COUNT_PER_ANCHOR;
    assert(new_anchors_count > table->min_anchors_count);
    rehash(table, new_anchors_count);
}

static inline small_hash__node *find_and_report(
    small_hash__table *table,
    small_hash__hash hash, const void *key)
{
    small_hash__anchor *anchor = anchor_of_hash(table, hash);
    small_hash__node *node;
    unsigned lookup_cost = 0;
    for(node = anchor->first; node; node = node->next) {
        lookup_cost++;
        if(table->user_funcs->match_key(table->user_arg, key, node)) {
            break;
        }
    }
    report_lookup(table, lookup_cost);
    return node;
}

static inline small_hash__node *find(
    small_hash__table *table, small_hash__hash hash, const void *key)
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

small_hash__node *small_hash__table__find(
    small_hash__table *table,
    small_hash__hash hash, const void *key)
{
    if(table->is_dynamic && ++table->lookup_count >= BETWEEN_LOOKUP_REPORT_COUNT) {
        table->lookup_count = 0;
        return find_and_report(table, hash, key);
    }
    return find(table, hash, key);
}

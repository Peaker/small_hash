#include "small_hash.h"

#include <string.h>
#include <assert.h>

struct user {
    const char *name;
    unsigned age;
    struct small_hash__node by_name_node;
    struct small_hash__node by_age_node;
};

struct users {
    small_hash__table by_name_table;
    small_hash__anchor by_name_anchors[2];
    small_hash__table by_age_table;
    small_hash__anchor by_age_anchors[2];
};

/* By Name: */
static bool by_name__match(void *user_arg, const void *key, small_hash__node *node)
{
    const char *key_name = key;
    struct user *user_node = container_of(node, struct user, by_name_node);
    return 0 == strcmp(user_node->name, key_name);
}

struct small_hash__funcs by_name_funcs = SMALL_HASH__FUNCS(by_name__);

static small_hash__hash by_name__hash(const char *name)
{
    small_hash__hash result = 0x12345;
    const char *p;
    for(p = name; *p; p++) {
        result *= 33;
        result += *p;
    }
    return result;
}

/* By Age: */
static bool by_age__match(void *user_arg, const void *key, small_hash__node *node)
{
    unsigned age = (uintptr_t)key;
    struct user *user_node = container_of(node, struct user, by_age_node);
    return user_node->age == age;
}

struct small_hash__funcs by_age_funcs = SMALL_HASH__FUNCS(by_age__);

static small_hash__hash by_age__hash(unsigned age)
{
    return age;
}

static void users__init(struct users *users)
{
    SMALL_HASH__TABLE__INIT_STATIC(
        &users->by_name_table,
        &by_name_funcs, NULL, users->by_name_anchors);
    SMALL_HASH__TABLE__INIT_STATIC(
        &users->by_age_table,
        &by_age_funcs, NULL, users->by_age_anchors);
}

static void users__add(struct users *users, struct user *u)
{
    small_hash__table__add(&users->by_name_table, by_name__hash(u->name), &u->by_name_node);
    small_hash__table__add(&users->by_age_table, by_age__hash(u->age), &u->by_age_node);
}

static void users__del(struct users *users, struct user *u)
{
    small_hash__table__del(&users->by_name_table, by_name__hash(u->name), &u->by_name_node);
    small_hash__table__del(&users->by_age_table, by_age__hash(u->age), &u->by_age_node);
}

static struct user *users__find_by_name(struct users *users, const char *name)
{
    small_hash__node *node =
        small_hash__table__find(&users->by_name_table, by_name__hash(name), name);
    if(!node) return NULL;
    return container_of(node, struct user, by_name_node);
}

static struct user *users__find_by_age(struct users *users, unsigned age)
{
    small_hash__node *node =
        small_hash__table__find(&users->by_age_table, by_age__hash(age), (void *)(uintptr_t)age);
    if(!node) return NULL;
    return container_of(node, struct user, by_age_node);
}

int main() {
    struct users users;
    users__init(&users);
    struct user moshe = { .name = "Moshe", .age = 20 };
    struct user dani  = { .name = "Dani" , .age = 30 };
    struct user yossi = { .name = "Yossi", .age = 40 };
    users__add(&users, &moshe);
    users__add(&users, &dani);
    users__add(&users, &yossi);
    users__del(&users, &dani);
    assert(&moshe == users__find_by_name(&users, "Moshe"));
    assert(NULL   == users__find_by_name(&users, "Moshiko"));
    assert(NULL   == users__find_by_name(&users, "Dani"));
    assert(&yossi == users__find_by_name(&users, "Yossi"));

    assert(&moshe == users__find_by_age(&users, 20));
    assert(NULL   == users__find_by_age(&users, 25));
    assert(NULL   == users__find_by_age(&users, 30));
    assert(&yossi == users__find_by_age(&users, 40));
    return 0;
}

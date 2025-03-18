#include "monitor/watchpoint.h"
#include "monitor/expr.h"
#include <stdlib.h>

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool()
{
    int i;
    for (i = 0; i < NR_WP; i++)
    {
        wp_pool[i].NO = i;
        wp_pool[i].next = &wp_pool[i + 1];
    }
    wp_pool[NR_WP - 1].next = NULL;

    head = NULL;
    free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

WP *new_wp()
{
    Assert(free_, "WP pool is full");
    WP *temp = free_;
    free_ = free_->next;
    temp->next = head;
    head = temp;
    return temp;
}

void free_wp(WP *wp)
{
    WP **cur;
    for (cur = &head; *cur; cur = &(*cur)->next)
    {
        if (*cur == wp)
        {
            *cur = wp->next;
            wp->next = free_;
            free(wp->expr);
            free_ = wp;
            break;
        }
    }
}

bool wp_has_changed()
{
    WP *p;
    uint32_t new_val;
    bool success;
    bool has_changed = false;

    for (p = head; p != NULL; p = p->next)
    {
        new_val = expr(p->expr, &success);
        if (!success) {
            printf("Error: Expression '%s' is invalid\n", p->expr);
            continue;
        }

        if (new_val != p->old_value)
        {
            printf("\nWatchpoint %d: %s\n", p->NO, p->expr);
            printf("  Old value: %u\n", p->old_value);
            printf("  New value: %u\n\n", new_val);

            p->old_value = new_val;
            has_changed = true;
        }
    }

    return has_changed;
}

WP *find_wp(int n)
{
    WP *p;
    for (p = head; p; p = p->next)
        if (p->NO == n)
            return p;
    return NULL;
}

void print_wp()
{
    WP *p;
    bool has_entries = false;

    printf("Num\tExpression\t\tValue\n");
    printf("----\t-----------\t\t------\n");

    for (p = head; p != NULL; p = p->next) {
        has_entries = true;

        char expr_str[20];
        snprintf(expr_str, sizeof(expr_str), "%.18s", p->expr);

        printf("%-4d\t%-18s\t%8u\n", p->NO, expr_str, p->old_value);
    }

    if (!has_entries) {
        printf("No watchpoints defined.\n");
    }
}
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "tlb.h"
#include "mlpt.h"
#include "config.h"


#define NUM_SETS 16
#define ASSOCIATIVITY 4
#define PAGE_MASK (~((1 << POBITS) - 1))

typedef struct {
    int valid;
    size_t vpn;
    size_t ppn;
    int lru;
} tlb_entry_t;

typedef struct {
    tlb_entry_t entries[ASSOCIATIVITY];
} tlb_set_t;

static tlb_set_t tlb[NUM_SETS];

static inline size_t get_set_index(size_t vpn) {
    return (vpn >> POBITS) & (NUM_SETS - 1);
}

void tlb_clear(void) {
    for (int i = 0; i < NUM_SETS; i++) {
        for (int j = 0; j < ASSOCIATIVITY; j++) {
            tlb[i].entries[j].valid = 0;
            tlb[i].entries[j].lru = 0;
        }
    }
}

static void update_lru(tlb_set_t *set, int accessed_index) {
    for (int i = 0; i < ASSOCIATIVITY; i++) {
        if (set->entries[i].valid) {
            if (i == accessed_index) {
                set->entries[i].lru = 1;
            }
            else if (set->entries[i].lru < ASSOCIATIVITY) {
                set->entries[i].lru++;
            }
        }
    }
}

size_t tlb_translate(size_t va) {
    size_t vpn = va & PAGE_MASK;
    size_t set_index = get_set_index(vpn);
    tlb_set_t *set = &tlb[set_index];
    size_t offset = va & ((1 << POBITS) - 1);

    for (int i = 0; i < ASSOCIATIVITY; i++) {
        if (set->entries[i].valid && set->entries[i].vpn == vpn) {
            update_lru(set, i);
            return (set->entries[i].ppn << POBITS) | offset;
        }
    }

    size_t pa = translate(vpn);
    if (pa == (size_t)-1) {
        return -1;
    }

    int lru_index = 0;
    int max_lru = 0;

    for (int i = 0; i < ASSOCIATIVITY; i++) {
        if (!set->entries[i].valid) {
            lru_index = i;
            break;
        }
        if (set->entries[i].lru > max_lru) {
            max_lru = set->entries[i].lru;
            lru_index = i;
        }
    }

    set->entries[lru_index].valid = 1;
    set->entries[lru_index].vpn = vpn;
    set->entries[lru_index].ppn = pa >> POBITS;

    update_lru(set, lru_index);

    return (pa & PAGE_MASK) | offset;
}

int tlb_peek(size_t va) {
    size_t vpn = va & PAGE_MASK;
    size_t set_index = get_set_index(vpn);
    tlb_set_t *set = &tlb[set_index];

    for (int i = 0; i < ASSOCIATIVITY; i++) {
        if (set->entries[i].valid && set->entries[i].vpn == vpn) {
            return set->entries[i].lru;
        }
    }
    return 0;
}

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "tlb.h"
#include "mlpt.h"
#include "config.h"


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

size_t tlb_translate(size_t va) {
    size_t vpn = va >> POBITS;
    size_t set_index = get_set_index(vpn);
    tlb_set_t *set = &tlb[set_index];

    for (int i = 0; i < ASSOCIATIVITY; i++) {
        if (set->entries[i].valid && set->entries[i].vpn == vpn) {
            size_t offset = va & ((1 << POBITS) - 1);
            return (set->entries[i].ppn << POBITS) | offset;
        }
    }

    size_t pa = translate(vpn << POBITS);

    if (pa == -1) {
        return -1;
    }
    return pa | (va & ((1 << POBITS) - 1));
}

int tlb_peek(size_t va) {
    size_t vpn = va >> POBITS;
    size_t set_index = get_set_index(vpn);
    tlb_set_t *set = &tlb[set_index];

    for (int i = 0; i < ASSOCIATIVITY; i++) {
        if (set->entries[i].valid && set->entries[i].vpn == vpn) {
            return 1;
        }
    }
    return 0;
}

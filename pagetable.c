#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "config.h"
#include "mlpt.h"


size_t ptbr = 0;

size_t* allocate_page_table() {
    size_t *page;

    if (posix_memalign((void**)&page, (1 << POBITS), (1 << POBITS)) != 0) {
        perror("posix_memalign failed");
        return NULL;
    }

    memset(page, 0, (1 << POBITS));
    return page;
}

void page_allocate(size_t va) {
    size_t *table;
    size_t index;

    if (ptbr == 0) {
        ptbr = (size_t)allocate_page_table();
        if (ptbr == 0) {
            return;
        }
    }

    table = (size_t *) ptbr;

    for (int level = 0; level < LEVELS - 1; level++) {
        index = (va >> (POBITS + (LEVELS - 1 - level) * (POBITS - 3))) & ((1 << (POBITS - 3)) - 1);
        if ((table[index] & 1) == 0) {
            table[index] = (size_t)allocate_page_table() | 1;
            if (table[index] == 0) {
                return;
            }
        }
        table = (size_t *)(table[index] & ~1);
    }

    index = (va >> POBITS) & ((1 << (POBITS - 3)) - 1);

    if ((table[index] & 1) == 0) {
        table[index] = (size_t)allocate_page_table() | 1;
    }
}

int page_deallocate(size_t va) {
    if (ptbr == 0) {
        return -1;
    }

    size_t *table = (size_t *)ptbr;
    size_t index;

    for (int level = 0; level < LEVELS - 1; level++) {
        index = (va >> (POBITS + ((LEVELS - 1 - level) * (POBITS - 3)))) & ((1 << (POBITS - 3)) - 1);

        if ((table[index] & 1) == 0) {
            return -1;
        }

        table = (size_t *)(table[index] & ~1);
    }

    index = (va >> POBITS) & ((1 << (POBITS - 3)) - 1);

    if ((table[index] & 1) == 0) {
        return -1;
    }

    free((void *)(table[index] & ~1));
    table[index] = 0;

    return 0;
}

size_t translate(size_t va) {
    if (ptbr == 0) {
        return ~0;
    }

    size_t *table = (size_t *) ptbr;
    size_t index;

    for (int level = 0; level < LEVELS - 1; level++) {
        index = (va >> (POBITS + ((LEVELS - 1 - level) * (POBITS - 3)))) & ((1 << (POBITS - 3)) - 1);

        if ((table[index] & 1) == 0) {
            return ~0;
        }

        table = (size_t *)(table[index] & ~1);
    }

    index = (va >> POBITS) & ((1 << (POBITS - 3)) - 1);
    if ((table[index] & 1) == 0) {
        return ~0;
    }

    size_t ppn = table[index] >> POBITS;
    size_t offset = va & ((1 << POBITS) - 1);

    return (ppn << POBITS) | offset;
}

#define LEXEL_IMPLEMENTATION
#include "../lexel.h"

#include <stdio.h>

int main(void) {
    printf("abc == abc: %d (expected: 1)\n",
           lxl_sv_equal(LXL_SV_FROM_STRLIT("abc"), LXL_SV_FROM_STRLIT("abc")));
    printf("abc == cba: %d (expected: 0)\n",
           lxl_sv_equal(LXL_SV_FROM_STRLIT("abc"), LXL_SV_FROM_STRLIT("cba")));
    printf("ab <=> abc: %d (expected: -1)\n",
           lxl_sv_compare(LXL_SV_FROM_STRLIT("ab"), LXL_SV_FROM_STRLIT("abc")));
    printf("ab <=> a: %d (expected: 1)\n",
           lxl_sv_compare(LXL_SV_FROM_STRLIT("ab"), LXL_SV_FROM_STRLIT("a")));
    printf("abc <=> abc: %d (expected: 0)\n",
           lxl_sv_compare(LXL_SV_FROM_STRLIT("abc"), LXL_SV_FROM_STRLIT("abc")));
    printf("ab <=> b: %d (expected: -1)\n",
           lxl_sv_compare(LXL_SV_FROM_STRLIT("ab"), LXL_SV_FROM_STRLIT("b")));
    printf("a <=> b: %d (expected: -1)\n",
           lxl_sv_compare(LXL_SV_FROM_STRLIT("a"), LXL_SV_FROM_STRLIT("b")));
}

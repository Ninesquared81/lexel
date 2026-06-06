#include "../lexel.c"

#include <stdio.h>

int main(void) {
    printf("abc == abc: %d (expected: 1)\n",
           lxl_sv_eq(LXL_SV_FROM_STRLIT("abc"), LXL_SV_FROM_STRLIT("abc")));
    printf("abc == cba: %d (expected: 0)\n",
           lxl_sv_eq(LXL_SV_FROM_STRLIT("abc"), LXL_SV_FROM_STRLIT("cba")));
    printf("abc has prefix ab: %d (expected: 1)\n",
           lxl_sv_has_prefix(LXL_SV_FROM_STRLIT("abc"), LXL_SV_FROM_STRLIT("ab")));
    printf("abc has suffix ab: %d (expected: 0)\n",
           lxl_sv_has_suffix(LXL_SV_FROM_STRLIT("abc"), LXL_SV_FROM_STRLIT("ab")));
    printf("abc has prefix bc: %d (expected: 0)\n",
           lxl_sv_has_prefix(LXL_SV_FROM_STRLIT("abc"), LXL_SV_FROM_STRLIT("bc")));
    printf("abc has suffix bc: %d (expected: 1)\n",
           lxl_sv_has_suffix(LXL_SV_FROM_STRLIT("abc"), LXL_SV_FROM_STRLIT("bc")));
    // printf("ab <=> abc: %d (expected: -1)\n",
    //        lxl_sv_compare(LXL_SV_FROM_STRLIT("ab"), LXL_SV_FROM_STRLIT("abc")));
    // printf("ab <=> a: %d (expected: 1)\n",
    //        lxl_sv_compare(LXL_SV_FROM_STRLIT("ab"), LXL_SV_FROM_STRLIT("a")));
    // printf("abc <=> abc: %d (expected: 0)\n",
    //        lxl_sv_compare(LXL_SV_FROM_STRLIT("abc"), LXL_SV_FROM_STRLIT("abc")));
    // printf("ab <=> b: %d (expected: -1)\n",
    //        lxl_sv_compare(LXL_SV_FROM_STRLIT("ab"), LXL_SV_FROM_STRLIT("b")));
    // printf("a <=> b: %d (expected: -1)\n",
    //        lxl_sv_compare(LXL_SV_FROM_STRLIT("a"), LXL_SV_FROM_STRLIT("b")));

}

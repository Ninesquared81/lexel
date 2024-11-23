#include <stdio.h>

#define LEXEL_IMPLEMENTATION
#include "lexel.h"

int main(void) {
    printf("Hello, World!\n");
    struct lxl_lexer lexer = lxl_lexer_from_sv(LXL_SV_FROM_STRLIT("1 2 +"));
    for (int i = 0; !lxl_lexer__is_at_end(&lexer); ++i) {
        printf("Character %d: '%c'\n", i, lxl_lexer__advance(&lexer));
    }
    return 0;
}

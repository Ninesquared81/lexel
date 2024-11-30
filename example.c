#include <stdio.h>

#define LEXEL_IMPLEMENTATION
#include "lexel.h"

int main(void) {
    printf("Hello, World!\n");
    struct lxl_lexer lexer = lxl_lexer_from_sv(LXL_SV_FROM_STRLIT("#hi\n  1 2 +  # 3 4"));
    for (int i = 0; !lxl_lexer__is_at_end(&lexer); ++i) {
        printf("Character %d: '%c'\n", i, lxl_lexer__advance(&lexer));
    }
    printf("Restting lexer...\n");
    lxl_lexer_reset(&lexer);
    for (int i = 0; !lxl_lexer__is_at_end(&lexer); ++i) {
        i += lxl_lexer__skip_whitespace(&lexer);
        printf("Character %d: '%c'\n", i, lxl_lexer__advance(&lexer));
    }
    lxl_lexer_reset(&lexer);
    lexer.line_comment_openers = (const char*[]){"#", NULL};
    for (int i = 0; !lxl_lexer_is_finished(&lexer); ++i) {
        printf("lxl_lexer__check_line_comment('%c'): %d\n",
               *lexer.current,
               lxl_lexer__check_line_comment(&lexer));
        struct lxl_token token = lxl_lexer_next_token(&lexer);
        printf("Token %d: '%.*s' [type = %d]\n", i,
               (int)(token.end - token.start), token.start,
               token.token_type);
    }
    return 0;
}

#include <stdio.h>

#define LEXEL_IMPLEMENTATION
#include "lexel.h"

int main(void) {
    printf("Hello, World!\n");
    struct lxl_lexer lexer = lxl_lexer_from_sv(
        LXL_SV_FROM_STRLIT("#hi\n  1029 22 +  31 3548 /* hi*/\n\"Hello, World!\\n\""));
    /* for (int i = 0; !lxl_lexer__is_at_end(&lexer); ++i) { */
    /*     printf("Character %d: '%c'\n", i, lxl_lexer__advance(&lexer)); */
    /* } */
    /* printf("Restting lexer...\n"); */
    /* lxl_lexer_reset(&lexer); */
    for (int i = 0; !lxl_lexer__is_at_end(&lexer); ++i) {
        i += lxl_lexer__skip_whitespace(&lexer);
        int base = 10;
        if (lxl_lexer__check_digit(&lexer, base)) {
            const char *start = lexer.current;
            lxl_lexer__lex_integer(&lexer, base);
            struct lxl_string_view sv = lxl_sv_from_startend(start, lexer.current);
            printf("Integer literal: '"LXL_SV_FMT_SPEC"'.\n", LXL_SV_FMT_ARG(sv));
        }
        else {
            lxl_lexer__advance(&lexer);
        }
    }
    lxl_lexer_reset(&lexer);
    lexer.line_comment_openers = (const char*[]){"#", NULL};
    lexer.unnestable_comment_delims = (const struct delim_pair[]){{"/*", "*/"}, {0}};
    lexer.string_delims = "\"";
    lexer.string_escape_chars = "\\";
    lexer.string_types = (int []){0};
    for (int i = 0; !lxl_lexer_is_finished(&lexer); ++i) {
        /* printf("lxl_lexer__check_line_comment('%c'): %d\n", */
        /*        *lexer.current, */
        /*        lxl_lexer__check_line_comment(&lexer)); */
        struct lxl_token token = lxl_lexer_next_token(&lexer);
        struct lxl_string_view sv = lxl_token_value(token);
        printf("Token %d: '"LXL_SV_FMT_SPEC"' [type = %d]\n", i, LXL_SV_FMT_ARG(sv), token.token_type);
        if (token.token_type <= LXL_LERR_GENERIC) {
            printf("Error: %s.\n", lxl_error_message(token.token_type));
        }
    }
    return 0;
}

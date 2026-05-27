#include "../lexel.c"  // Source code include.
#include "c_lexer.h"

#include <stdio.h>
#include <ctype.h>

int main(void) {
    struct lxl_string_view src =
        LXL_SV_FROM_STRLIT_INIT("   assert(1 + 1 == 2);");
    struct lxl_lexer lexer = create_c_lexer(src);
    for (struct lxl_token token; !LXL_TOKEN_IS_END(token = lxl_lexer_next_token(&lexer));) {
        struct lxl_string_view token_sv = lxl_token_value(token);
        // struct lxl_string_view kind_sv = c_token_kind_name(token.kind);
        printf("%"LXL_SV_FMT_SPC"\n",
               LXL_SV_FMT_ARG(token_sv));
    }
    return 0;
}

struct lxl_lexer create_c_lexer(struct lxl_string_view src) {
    struct lxl_lexer lexer = lxl_lexer_new(src);
    // Query functions.
    lexer.match_word_init_char = match_word_init_char;
    lexer.match_word_char = match_word_char;
    lexer.match_int_prefix = match_int_prefix;
    lexer.match_punct = match_punct;
    // Token type getters.
    lexer.get_word_type = get_word_type;
    lexer.get_int_type = get_int_type;
    lexer.get_float_type = get_float_type;
    lexer.get_punct_type = get_punct_type;
    return lexer;
}

struct lxl_string_view c_token_kind_name(enum c_token_type type) {
    static struct lxl_string_view names[] = {
        C_TOKENS(C_TOKENS_STRING_TABLE)
    };
    LXL_ASSERT(0 <= type && type < sizeof names / sizeof names[0]);
    return names[type];
}

bool match_word_init_char(struct lxl_lexer *self) {
    lxl_UnicodeCodepoint ch = lxl_lexer__advance(self);
    if (isalpha(ch) || ch == '_') return true;
    lxl_lexer__rewind(self);
    return false;
}

bool match_word_char(struct lxl_lexer *self) {
    lxl_UnicodeCodepoint ch = lxl_lexer__advance(self);
    if (isalnum(ch) || ch == '_') return true;
    lxl_lexer__rewind(self);
    return false;
}

bool match_int_prefix(struct lxl_lexer *self) {
    lxl_UnicodeCodepoint ch = lxl_lexer__advance(self);
    self->match_int_digit = NULL;  // Use default integer lexer.
    if (ch == '0') {
        ch = lxl_lexer__advance(self);
        if (toupper(ch) == 'X') {
            // Hexadecimal.
            self->match_int_digit = lxl_lexer__match_int_digit_builtin_hex;
            return true;
        }
        if (toupper(ch) != 'B') {
            // Binary.
            return true;
        }
        // Octal.
        lxl_lexer__rewind(self);
        return true;
    }
    if ('1' <= ch && ch <= '9') return true;
    lxl_lexer__rewind(self);
    return false;
}

bool match_punct(struct lxl_lexer *self) {
    /* This function uses a switch-trie to lex punctuation.
     * It rewinds the lexer as appropriate on failure.
     * Puncatuation sequences are separated into categories
     * with the cases stacked for each category.
     */
    lxl_UnicodeCodepoint ch = lxl_lexer__advance(self);
    switch (ch) {
    case '(': case ')':
    case '{': case '}':
    case '[': case ']':
    case ',':
    case ':':
    case ';':
    case '\\':
    case '?':
    case '~':
        /* Single-character only. */
        return true;
    case '!':
    case '%':
    case '*':
    case '=':
    case '^':
        /* Either single- or double-character with '='. */
        ch = lxl_lexer__advance(self);
        if (ch != '=') {
            lxl_lexer__rewind(self);
        }
        return true;
    case '&':
    case '+':
    case '|':
    {
        /* Single-character or double-character with either itself or '='. */
        lxl_UnicodeCodepoint ch2 = lxl_lexer__advance(self);
        if (ch2 != ch && ch2 != '=') {
            lxl_lexer__rewind(self);
        }
        return true;
    }
    case '-':
    {
        /* Single-character or double-character with either itself, '=', or '>'. */
        lxl_UnicodeCodepoint ch2 = lxl_lexer__advance(self);
        if (ch2 != ch && ch2 != '=' && ch2 != '>') {
            lxl_lexer__rewind(self);
        }
        return true;

    }
    case '<':
    case '>':
    {
        /* Single-character, or double-character with either itself or '=',
           or triple-character with itself twice and then '='. */
        lxl_UnicodeCodepoint ch2 = lxl_lexer__advance(self);
        if (ch2 == '=') return true;
        if (ch2 == ch) {
            lxl_UnicodeCodepoint ch3 = lxl_lexer__advance(self);
            if (ch3 == '=') return true;
            lxl_lexer__rewind(self);  // ch3.
        }
        lxl_lexer__rewind(self);  // ch2.
        return true;
    }
    case '.':
    {
        /* Either single-character or triple-character with itself three times. */
        lxl_UnicodeCodepoint ch2 = lxl_lexer__advance(self);
        if (ch2 == ch) {
            lxl_UnicodeCodepoint ch3 = lxl_lexer__advance(self);
            if (ch3 == ch) return true;
            lxl_lexer__rewind(self);  // ch3.
        }
        lxl_lexer__rewind(self);  // ch2.
        return true;
    }
    }  // switch (ch).
    lxl_lexer__rewind(self);  // ch.
    return false;
}

int get_word_type(struct lxl_lexer *self) {
    (void)self;
    return CTOK_IDENTIFIER;
}

int get_int_type(struct lxl_lexer *self) {
    (void)self;
    return CTOK_LIT_INT;
}

int get_float_type(struct lxl_lexer *self) {
    (void)self;
    return CTOK_LIT_FLOAT;
}

int get_punct_type(struct lxl_lexer *self) {
    struct lxl_string_view token_sv = lxl_lexer__peek_token(self);
    LXL_ASSERT(token_sv.length >= 1);
    if (token_sv.length == 1) {
        // Single-character tokens have their type as their value.
        return token_sv.start[0];
    }
    if (lxl_sv_eq(token_sv, LXL_SV_FROM_STRLIT("!=")))  return CTOK_BANG_EQ;
    if (lxl_sv_eq(token_sv, LXL_SV_FROM_STRLIT("%=")))  return CTOK_PERCENT_EQ;
    if (lxl_sv_eq(token_sv, LXL_SV_FROM_STRLIT("&&")))  return CTOK_AMPERSAND_AMPERSAND;
    if (lxl_sv_eq(token_sv, LXL_SV_FROM_STRLIT("&=")))  return CTOK_AMPERSAND_EQ;
    if (lxl_sv_eq(token_sv, LXL_SV_FROM_STRLIT("*=")))  return CTOK_ASTERISK_EQ;
    if (lxl_sv_eq(token_sv, LXL_SV_FROM_STRLIT("++")))  return CTOK_PLUS_PLUS;
    if (lxl_sv_eq(token_sv, LXL_SV_FROM_STRLIT("+=")))  return CTOK_PLUS_EQ;
    if (lxl_sv_eq(token_sv, LXL_SV_FROM_STRLIT("--")))  return CTOK_MINUS_MINUS;
    if (lxl_sv_eq(token_sv, LXL_SV_FROM_STRLIT("-=")))  return CTOK_MINUS_EQ;
    if (lxl_sv_eq(token_sv, LXL_SV_FROM_STRLIT("->")))  return CTOK_ARROW;
    if (lxl_sv_eq(token_sv, LXL_SV_FROM_STRLIT("..."))) return CTOK_ELIPSIS;
    if (lxl_sv_eq(token_sv, LXL_SV_FROM_STRLIT("/=")))  return CTOK_SLASH_EQ;
    if (lxl_sv_eq(token_sv, LXL_SV_FROM_STRLIT("<<")))  return CTOK_LT_LT;
    if (lxl_sv_eq(token_sv, LXL_SV_FROM_STRLIT("<<="))) return CTOK_LT_LT_EQ;
    if (lxl_sv_eq(token_sv, LXL_SV_FROM_STRLIT("<=")))  return CTOK_LT_EQ;
    if (lxl_sv_eq(token_sv, LXL_SV_FROM_STRLIT("==")))  return CTOK_EQ_EQ;
    if (lxl_sv_eq(token_sv, LXL_SV_FROM_STRLIT(">=")))  return CTOK_GT_EQ;
    if (lxl_sv_eq(token_sv, LXL_SV_FROM_STRLIT(">>")))  return CTOK_GT_GT;
    if (lxl_sv_eq(token_sv, LXL_SV_FROM_STRLIT(">>="))) return CTOK_GT_GT_EQ;
    if (lxl_sv_eq(token_sv, LXL_SV_FROM_STRLIT("^=")))  return CTOK_CARET_EQ;
    if (lxl_sv_eq(token_sv, LXL_SV_FROM_STRLIT("|=")))  return CTOK_VBAR_EQ;
    if (lxl_sv_eq(token_sv, LXL_SV_FROM_STRLIT("||")))  return CTOK_VBAR_VBAR;
    return LXL_LERR_GENERIC;
}

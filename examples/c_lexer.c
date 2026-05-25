#include "../lexel.c"  // Source code include.
#include "c_lexer.h"

#include <stdio.h>
#include <ctype.h>

int main(void) {
    printf("Hello, Lexel!\n");
    return 0;
}

bool match_word_char(struct lxl_lexer *self) {
    lxl_UnicodeCodepoint ch = lxl_lexer__advance(self);
    if (isalnum(ch) || ch == '_') return true;
    lxl_lexer__rewind(self);
    return false;
}

bool match_int_prefix(struct lxl_lexer *self) {
    lxl_UnicodeCodepoint ch = lxl_lexer__advance(self);
    if (ch == '0') {
        ch = lxl_lexer__advance(self);
        if (ch != 'x' && ch != 'X' && ch != 'b' && ch != 'B') {
            lxl_lexer__rewind(self);
        }
        return true;
    }
    if ('1' <= ch && ch <= '9') return true;
    lxl_lexer__rewind(self);
    return false;
}

bool match_int_digit(struct lxl_lexer *self) {
    lxl_UnicodeCodepoint ch = lxl_lexer__advance(self);
    if (isxdigit(ch) || ch == '\'') return true;
    lxl_lexer__rewind(self);
    return false;
}

bool match_float_prefix(struct lxl_lexer *self) {
    if (match_int_prefix(self)) return true;
    lxl_UnicodeCodepoint ch = lxl_lexer__advance(self);
    if (ch == '.') return true;
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

int get_punct_type(struct lxl_lexer *self) {
    struct lxl_string_view token_sv = lxl_token_value(self->token);
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

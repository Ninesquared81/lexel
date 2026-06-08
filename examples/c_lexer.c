#include "../lexel.c"  // Source code include.
#include "c_lexer.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

int main(void) {
    // struct lxl_string_view src =
    //     LXL_SV_FROM_STRLIT_INIT(
    //         "int printf(const char *restrict, ...);\n"
    //         "\n"
    //         "/+ This was a triumph.\n"
    //         " + I'm making a note here\n"
    //         " + HUGE SUCCESS! /++/ @\n"
    //         " +/\n"
    //         "int main(void) {\n"
    //         "    assert(1 + 1 == 2);\n"
    //         "    float x = 42.;\n"
    //         "    float y = x + .5f;\n"
    //         "    float z = 0x11.2fp0;\n"
    //         "    float alpha = 3.14'15f;\n"
    //         ".\n"
    //         "    printf(\"Hello, World!\\n\");\n"
    //         "}\n"
    //         );
    FILE *fp = fopen(__FILE__, "r");
    if (!fp) return 1;
    struct lxl_string_view src = read_file_logical_lines(fp);
    fclose(fp);
    struct c_lexer c_lexer = {0};
    init_c_lexer(&c_lexer, src);
    struct lxl_lexer *lexer = &c_lexer.base;
    int n_semis = 0;
    for (struct lxl_token token; !lxl_token_is_end(token = lxl_lexer_next_token(lexer));) {
        show_token(token);
        if (token.kind == CTOK_SEMICOLON) ++n_semis;
    }
    fprintf(stderr, "Number of semicolons: %d\n", n_semis);
    return 0;
}

struct lxl_string_view read_file_logical_lines(FILE *fp) {
    int ch = EOF;
    size_t size = 1024;
    size_t length = 0;
    char *buf = malloc(size);
    while ((ch = fgetc(fp)) != EOF) {
        LXL_ASSERT(length <= size);
        if (length == size) {
            if (!grow_buffer(&buf, &size)) return lxl_sv_empty();
            LXL_ASSERT(size > length);
        }
        if (ch == '\\') {
            int ch2 = fgetc(fp);
            if (ch2 == '\n') continue;
            ungetc(ch2, fp);
        }
        buf[length++] = ch;
    }
    return lxl_sv_from_startlen(buf, length);
}

void *grow_buffer(char **buf, size_t *size) {
    LXL_ASSERT(*size > 0);
    *size += *size/2;
    void *temp = realloc(*buf, *size);
    if (!temp) {
        free(*buf);
        *size = 0;
        return NULL;
    }
    *buf = temp;
    return *buf;
}

void init_c_lexer(struct c_lexer *lexer, struct lxl_string_view src) {
    // Lexer base.
    lexer->base = lxl_lexer_new(src);

    // Query functions.
    lexer->base.match_comment_line_opener = match_comment_line_opener;
    lexer->base.match_comment_block_opener = match_comment_block_opener;
    lexer->base.match_comment_block_closer = match_comment_block_closer;
    lexer->base.match_comment_block_nest_opener = match_comment_block_nest_opener;
    lexer->base.match_comment_block_nest_closer = match_comment_block_nest_closer;
    lexer->base.match_word_init_char = match_word_init_char;
    lexer->base.match_word_char = match_word_char;
    lexer->base.match_int_prefix = match_int_prefix;
    lexer->base.match_int_digit = match_digit_dec;
    lexer->base.match_int_suffix = match_int_suffix;
    lexer->base.match_float_prefix = match_float_prefix;
    lexer->base.match_float_digit = match_digit_dec;
    lexer->base.match_float_suffix = match_float_suffix;
    lexer->base.match_punct = match_punct;
    lexer->base.match_string_char = match_string_char;
    // Token kind getters.
    lexer->base.get_word_kind = get_word_kind;
    lexer->base.get_int_kind = get_int_kind;
    lexer->base.get_float_kind = get_float_kind;
    lexer->base.get_punct_kind = get_punct_kind;
    lexer->base.get_string_kind = get_string_kind;
    // Hook functions.
    lexer->base.before_integer_hook = before_integer_hook;
    lexer->base.after_integer_hook = after_integer_hook_verify_suffix;
    lexer->base.before_float_frac_hook = before_float_frac_hook;
    lexer->base.before_float_exp_hook = before_float_exp_hook;
    lexer->base.after_float_hook = after_float_hook_verify_suffix;

    // Preprocessor.
    lexer->base.custom_info = &lexer->pp_info;
}

struct lxl_string_view c_token_kind_name(int kind) {
    static struct lxl_string_view names[] = {
        C_TOKENS(C_TOKENS_STRING_TABLE)
    };
    LXL_ASSERT(0 <= kind && (size_t)kind < sizeof names / sizeof names[0]);
    return names[kind];
}

const char *escape_char(char ch) {
    switch (ch) {
    case '\n': return "\\n";
    case '\t': return "\\t";
    case '\f': return "\\f";
    case '\v': return "\\v";
    case '\r': return "\\r";
    }
    static char ch_buf[5];
    if (isprint(ch)) {
        snprintf(ch_buf, sizeof ch_buf, "%c", ch);
    }
    else {
        snprintf(ch_buf, sizeof ch_buf, "\\%o", (unsigned)ch);
    }
    return ch_buf;
}

void show_token(struct lxl_token token) {
    if (lxl_token_is_error(token)) {
        struct lxl_string_view msg = lxl_error_message(token.kind);
        printf("Error: %"LXL_SV_FMT_SPC".\n", LXL_SV_FMT_ARG(msg));
    }
    struct lxl_string_view kind_sv = lxl_token_kind_name(token, c_token_kind_name);
    printf("%-32"LXL_SV_FMT_SPC"", LXL_SV_FMT_ARG(kind_sv));
    for (const char *p = token.start; p < token.end; ++p) {
        const char *esc = escape_char(*p);
        printf("%s", esc);
    }
    printf("\n");
}

bool match_comment_line_opener(struct lxl_lexer *self) {
    return lxl_lexer__match_string(self, LXL_SV_FROM_STRLIT("//"));
}

bool match_comment_block_opener(struct lxl_lexer *self) {
    return lxl_lexer__match_string(self, LXL_SV_FROM_STRLIT("/*"));
}

bool match_comment_block_closer(struct lxl_lexer *self) {
    return lxl_lexer__match_string(self, LXL_SV_FROM_STRLIT("*/"));
}

bool match_comment_block_nest_opener(struct lxl_lexer *self) {
    return lxl_lexer__match_string(self, LXL_SV_FROM_STRLIT("/+"));
}

bool match_comment_block_nest_closer(struct lxl_lexer *self) {
    return lxl_lexer__match_string(self, LXL_SV_FROM_STRLIT("+/"));
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
    if (lxl_lexer__match_string(self, LXL_SV_FROM_STRLIT("0x"))) return true;
    if (lxl_lexer__match_string(self, LXL_SV_FROM_STRLIT("0X"))) return true;
    if (lxl_lexer__match_string(self, LXL_SV_FROM_STRLIT("0b"))) return true;
    if (lxl_lexer__match_string(self, LXL_SV_FROM_STRLIT("0B"))) return true;
    return lxl_lexer__match_chars(self, LXL_SV_FROM_STRLIT("0123456789"));
}

bool match_digit_dec(struct lxl_lexer *self) {
    return lxl_lexer__match_chars(self, LXL_SV_FROM_STRLIT("0123456789'"));
}

bool match_digit_hex(struct lxl_lexer *self) {
    return lxl_lexer__match_chars(self, LXL_SV_FROM_STRLIT("0123456789'ABCDEFabcdef"));
}

bool match_int_suffix(struct lxl_lexer *self) {
    if (!lxl_lexer__match_word_char(self)) return false;
    while (lxl_lexer__match_word_char(self)) {
        /* Do nothing. */
    }
    return true;
}

bool match_float_prefix(struct lxl_lexer *self) {
    if (lxl_lexer__match_string(self, LXL_SV_FROM_STRLIT("0x"))) return true;
    if (lxl_lexer__match_string(self, LXL_SV_FROM_STRLIT("0X"))) return true;
    return lxl_lexer__match_chars(self, LXL_SV_FROM_STRLIT("0123456789"));
}

bool match_float_suffix(struct lxl_lexer *self) {
    if (!lxl_lexer__match_word_char(self)) return false;
    while (lxl_lexer__match_word_char(self)) {
        /* Do nothing. */
    }
    return true;
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
    case '/':
    case '=':
    case '^':
        /* Either single- or double-character with '='. */
        ch = lxl_lexer__advance(self);
        if (ch != '=') {
            lxl_lexer__rewind(self);
        }
        return true;
    case '#':
    {
        /* Either single- or double-character with itself. */
        lxl_UnicodeCodepoint ch2 = lxl_lexer__advance(self);
        if (ch2 != ch) {
            lxl_lexer__rewind(self);
        }
        return true;
    }
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

bool match_string_char(struct lxl_lexer *self) {
    if (lxl_lexer__match_string(self, LXL_SV_FROM_STRLIT("\\"))) {
        return !!lxl_lexer__advance(self);
    }
    return lxl_lexer__match_string_char_default(self);
}

int get_word_kind(struct lxl_lexer *self) {
    struct lxl_string_view token_sv = lxl_lexer__peek_token(self);
    if (lxl_sv_eq_strings(token_sv, "alignas", "_Alignas")) return CTOK_KW_ALIGNAS;
    if (lxl_sv_eq_strings(token_sv, "alignof", "_Alignof")) return CTOK_KW_ALIGNOF;
    if (lxl_sv_eq_strings(token_sv, "_Atomic")) return CTOK_KW_ATOMIC;
    if (lxl_sv_eq_strings(token_sv, "auto")) return CTOK_KW_AUTO;
    if (lxl_sv_eq_strings(token_sv, "_BitInt")) return CTOK_KW_BITINT;
    if (lxl_sv_eq_strings(token_sv, "_Bool")) return CTOK_KW_BOOL;
    if (lxl_sv_eq_strings(token_sv, "break")) return CTOK_KW_BREAK;
    if (lxl_sv_eq_strings(token_sv, "_Complex")) return CTOK_KW_COMPLEX;
    if (lxl_sv_eq_strings(token_sv, "case")) return CTOK_KW_CASE;
    if (lxl_sv_eq_strings(token_sv, "char")) return CTOK_KW_CHAR;
    if (lxl_sv_eq_strings(token_sv, "const")) return CTOK_KW_CONST;
    if (lxl_sv_eq_strings(token_sv, "constexpr")) return CTOK_KW_CONSTEXPR;
    if (lxl_sv_eq_strings(token_sv, "continue")) return CTOK_KW_CONTINUE;
    if (lxl_sv_eq_strings(token_sv, "_Decimal128")) return CTOK_KW_DECIMAL128;
    if (lxl_sv_eq_strings(token_sv, "_Decimal32")) return CTOK_KW_DECIMAL32;
    if (lxl_sv_eq_strings(token_sv, "_Decimal64")) return CTOK_KW_DECIMAL64;
    if (lxl_sv_eq_strings(token_sv, "default")) return CTOK_KW_DEFAULT;
    if (lxl_sv_eq_strings(token_sv, "do")) return CTOK_KW_DO;
    if (lxl_sv_eq_strings(token_sv, "double")) return CTOK_KW_DOUBLE;
    if (lxl_sv_eq_strings(token_sv, "else")) return CTOK_KW_ELSE;
    if (lxl_sv_eq_strings(token_sv, "enum")) return CTOK_KW_ENUM;
    if (lxl_sv_eq_strings(token_sv, "extern")) return CTOK_KW_EXTERN;
    if (lxl_sv_eq_strings(token_sv, "false")) return CTOK_KW_FALSE;
    if (lxl_sv_eq_strings(token_sv, "float")) return CTOK_KW_FLOAT;
    if (lxl_sv_eq_strings(token_sv, "for")) return CTOK_KW_FOR;
    if (lxl_sv_eq_strings(token_sv, "_Generic")) return CTOK_KW_GENERIC;
    if (lxl_sv_eq_strings(token_sv, "goto")) return CTOK_KW_GOTO;
    if (lxl_sv_eq_strings(token_sv, "if")) return CTOK_KW_IF;
    if (lxl_sv_eq_strings(token_sv, "_Imaginary")) return CTOK_KW_IMAGINARY;
    if (lxl_sv_eq_strings(token_sv, "inline")) return CTOK_KW_INLINE;
    if (lxl_sv_eq_strings(token_sv, "int")) return CTOK_KW_INT;
    if (lxl_sv_eq_strings(token_sv, "long")) return CTOK_KW_LONG;
    if (lxl_sv_eq_strings(token_sv, "_Noreturn")) return CTOK_KW_NORETURN;
    if (lxl_sv_eq_strings(token_sv, "nullptr")) return CTOK_KW_NULLPTR;
    if (lxl_sv_eq_strings(token_sv, "register")) return CTOK_KW_REGISTER;
    if (lxl_sv_eq_strings(token_sv, "restrict")) return CTOK_KW_RESTRICT;
    if (lxl_sv_eq_strings(token_sv, "return")) return CTOK_KW_RETURN;
    if (lxl_sv_eq_strings(token_sv, "short")) return CTOK_KW_SHORT;
    if (lxl_sv_eq_strings(token_sv, "signed")) return CTOK_KW_SIGNED;
    if (lxl_sv_eq_strings(token_sv, "sizeof")) return CTOK_KW_SIZEOF;
    if (lxl_sv_eq_strings(token_sv, "static")) return CTOK_KW_STATIC;
    if (lxl_sv_eq_strings(token_sv, "static_assert", "_Static_Assert")) return CTOK_KW_STATIC_ASSERT;
    if (lxl_sv_eq_strings(token_sv, "struct")) return CTOK_KW_STRUCT;
    if (lxl_sv_eq_strings(token_sv, "switch")) return CTOK_KW_SWITCH;
    if (lxl_sv_eq_strings(token_sv, "thread_local", "_Thread_Local")) return CTOK_KW_THREAD_LOCAL;
    if (lxl_sv_eq_strings(token_sv, "true")) return CTOK_KW_TRUE;
    if (lxl_sv_eq_strings(token_sv, "typedef")) return CTOK_KW_TYPEDEF;
    if (lxl_sv_eq_strings(token_sv, "typeof")) return CTOK_KW_TYPEOF;
    if (lxl_sv_eq_strings(token_sv, "typeof_unqual")) return CTOK_KW_TYPEOF_UNQUAL;
    if (lxl_sv_eq_strings(token_sv, "union")) return CTOK_KW_UNION;
    if (lxl_sv_eq_strings(token_sv, "unsigned")) return CTOK_KW_UNSIGNED;
    if (lxl_sv_eq_strings(token_sv, "void")) return CTOK_KW_VOID;
    if (lxl_sv_eq_strings(token_sv, "volatile")) return CTOK_KW_VOLATILE;
    if (lxl_sv_eq_strings(token_sv, "while")) return CTOK_KW_WHILE;

    return CTOK_IDENTIFIER;
}

int get_int_kind(struct lxl_lexer *self) {
    (void)self;
    return CTOK_LIT_INT;
}

int get_float_kind(struct lxl_lexer *self) {
    (void)self;
    return CTOK_LIT_FLOAT;
}

int get_punct_kind(struct lxl_lexer *self) {
    struct lxl_string_view token_sv = lxl_lexer__peek_token(self);
    LXL_ASSERT(token_sv.length >= 1);
    if (token_sv.length == 1) {
        // Single-character tokens have their kind as their value.
        return token_sv.start[0];
    }
    if (lxl_sv_eq_strings(token_sv, "!="))  return CTOK_BANG_EQ;
    if (lxl_sv_eq_strings(token_sv, "##"))  return CTOK_HASH_HASH;
    if (lxl_sv_eq_strings(token_sv, "%="))  return CTOK_PERCENT_EQ;
    if (lxl_sv_eq_strings(token_sv, "&&"))  return CTOK_AMPERSAND_AMPERSAND;
    if (lxl_sv_eq_strings(token_sv, "&="))  return CTOK_AMPERSAND_EQ;
    if (lxl_sv_eq_strings(token_sv, "*="))  return CTOK_ASTERISK_EQ;
    if (lxl_sv_eq_strings(token_sv, "++"))  return CTOK_PLUS_PLUS;
    if (lxl_sv_eq_strings(token_sv, "+="))  return CTOK_PLUS_EQ;
    if (lxl_sv_eq_strings(token_sv, "--"))  return CTOK_MINUS_MINUS;
    if (lxl_sv_eq_strings(token_sv, "-="))  return CTOK_MINUS_EQ;
    if (lxl_sv_eq_strings(token_sv, "->"))  return CTOK_ARROW;
    if (lxl_sv_eq_strings(token_sv, "...")) return CTOK_ELIPSIS;
    if (lxl_sv_eq_strings(token_sv, "/="))  return CTOK_SLASH_EQ;
    if (lxl_sv_eq_strings(token_sv, "<<"))  return CTOK_LT_LT;
    if (lxl_sv_eq_strings(token_sv, "<<=")) return CTOK_LT_LT_EQ;
    if (lxl_sv_eq_strings(token_sv, "<="))  return CTOK_LT_EQ;
    if (lxl_sv_eq_strings(token_sv, "=="))  return CTOK_EQ_EQ;
    if (lxl_sv_eq_strings(token_sv, ">="))  return CTOK_GT_EQ;
    if (lxl_sv_eq_strings(token_sv, ">>"))  return CTOK_GT_GT;
    if (lxl_sv_eq_strings(token_sv, ">>=")) return CTOK_GT_GT_EQ;
    if (lxl_sv_eq_strings(token_sv, "^="))  return CTOK_CARET_EQ;
    if (lxl_sv_eq_strings(token_sv, "|="))  return CTOK_VBAR_EQ;
    if (lxl_sv_eq_strings(token_sv, "||"))  return CTOK_VBAR_VBAR;
    return LXL_LERR_GENERIC;
}

int get_string_kind(struct lxl_lexer *self) {
    struct lxl_string_view token_sv = lxl_lexer__peek_token(self);
    LXL_ASSERT(token_sv.length >= 2);  // 1 for the opener and 1 for the closer.
    char opener = token_sv.start[0];
    if (opener == '"') return CTOK_LIT_STRING;
    if (opener == '\'') return CTOK_LIT_CHAR;
    LXL_UNREACHABLE();
    return LXL_LERR_GENERIC;
}

void before_integer_hook(struct lxl_lexer *self) {
    struct lxl_string_view token_sv = lxl_lexer__peek_token(self);
    if (lxl_sv_has_prefix_strings(token_sv, "0x", "0X")) {
        self->match_int_digit = match_digit_hex;
    }
    else {
        self->match_int_digit = match_digit_dec;
    }
}

void after_integer_hook_verify_suffix(struct lxl_lexer *self) {
    struct lxl_string_view token_sv = lxl_lexer__peek_token(self);
    int (*digit_pred)(int ch) = is_c_digit_dec;
    if (lxl_sv_has_prefix_strings(token_sv, "0x", "0X")) {
        token_sv = lxl_sv_slice_end(token_sv, -2);
        digit_pred = is_c_digit_hex;
    }
    else if (lxl_sv_has_prefix_strings(token_sv, "0b", "0B")) {
        token_sv = lxl_sv_slice_start(token_sv, 2);
    }
    token_sv = lxl_sv_remove_predicate_left(token_sv, digit_pred);
    if (lxl_sv_has_prefix_strings(token_sv, "u", "U")) {
        token_sv = lxl_sv_slice_end(token_sv, 1);
    }
    else if (lxl_sv_has_suffix_strings(token_sv, "u", "U")) {
        token_sv = lxl_sv_slice_start(token_sv, -1);
    }
    if (lxl_sv_is_empty(token_sv)) return;
    if (lxl_sv_eq_strings(token_sv, "l", "L", "ll", "LL")) return;
    lxl_lexer__error(self, LXL_LERR_INVALID_INTEGER);
    self->next_state = lxl_lstate_Return;
}

void before_float_frac_hook(struct lxl_lexer *self) {
    struct lxl_string_view token_sv = lxl_lexer__peek_token(self);
    if (lxl_sv_has_prefix_strings(token_sv, "0x", "0X")) {
        self->match_float_digit = match_digit_hex;
    }
    else {
        self->match_float_digit = match_digit_dec;
    }
}

void before_float_exp_hook(struct lxl_lexer *self) {
    self->match_float_digit = match_digit_dec;
}

void after_float_hook_verify_suffix(struct lxl_lexer *self) {
    struct lxl_string_view token_sv = lxl_lexer__peek_token(self);
    int (*digit_pred)(int ch) = is_c_digit_dec;
    if (lxl_sv_has_prefix_strings(token_sv, "0x", "0X")) {
        token_sv = lxl_sv_slice_end(token_sv, -2);
        digit_pred = is_c_digit_hex;
    }
    else if (lxl_sv_has_prefix_strings(token_sv, "0b", "0B")) {
        lxl_lexer__error(self, LXL_LERR_INVALID_FLOAT);
        return;
    }
    token_sv = lxl_sv_remove_predicate_left(token_sv, digit_pred);
    if (lxl_sv_has_prefix_strings(token_sv, ".")) {
        token_sv = lxl_sv_slice_end(token_sv, 1);
        token_sv = lxl_sv_remove_predicate_left(token_sv, digit_pred);
    }
    if (digit_pred == is_c_digit_hex) {
        if (!lxl_sv_has_prefix_strings(token_sv, "p", "P")) {
            lxl_lexer__error(self, LXL_LERR_INVALID_FLOAT);
            return;
        }
        token_sv = lxl_sv_slice_end(token_sv, 1);
    }
    else if (lxl_sv_has_prefix_strings(token_sv, "e", "E")) {
        token_sv = lxl_sv_slice_end(token_sv, 1);
    }
    token_sv = lxl_sv_remove_predicate_left(token_sv, is_c_digit_dec); // NOTE: exponent is always decimal.
    if (lxl_sv_is_empty(token_sv)) return;  // OK.
    if (lxl_sv_eq_strings(token_sv, "f", "F", "l", "L")) return;  // OK.
    // Invalid suffix.
    lxl_lexer__error(self, LXL_LERR_INVALID_FLOAT);
    self->next_state = lxl_lstate_Return;
}

int is_c_digit_dec(int ch) {
    return isdigit(ch) || ch == '\'';
}

int is_c_digit_hex(int ch) {
    return isxdigit(ch) || ch == '\'';
}

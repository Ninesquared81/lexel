#ifndef LEXEL_H
#define LEXEL_H

#include <stddef.h>  // size_t

// LEXEL CORE.

// These are the core definitions for lexel -- the lexer and token.

struct lxl_lexer {
    const char *start;  // The start of the lexer's source code.
    const char *end;    // The end of the lexer's source code.
    const char *current;
};

struct lxl_token {
    const char *start;  // The start of the token.
    const char *end;    // The end of the token.
    int token_type;     // The type of the lexical token. Negative values have special menanings.
};

// END LEXEL CORE.

// LEXEL ADDITIONAL.

// Additional definitions beyond the core above.

struct lxl_string_view {
    const char *start;
    size_t length;
};

// END LEXEL ADDITIONAL.


// LEXEL MAGIC VALUES (MVs).

// These enums define human-friendly names for the various magic values used by lexel.

enum lxl__token_mvs {
    LXL_TOKENS_END = -1,
};

// END LEXEL MAGIC VALUES.


// LEXER EXTERNAL INTERFACE.

// These functions are for communicating with the lexer, e.g. when parsing.

// Create a new `lxl_lexer` object from start and end pointers.
// `start` and `end` must be valid, non-NULL pointers, moreover, `end` must point one past the end
//  of the the string beginning at `start`.
struct lxl_lexer lxl_lexer_new(const char *start, const char *end);

// Create a new `lxl_lexer` object from a string view.
struct lxl_lexer lxl_lexer_from_sv(struct lxl_string_view sv);

// Get the next token from the lexer. A token of type LXL_TOKENS_END is returned when
// the token stream is exhausted.
struct lxl_token lxl_lexer_next_token(struct lxl_lexer *lexer);

// Return whether the token stream of the lexer is exhausted
// (i.e. there are no more tokens in the source code).
int lxl_lexer_is_finished(struct lxl_lexer *lexer);

// END LEXER EXTERNAL INTERFACE.


// LEXER INTERNAL INTERFACE.

// These functions are used by the lexer to alter its own state.
// They should not be used from a parser, but the interface is exposed to make writing a custom lexer easier.


// Return the number of characters consumed so far.
size_t lxl_lexer__head_length(struct lxl_lexer *lexer);
// Return the number of characters left in the lexer's source.
size_t lxl_lexer__tail_length(struct lxl_lexer *lexer);

// Return whether the lexer is at the end of its input.
int lxl_lexer__is_at_end(struct lxl_lexer *lexer);
// Return the current character and advance the lexer to the next character.
char lxl_lexer__advance(struct lxl_lexer *lexer);
// Advance the lexer by up to n characters and return whether all n characters could be advanced
// (this will be false when there are fewer than n characters left, in which case, the lexer
// reaches the end and stops).
int lxl_lexer__advance_by(struct lxl_lexer *lexer, size_t n);

// Return whether the current character matches any of those passed, but do not consume it.
int lxl_lexer__check_chars(struct lxl_lexer *lexer, const char *chars);
// Return whether the next characters match exactly the string passed, but do not consume them.
int lxl_lexer__check_string(struct lxl_lexer *lexer, const char *s);
// Return whether the next n characters match the first n characters of the string passed,
// but do not consume them.
int lxl_lexer__check_string_n(struct lxl_lexer *lexer, const char *s, size_t n);

// Return whether the current character matches any of those passed, and consume it if so.
int lxl_lexer__match_chars(struct lxl_lexer *lexer, const char *chars);
// Return whether the next characters match exactly the string passed, and consume them if so.
int lxl_lexer__match_string(struct lxl_lexer *lexer, const char *s);
// Return whether the next n characters match the first n characters of the string passed,
// and consume them if so.
int lxl_lexer__match_string_n(struct lxl_lexer *lexer, const char *s, size_t n);

// END LEXER INTERNAL INTERFACE.


// LEXEL STRING VIEW.

// Functions and macros for working with string views.

// Create a `string_view` object from a null-terminated string.
struct lxl_string_view lxl_sv_from_string(const char *s);
// Create a `string_view` object from a C string literal token.
#define LXL_SV_FROM_STRLIT(lit) (struct lxl_string_view) {.start = lit, .length = sizeof(lit) - 1}
// Create a `string_view` object from `start` and `end` pointers.
struct lxl_string_view lxl_sv_from_startend(const char *start, const char * end);

// Get a pointer to (one past) the end of a string view.
#define LXL_SV_END(sv) (sv).start + (sv).length

// END LEXEL STRING VIEW.


// Implementation.

#ifdef LEXEL_IMPLEMENTATION

#include <stdbool.h>
#include <string.h>

struct lxl_lexer lxl_lexer_new(const char *start, const char *end) {
    return (struct lxl_lexer) {
        .start = start,
        .end = end,
        .current = start,
    };
}

struct lxl_lexer lxl_lexer_from_sv(struct lxl_string_view sv) {
    return lxl_lexer_new(sv.start, LXL_SV_END(sv));
}

size_t lxl_lexer__head_length(struct lxl_lexer *lexer) {
    return lexer->current - lexer->start;
}

size_t lxl_lexer__tail_length(struct lxl_lexer *lexer) {
    return lexer->end - lexer->current;
}

int lxl_lexer__is_at_end(struct lxl_lexer *lexer) {
    return lexer->current >= lexer->end;
}

char lxl_lexer__advance(struct lxl_lexer *lexer) {
    if (lxl_lexer__is_at_end(lexer)) return '\0';
    ++lexer->current;
    return lexer->current[-1];
}

int lxl_lexer__advance_by(struct lxl_lexer *lexer, size_t n) {
    size_t tail_length = lxl_lexer__tail_length(lexer);
    if (n < tail_length) {
        lexer->current = lexer->end;
        return false;
    }
    lexer->current += n;
    return true;
}

int lxl_lexer__check_chars(struct lxl_lexer *lexer, const char *chars) {
    while (*chars != '\0') {
        if (*lexer->current == *chars) return true;
        ++chars;
    }
    return false;
}

int lxl_lexer__check_string(struct lxl_lexer *lexer, const char *s) {
    return strncmp(lexer->current, s, lexer->end - lexer->current) == 0;
}

int lxl_lexer__check_string_n(struct lxl_lexer *lexer, const char *s, size_t n) {
    size_t tail_length = lxl_lexer__tail_length(lexer);
    if (n < tail_length) n = tail_length;
    return strncmp(lexer->current, s, n) == 0;
}

int lxl_lexer__match_chars(struct lxl_lexer *lexer, const char *chars) {
    if (lxl_lexer__check_chars(lexer, chars)) {
        return !!lxl_lexer__advance(lexer);
    }
    return false;
}

int lxl_lexer__match_string(struct lxl_lexer *lexer, const char *s) {
    if (lxl_lexer__check_string(lexer, s)) {
        size_t length = strlen(s);
        return !!lxl_lexer__advance_by(lexer, length);
    }
    return false;
}

int lxl_lexer__match_string_n(struct lxl_lexer *lexer, const char *s, size_t n) {
    if (lxl_lexer__check_string_n(lexer, s, n)) {
        return !!lxl_lexer__advance_by(lexer, n);
    }
    return false;
}

struct lxl_string_view lxl_sv_from_string(const char *s) {
    return (struct lxl_string_view) {.start = s, .length = strlen(s)};
}

struct lxl_string_view lxl_sv_from_startend(const char *start, const char *end) {
    return (struct lxl_string_view) {.start = start, .length = end - start};
}

#undef LEXEL_IMPLEMENTATION
#endif  // LEXEL_IMPLEMENTATION

#endif  // LEXEL_H

#ifndef LEXEL_H
#define LEXEL_H

#include <stdbool.h>  // bool, false, true -- requires C99
#include <stddef.h>   // size_t, ptrdiff_t
#include <limits.h>   // INT_MAX

// LEXEL CORE.

// These are the core definitions for lexel -- the lexer and token.

// The line and column position with text.
struct lxl_location {
    int line, column;
};

// Lex error codes.
enum lxl_lex_error {
    LXL_LERR_OK = 0,                  // No error.
    LXL_LERR_GENERIC = -16,           // Generic error.
    LXL_LERR_EOF = -17,               // Unexepected EOF.
    LXL_LERR_UNCLOSED_COMMENT = -18,  // A block comment had no closing delimiter before the end.
};

// A pair of delimiters for block comments, e.g. "/*" and "*/" for C-style comments.
struct delim_pair {
    const char *opener;
    const char *closer;
};

// The main lexer object.
struct lxl_lexer {
    const char *start;        // The start of the lexer's source code.
    const char *end;          // The end of the lexer's source code.
    const char *current;      // Pointer to the current character.
    struct lxl_location pos;  // The current position (line, col) in the source.
    const char *const *line_comment_openers;             // List of line comment openers.
    const struct delim_pair *nestable_comment_delims;    // List of paired nestable block comment delimiters.
    const struct delim_pair *unnestable_comment_delims;  // List of paired unnestable block comment delimiters.
    enum lxl_lex_error error; // Error code set to the current lexing error.
    bool is_finished;         // Flag which is set when the lexer emits a LXL_TOKENS_END token.
};

// A lexical token.
// The token's value is stored as a string (via the `start` and `end` pointers).
// Further processing of this value is left to the caller.
// The `token_type` determines the type of the token. The meanings of different types
// is left to the caller, but negative types are reserved by lexel and have special
// meanings. For example, a value of -1 (see LXL_TOKENS_END) denotes the end of the
// token stream.
struct lxl_token {
    const char *start;        // The start of the token.
    const char *end;          // The end of the token.
    struct lxl_location loc;  // The location (line, col) of the token in the source.
    int token_type;           // The type of the lexical token. Negative values have special meanings.
};

// END LEXEL CORE.

// LEXEL ADDITIONAL.

// Additional definitions beyond the core above.

// A string view.
// This is a read-only (non-owning) view into a string, consiting of a `start` pointer and `length`.
struct lxl_string_view {
    const char *start;
    size_t length;
};

// END LEXEL ADDITIONAL.


// LEXEL MAGIC VALUES (MVs).

// These enums define human-friendly names for the various magic values used by lexel.

enum lxl__token_mvs {
    LXL_TOKENS_END = -1,
    LXL_TOKEN_UNINIT = -2,
    // See enum lxl_lex_error for token error types.
};

// A string contining all the characters lexel considers whitespace.
#define LXL_WHITESPACE_CHARS " \t\n\r\f\v"

// END LEXEL MAGIC VALUES.


// TOKEN INTERFACE.

// These functions are for working with tokens.

// Return whether `tok` is a special TOKENS_END token.
#define LXL_TOKEN_IS_END(tok) (tok).token_type = LXL_TOKENS_END

// Return the token's value as a string view.
struct lxl_string_view lxl_token_value(struct lxl_token token);

// Return a textual representation of the error code.
const char *lxl_error_message(enum lxl_lex_error error);

// END TOKEN INTERFACE.


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
bool lxl_lexer_is_finished(struct lxl_lexer *lexer);

// Reset the lexer to the start of its input.
void lxl_lexer_reset(struct lxl_lexer *lexer);

// END LEXER EXTERNAL INTERFACE.


// LEXER INTERNAL INTERFACE.

// These functions are used by the lexer to alter its own state.
// They should not be used from a parser, but the interface is exposed to make writing a custom lexer easier.


// Return the number of characters consumed so far.
ptrdiff_t lxl_lexer__head_length(struct lxl_lexer *lexer);
// Return the number of characters left in the lexer's source.
ptrdiff_t lxl_lexer__tail_length(struct lxl_lexer *lexer);
// Return the number of characters consumed after `start_point`.
ptrdiff_t lxl_lexer__length_from(struct lxl_lexer *lexer, const char *start_point);

// Return whether the lexer is at the end of its input.
bool lxl_lexer__is_at_end(struct lxl_lexer *lexer);
// Return the current character and advance the lexer to the next character.
char lxl_lexer__advance(struct lxl_lexer *lexer);
// Advance the lexer by up to n characters and return whether all n characters could be advanced
// (this will be false when there are fewer than n characters left, in which case, the lexer
// reaches the end and stops).
bool lxl_lexer__advance_by(struct lxl_lexer *lexer, size_t n);

// Return whether the current character matches any of those passed, but do not consume it.
bool lxl_lexer__check_chars(struct lxl_lexer *lexer, const char *chars);
// Return whether the next characters match exactly the string passed, but do not consume them.
bool lxl_lexer__check_string(struct lxl_lexer *lexer, const char *s);
// Return whether the next n characters match the first n characters of the string passed,
// but do not consume them.
bool lxl_lexer__check_string_n(struct lxl_lexer *lexer, const char *s, size_t n);
// Return whether the current character is whitespace (see LXL_WHITESPACE_CHARS).
bool lxl_lexer__check_whitespace(struct lxl_lexer *lexer);
// Return whether the current character is a line comment opener.
bool lxl_lexer__check_line_comment(struct lxl_lexer *lexer);

// Return whether the current character matches any of those passed, and consume it if so.
bool lxl_lexer__match_chars(struct lxl_lexer *lexer, const char *chars);
// Return whether the next characters match exactly the string passed, and consume them if so.
bool lxl_lexer__match_string(struct lxl_lexer *lexer, const char *s);
// Return whether the next n characters match the first n characters of the string passed,
// and consume them if so.
bool lxl_lexer__match_string_n(struct lxl_lexer *lexer, const char *s, size_t n);
// Return whether the next characters comprise a line comment, and consume them if so.
bool lxl_lexer__match_line_comment(struct lxl_lexer *lexer);
// Return wheteher the next characters comprise a block comment, and consume them if so.
bool lxl_lexer__match_block_comment(struct lxl_lexer *lexer);
// Return whether the next characters comprise a nestable block comment, and consume them if so.
bool lxl_lexer__match_nestable_comment(struct lxl_lexer *lexer);
// Return whether the next characters comprise an unnestable block comment, and consume them if so.
bool lxl_lexer__match_unnestable_comment(struct lxl_lexer *lexer);

// Advance the lexer past any whitespace characters and return the number of characters consumed.
int lxl_lexer__skip_whitespace(struct lxl_lexer *lexer);
// Advance the lexer past the rest of the current line and return the number of characters consumed.
int lxl_lexer__skip_line(struct lxl_lexer *lexer);
// Advance the lexer past the current (possibly nestable) block comment (opener already consumed)
// and return the number of characters consumed.
int lxl_lexer__skip_block_comment(struct lxl_lexer *lexer, struct delim_pair delims, bool nested);

// Create an unitialised token starting at the lexer's current position.
struct lxl_token lxl_lexer__start_token(struct lxl_lexer *lexer);
// Finish the token ending at the lexer's current position. If an error ocurred during lexing of this
// token, emit an error token instead. The value still includes all the characters lexed.
void lxl_lexer__finish_token(struct lxl_lexer *lexer, struct lxl_token *token);
// Create a special `LXL_TOKENS_END` token at the lexer's current position.
struct lxl_token lxl_lexer__create_end_token(struct lxl_lexer *lexer);
// Create a special error token at the lexer's current position. If the lexer has no error set, use
// LXL_LERR_GENERIC as the error type. Note that this function will not include a value in the token.
// To emit a non-empty error token, use `lxl_lexer__finish_token()` instead.
struct lxl_token lxl_lexer__create_error_token(struct lxl_lexer *lexer);

// Consume all non-whitespace characters and return the number consumed.
int lxl_lexer__lex_symbolic(struct lxl_lexer *lexer);

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
#define LXL_SV_END(sv) ((sv).start + (sv).length)

// Format specifier for printf et al.
#define LXL_SV_FMT_SPEC "%.*s"
// Use when printing a string view with LXL_SV_FMT_SPEC. The argument is evalucated multiple times.
#define LXL_SV_FMT_ARG(sv) ((sv).length < INT_MAX) ? (int)(sv).length : INT_MAX, (sv).start

// END LEXEL STRING VIEW.


// Implementation.

#ifdef LEXEL_IMPLEMENTATION

#include <string.h>

// TOKEN FUNCTIONS.

struct lxl_string_view lxl_token_value(struct lxl_token token) {
    return lxl_sv_from_startend(token.start, token.end);
}

const char *lxl_error_message(enum lxl_lex_error error) {
    switch (error) {
    case LXL_LERR_OK: return "No error";
    case LXL_LERR_GENERIC: return "Generic error";
    case LXL_LERR_EOF: return "Unexpected EOF";
    case LXL_LERR_UNCLOSED_COMMENT: return "Unclosed block comment";
    }
    return NULL;  // Unreachable.
}

// END TOKEN FUNCTIONS.


// LEXER FUNCTIONS.

struct lxl_lexer lxl_lexer_new(const char *start, const char *end) {
    return (struct lxl_lexer) {
        .start = start,
        .end = end,
        .current = start,
        .pos = {0, 0},
        .line_comment_openers = NULL,
        .nestable_comment_delims = NULL,
        .unnestable_comment_delims = NULL,
        .error = LXL_LERR_OK,
        .is_finished = false,
    };
}

struct lxl_lexer lxl_lexer_from_sv(struct lxl_string_view sv) {
    return lxl_lexer_new(sv.start, LXL_SV_END(sv));
}

struct lxl_token lxl_lexer_next_token(struct lxl_lexer *lexer) {
    if (lxl_lexer_is_finished(lexer)) {
        return lxl_lexer__create_end_token(lexer);
    }
    lxl_lexer__skip_whitespace(lexer);
    if (lexer->error) {
        return lxl_lexer__create_error_token(lexer);
    }
    else if (lxl_lexer__is_at_end(lexer)) {
        return lxl_lexer__create_end_token(lexer);
    }
    struct lxl_token token = lxl_lexer__start_token(lexer);
    lxl_lexer__lex_symbolic(lexer);
    lxl_lexer__finish_token(lexer, &token);
    return token;
}

bool lxl_lexer_is_finished(struct lxl_lexer *lexer) {
    return lexer->is_finished;
}

void lxl_lexer_reset(struct lxl_lexer *lexer) {
    lexer->current = lexer->start;
    lexer->is_finished = false;
}

ptrdiff_t lxl_lexer__head_length(struct lxl_lexer *lexer) {
    return lexer->current - lexer->start;
}

ptrdiff_t lxl_lexer__tail_length(struct lxl_lexer *lexer) {
    return lexer->end - lexer->current;
}

ptrdiff_t lxl_lexer__length_from(struct lxl_lexer *lexer, const char *start_point) {
    return lexer->current - start_point;
}

bool lxl_lexer__is_at_end(struct lxl_lexer *lexer) {
    return lexer->current >= lexer->end;
}

char lxl_lexer__advance(struct lxl_lexer *lexer) {
    if (lxl_lexer__is_at_end(lexer)) return '\0';
    ++lexer->current;
    return lexer->current[-1];
}

bool lxl_lexer__advance_by(struct lxl_lexer *lexer, size_t n) {
    size_t tail_length = lxl_lexer__tail_length(lexer);
    if (tail_length < n) {
        lexer->current = lexer->end;
        return false;
    }
    lexer->current += n;
    return true;
}

bool lxl_lexer__check_chars(struct lxl_lexer *lexer, const char *chars) {
    while (*chars != '\0') {
        if (*lexer->current == *chars) return true;
        ++chars;
    }
    return false;
}

bool lxl_lexer__check_string(struct lxl_lexer *lexer, const char *s) {
    size_t tail_length = lxl_lexer__tail_length(lexer);
    size_t n = strlen(s);
    if (n > tail_length) return false;
    return memcmp(lexer->current, s, n) == 0;
}

bool lxl_lexer__check_string_n(struct lxl_lexer *lexer, const char *s, size_t n) {
    size_t tail_length = lxl_lexer__tail_length(lexer);
    if (n < tail_length) n = tail_length;
    return strncmp(lexer->current, s, n) == 0;
}

bool lxl_lexer__check_whitespace(struct lxl_lexer *lexer) {
    return lxl_lexer__check_chars(lexer, LXL_WHITESPACE_CHARS);
}

bool lxl_lexer__check_line_comment(struct lxl_lexer *lexer) {
    if (lexer->line_comment_openers == NULL) return false;
    for (const char *const *opener = lexer->line_comment_openers; *opener != NULL; ++opener) {
        if (lxl_lexer__check_string(lexer, *opener)) {
            return true;
        }
    }
    return false;
}

bool lxl_lexer__match_chars(struct lxl_lexer *lexer, const char *chars) {
    if (lxl_lexer__check_chars(lexer, chars)) {
        return !!lxl_lexer__advance(lexer);
    }
    return false;
}

bool lxl_lexer__match_string(struct lxl_lexer *lexer, const char *s) {
    if (lxl_lexer__check_string(lexer, s)) {
        size_t length = strlen(s);
        return !!lxl_lexer__advance_by(lexer, length);
    }
    return false;
}

bool lxl_lexer__match_string_n(struct lxl_lexer *lexer, const char *s, size_t n) {
    if (lxl_lexer__check_string_n(lexer, s, n)) {
        return !!lxl_lexer__advance_by(lexer, n);
    }
    return false;
}

bool lxl_lexer__match_line_comment(struct lxl_lexer *lexer) {
    if (!lxl_lexer__check_line_comment(lexer)) return false;
    lxl_lexer__skip_line(lexer);
    return true;
}

bool lxl_lexer__match_block_comment(struct lxl_lexer *lexer) {
    if (lxl_lexer__match_nestable_comment(lexer)) return true;
    return lxl_lexer__match_unnestable_comment(lexer);
}

bool lxl_lexer__match_nestable_comment(struct lxl_lexer *lexer) {
    if (lexer->nestable_comment_delims == NULL) return false;
    for (const struct delim_pair *delims = lexer->nestable_comment_delims;
         delims->opener != NULL;
         ++delims) {
        if (lxl_lexer__match_string(lexer, delims->opener)) {
            lxl_lexer__skip_block_comment(lexer, *delims, true);
            return true;
        }
    }
    return false;
}

bool lxl_lexer__match_unnestable_comment(struct lxl_lexer *lexer) {
    if (lexer->unnestable_comment_delims == NULL) return false;
    for (const struct delim_pair *delims = lexer->unnestable_comment_delims;
         delims->opener != NULL;
         ++delims) {
        if (lxl_lexer__match_string(lexer, delims->opener)) {
            lxl_lexer__skip_block_comment(lexer, *delims, false);
            return true;
        }
    }
    return false;
}

int lxl_lexer__skip_whitespace(struct lxl_lexer *lexer) {
    const char *whitespace_start = lexer->current;
    for(;;) {
        if (lxl_lexer__check_whitespace(lexer)) {
            // Whitespace, skip.
            if (!lxl_lexer__advance(lexer)) break;
        }
        else if (lxl_lexer__match_line_comment(lexer)) {
            /* Do nothing; comment already consumed. */
        }
        else if (lxl_lexer__match_block_comment(lexer)) {
            /* Do nothing; comment already consumed. */
        }
        else {
            // Not a comment or whitespace.
            break;
        }
    }
    return lxl_lexer__length_from(lexer, whitespace_start);
}

int lxl_lexer__skip_line(struct lxl_lexer *lexer) {
    const char *line_start = lexer->current;
    // NOTE: Use `match()` to consume the final `\n` along with the comment.
    while (!lxl_lexer__match_chars(lexer, "\n")) {
        if (!lxl_lexer__advance(lexer)) break;
    }
    return lxl_lexer__length_from(lexer, line_start);
}

int lxl_lexer__skip_block_comment(struct lxl_lexer *lexer, struct delim_pair delims, bool nestable) {
    const char *comment_start = lexer->start;
    while (!lxl_lexer__match_string(lexer, delims.closer)) {
        if (nestable && lxl_lexer__match_string(lexer, delims.opener)) {
            if (lxl_lexer__skip_block_comment(lexer, delims, true) <= 0) {
                // Unclosed nested comment.
                lexer->error = LXL_LERR_UNCLOSED_COMMENT;
                break;
            }
        }
        else {
            if (!lxl_lexer__advance(lexer)) {
                lexer->error = LXL_LERR_UNCLOSED_COMMENT;
                break;
            }
        }
    }
    return lxl_lexer__length_from(lexer, comment_start);
}

struct lxl_token lxl_lexer__start_token(struct lxl_lexer *lexer) {
    return (struct lxl_token) {
        .start = lexer->current,
        .end = lexer->current,
        .loc = lexer->pos,
        .token_type = LXL_TOKEN_UNINIT,
    };
}

void lxl_lexer__finish_token(struct lxl_lexer *lexer, struct lxl_token *token) {
    token->end = lexer->current;
    if (lexer->error) {
        token->token_type = lexer->error;  // Set error as token type.
        lexer->error = LXL_LERR_OK;  // Clear error.
    }
}

struct lxl_token lxl_lexer__create_end_token(struct lxl_lexer *lexer) {
    struct lxl_token token = lxl_lexer__start_token(lexer);
    token.token_type = LXL_TOKENS_END;
    lexer->is_finished = true;
    return token;
}

struct lxl_token lxl_lexer__create_error_token(struct lxl_lexer *lexer) {
    struct lxl_token token = lxl_lexer__start_token(lexer);
    if (!lexer->error) lexer->error = LXL_LERR_GENERIC;  // Emit a generic error token if no error is set.
    lxl_lexer__finish_token(lexer, &token);  // This function handles setting the error type.
    return token;
}

int lxl_lexer__lex_symbolic(struct lxl_lexer *lexer) {
    int count = 0;
    while (!lxl_lexer__is_at_end(lexer) && !lxl_lexer__check_whitespace(lexer)) {
        lxl_lexer__advance(lexer);
        ++count;
    }
    return count;
}

// END LEXER FUNCTIONS.

// STRING VIEW FUNCTIONS.

struct lxl_string_view lxl_sv_from_string(const char *s) {
    return (struct lxl_string_view) {.start = s, .length = strlen(s)};
}

struct lxl_string_view lxl_sv_from_startend(const char *start, const char *end) {
    return (struct lxl_string_view) {.start = start, .length = end - start};
}

// END STRING VIEW FUNCTIONS.

#undef LEXEL_IMPLEMENTATION
#endif  // LEXEL_IMPLEMENTATION

#endif  // LEXEL_H

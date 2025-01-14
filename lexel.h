#ifndef LEXEL_H
#define LEXEL_H

#include <stdbool.h>  // bool, false, true -- requires C99
#include <stddef.h>   // size_t, ptrdiff_t
#include <limits.h>   // INT_MAX

// CUSTOMISATION OPTIONS.

// These options customise certain behaviours of lexel.
// They're defined at the top of the file for visibility.

// Customisable options can be given custom definitions before including lexel.h.


// LEXEL_IMPLEMENTATION enables implementation of lexel functions. It should be defined at most ONCE.

// LXL_NO_ASSERT disables lexel library assertions. This setting is independant of NDEBUG.


// This option controls which macro should be used for lexel library assertions.
// The default value is the standard assert() macro.
// NOTE: this macro should not be customised to merely disable assertions. To do that, use LXL_NO_ASSERT.
#ifndef LXL_ASSERT_MACRO
 #include <assert.h>
 #define LXL_ASSERT_MACRO assert
#endif

// END CUSTOMISATION OPTIONS.

// META-DEFINITIONS.

// These definitions are not part of the lexel interface per se, but have special signficance within this file.

// LXL_ASSERT() is lexel's library assertion macro. It should not be directly customised.
// Use LXL_ASSERT_MACRO and LXL_NO_ASSERT to cusomise instead.
#ifndef LXL_NO_ASSERT
 #define LXL_ASSERT(...) LXL_ASSERT_MACRO(__VA_ARGS__)
#else  // Disable library assertions.
 #define LXL_ASSERT(...) ((void)0)
#endif

// This marks a location in code as logically unreachable.
// If the assertion fires, it suggests a bug in lexel itself.
#define LXL_UNREACHABLE() LXL_ASSERT(0 && "Unreachable. This may be a bug in lexel.")

// END META-DEFINITIONS.

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
    LXL_LERR_UNCLOSED_STRING = -19,   // A string-like literal had no closing delimiter before the end.
    LXL_LERR_INVALID_INTEGER = -20,   // An integer literal was invalid (e.g. had a prefix but no payload).
};

// Lexer status.
enum lxl_lexer_status {
    LXL_LSTS_READY,              // Ready to lex next token.
    LXL_LSTS_LEXING,             // In the process of lexing a token.
    LXL_LSTS_FINISHED,           // Reached the end of tokens.
    LXL_LSTS_FINISHED_ABNORMAL,  // Reached the end of tokens abnormally.
};

enum lxl_word_lexing_rule {
    LXL_LEX_SYMBOLIC,  // Lex all symbolic characters (any non-whitespace).
    LXL_LEX_WORD,      // Lex only word characters (any non-reserved symbolic).
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
    struct lxl_location pos;  // The current position (line, column) in the source.
    const char *const *line_comment_openers;            // List of line comment openers.
    const struct delim_pair *nestable_comment_delims;   // List of paired nestable block comment delimiters.
    const struct delim_pair *unnestable_comment_delims; // List of paired unnestable block comment delimiters.
    const char *string_delims;       // List of matched string (or character) literal delimiters.
    const char *string_escape_chars; // List of escape characters in strings (ignore delimiters after).
    const int *string_types;         // List of token types associated with each string delimiter.
    const char *digit_separators;    // List of digit separator characters allowed in number literals.
    const char *const *number_signs;      // List of signs which can precede number literals (e.g. "+", "-").
    const char *const *integer_prefixes;  // List of prefixes for integer literals.
    int *integer_bases;                   // List of bases associated with each prefix.
    const char *const *integer_suffixes;  // List of suffixes for integer literals.
    int default_int_type;     // Default token type for integer literals.
    int default_int_base;     // Default base for (unprefixed) integer literals.
    const char *const *puncts;   // List of (non-word) punctaution token values (e.g., "+", "==", ";", etc.).
    int *punct_types;            // List of token types corresponding to each punctuation token above.
    const char *const *keywords; // List of keywords (word tokens with unique types).
    int *keyword_types;          // List of token types corresponding to each keyword.
    int default_word_type;       // Default word token type (for non-keywords).
    enum lxl_word_lexing_rule word_lexing_rule;  // The word lexing rule to use (default: symbolic).
    enum lxl_lex_error error;      // Error code set to the current lexing error.
    enum lxl_lexer_status status;  // Current status of the lexer.
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
    struct lxl_location loc;  // The location (line, column) of the token in the source.
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
    LXL_TOKENS_END_ABNORMAL = -3,
    // See enum lxl_lex_error for token error types.
};

// A string contining all the characters lexel considers whitespace.
#define LXL_WHITESPACE_CHARS " \t\n\r\f\v"

// A string containing the basic uppercase latin alphabet in order.
#define LXL_BASIC_UPPER_LATIN_CHARS "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
// A string containing the basic lowercase latin alphabet in order.
#define LXL_BASIC_LOWER_LATIN_CHARS "abcdefghijklmnopqrstuvwxyz"
// A string containing the basic mixed case latin alphabet in alphabetical order.
#define LXL_BASIC_MIXED_LATIN_CHARS "AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz"
// A string containing the digits 0--9 in order.
#define LXL_DIGITS "0123456789"

// END LEXEL MAGIC VALUES.


// TOKEN INTERFACE.

// These functions are for working with tokens.

// Return whether `tok` is a special end-of-tokens token.
#define LXL_TOKEN_IS_END(tok) \
    ((tok).token_type == LXL_TOKENS_END || (tok).token_type == LXL_TOKENS_END_ABNORMAL)

// Return whetehr `tok` is a special error token.
#define LXL_TOKEN_IS_ERROR(tok) ((tok).token_type <= LXL_LERR_GENERIC)

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
// Return whether the lexer is at the start of its input.
bool lxl_lexer__is_at_start(struct lxl_lexer *lexer);
// Return the current character and advance the lexer to the next character.
char lxl_lexer__advance(struct lxl_lexer *lexer);
// Advance the lexer by up to n characters and return whether all n characters could be advanced
// (this will be false when there are fewer than n characters left, in which case, the lexer
// reaches the end and stops).
bool lxl_lexer__advance_by(struct lxl_lexer *lexer, size_t n);
// Rewind the lexer to the previous character and return whether the rewind was successful (the lexer
// cannot be rewound beyond its starting point).
bool lxl_lexer__rewind(struct lxl_lexer *lexer);
// Rewind the lexer by up to n characters and return whether all n chracters could be rewound.
bool lxl_lexer__rewind_by(struct lxl_lexer *lexer, size_t n);

// Recalculate the current column in the lexer.
void lxl_lexer__recalc_column(struct lxl_lexer *lexer);

// Return non-NULL if the current current matches any of those passed but do not consume it, otherwise,
// return NULL. On success, the return value is the pointer to the matching character, i.e., into the
// null-terminated string `chars`.
const char *lxl_lexer__check_chars(struct lxl_lexer *lexer, const char *chars);
// Return whether the next characters match exactly the string passed, but do not consume them.
bool lxl_lexer__check_string(struct lxl_lexer *lexer, const char *s);
// Return whether the next n characters match the first n characters of the string passed,
// but do not consume them.
bool lxl_lexer__check_string_n(struct lxl_lexer *lexer, const char *s, size_t n);
// Return whether the current character is whitespace (see LXL_WHITESPACE_CHARS).
bool lxl_lexer__check_whitespace(struct lxl_lexer *lexer);
// Return whether the current characer is reserved (has a special meaning, like starting a comment or string).
bool lxl_lexer__check_reserved(struct lxl_lexer *lexer);
// Return whether the current character is a line comment opener.
bool lxl_lexer__check_line_comment(struct lxl_lexer *lexer);
// Return whether the next characters comprise a block comment.
bool lxl_lexer__check_block_comment(struct lxl_lexer *lexer);
// Return whether the next characters comprise a nestable block comment.
bool lxl_lexer__check_nestable_comment(struct lxl_lexer *lexer);
// Return whether the next charactrs comprise an unnestable block comment.
bool lxl_lexer__check_unnestable_comment(struct lxl_lexer *lexer);
// Return non-NULL if the current character matches one of the lexer's string delimiters but do not consume
// it, otherwise, return NULL. On success, the return value is the pointer to the matching delimiter.
const char *lxl_lexer__check_string_delim(struct lxl_lexer *lexer);
// Return whether the current character is digit of the specified base but do not consume it. The base is
// an integer in the range  2--36 (inclusive). For bases 11+, the letters a--z (case-insensitive) are
// used for digit values 10+ (as in hexadecimal).
bool lxl_lexer__check_digit(struct lxl_lexer *lexer, int base);
// Return whether the current character is a digit separator but do not consume it.
bool lxl_lexer__check_digit_separator(struct lxl_lexer *lexer);
// Return whether the current character is a digit (see above) or digit separator but do not consume it.
bool lxl_lexer__check_digit_or_separator(struct lxl_lexer *lexer, int base);
// Return non-zero if the next characters comprise an integer literal prefix but do not consume them. The
// return value is the base corresponding to the matched prefix.
int lxl_lexer__check_int_prefix(struct lxl_lexer *lexer);
// Return whether the next characters comprise an integer literal suffix but do not consume them.
bool lxl_lexer__check_int_suffix(struct lxl_lexer *lexer);
// Return whether the next characters comprise a number literal sign (e.g. +, -) but do not consume them.
bool lxl_lexer__check_number_sign(struct lxl_lexer *lexer);
// Return non-NULL if the next characters comprise an punct but do not consume them, otherwise,
// return NULL. On success, the return value is the pointer to the matching punct in the .puncts list.
const char *const *lxl_lexer__check_punct(struct lxl_lexer *lexer);

// Return non-NULL if the current current matches any of those passed and consume it if so, otherwise,
// return NULL. On success, the return value is the pointer to the matching character, i.e., into the
// null-terminated string `chars`.
const char *lxl_lexer__match_chars(struct lxl_lexer *lexer, const char *chars);
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
// Return non-NULL if the current character matches one of the lexer's string delimiters and consume it
// if so, otherwise, return NULL. On success, the return value is the pointer to the matching delimiter.
const char *lxl_lexer__match_string_delim(struct lxl_lexer *lexer);
// Return whether the current character is digit of the specified base, and consume it if so. The base is
// an integer in the range  2--36 (inclusive). For bases 11+, the letters a--z (case-insensitive) are
// used for digit values 10+ (as in hexadecimal).
bool lxl_lexer__match_digit(struct lxl_lexer *lexer, int base);
// Return whether the current character is a digit separator but do not consume it.
bool lxl_lexer__match_digit_separator(struct lxl_lexer *lexer);
// Return whether the current character is a digit (see above) or digit separator, and consume it if so.
bool lxl_lexer__match_digit_or_separator(struct lxl_lexer *lexer, int base);
// Return non-zero if the next characters comprise an integer literal prefix, and consume them if so. The
// return value is the base corresponding to the matched prefix.
int lxl_lexer__match_int_prefix(struct lxl_lexer *lexer);
// Return whether the next characters comprise an integer literal suffix, and consume them if so.
bool lxl_lexer__match_int_suffix(struct lxl_lexer *lexer);
// Return whether the next characters comprise a number literal sign (e.g. +, -), and consume them if so.
bool lxl_lexer__match_number_sign(struct lxl_lexer *lexer);
// Return non-NULL if the next characters comprise an punct and consume them if so, otherwise,
// return NULL. On success, the return value is the pointer to the matching punct in the .puncts list.
const char *const *lxl_lexer__match_punct(struct lxl_lexer *lexer);

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
// Consume a word token (non-reserved symbolic) and return the number of characters read.
int lxl_lexer__lex_word(struct lxl_lexer *lexer);
// Consume a string-like token delimited by `delim` and return the number of characters read.
int lxl_lexer__lex_string(struct lxl_lexer *lexer, char delim);
// Consume the digits of an integer literal in the given base (2--36).
int lxl_lexer__lex_integer(struct lxl_lexer *lexer, int base);

// Get the token type corresponding to the word specified.
int lxl_lexer__get_word_type(struct lxl_lexer *lexer, const char *word_start);

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
    case LXL_LERR_UNCLOSED_STRING: return "Unclosed string-like literal";
    case LXL_LERR_INVALID_INTEGER: return "Inavlid integer";
    }
    LXL_UNREACHABLE();
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
        .string_delims = NULL,
        .string_escape_chars = NULL,
        .number_signs = NULL,
        .digit_separators = NULL,
        .integer_prefixes = NULL,
        .integer_bases = NULL,
        .integer_suffixes = NULL,
        .default_int_type = LXL_LERR_GENERIC,
        .default_int_base = 0,
        .puncts = NULL,
        .punct_types = NULL,
        .keywords = NULL,
        .keyword_types = NULL,
        .default_word_type = LXL_TOKEN_UNINIT,
        .word_lexing_rule = LXL_LEX_SYMBOLIC,
        .error = LXL_LERR_OK,
        .status = LXL_LSTS_READY,
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
    const char *matched_char = NULL;
    const char *const *matched_string = NULL;
    int int_base = 0;
    if ((matched_char = lxl_lexer__match_string_delim(lexer))) {
        lxl_lexer__lex_string(lexer, *matched_char);
        int delim_index = matched_char - lexer->string_delims;
        token.token_type = lexer->string_types[delim_index];
    }
    else if ((int_base = lxl_lexer__match_int_prefix(lexer))) {
        int digit_count = lxl_lexer__lex_integer(lexer, int_base);
        token.token_type = (digit_count > 0) ? lexer->default_int_type : LXL_LERR_INVALID_INTEGER;
        lxl_lexer__match_int_suffix(lexer);
    }
    else if ((matched_string = lxl_lexer__match_punct(lexer))) {
        int punct_index = matched_string - lexer->puncts;
        LXL_ASSERT(lexer->punct_types != NULL);
        token.token_type = lexer->punct_types[punct_index];
    }
    else {
        switch (lexer->word_lexing_rule) {
        case LXL_LEX_SYMBOLIC:
            lxl_lexer__lex_symbolic(lexer);
            break;
        case LXL_LEX_WORD:
            lxl_lexer__lex_word(lexer);
            break;
        }
        token.token_type = lxl_lexer__get_word_type(lexer, token.start);
    }
    lxl_lexer__finish_token(lexer, &token);
    return token;
}

bool lxl_lexer_is_finished(struct lxl_lexer *lexer) {
    return lexer->status == LXL_LSTS_FINISHED || lexer->status == LXL_LSTS_FINISHED_ABNORMAL;
}

void lxl_lexer_reset(struct lxl_lexer *lexer) {
    lexer->current = lexer->start;
    lexer->status = LXL_LSTS_READY;
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

bool lxl_lexer__is_at_start(struct lxl_lexer *lexer) {
    return lexer->current <= lexer->start;
}

char lxl_lexer__advance(struct lxl_lexer *lexer) {
    if (lxl_lexer__is_at_end(lexer)) return '\0';
    if (*lexer->current != '\n') {
        ++lexer->pos.column;
    }
    else {
        lexer->pos.column = 0;
        ++lexer->pos.line;
    }
    ++lexer->current;
    return lexer->current[-1];
}

bool lxl_lexer__advance_by(struct lxl_lexer *lexer, size_t n) {
    while (n-- > 0) {
        if (lxl_lexer__is_at_end(lexer)) return false;
        ++lexer->current;
        if (*lexer->current == '\n') {
            ++lexer->pos.line;
        }
    }
    lxl_lexer__recalc_column(lexer);
    return true;
}

bool lxl_lexer__rewind(struct lxl_lexer *lexer) {
    if (lxl_lexer__is_at_start(lexer)) return false;
    --lexer->current;
    if (*lexer->current != '\n') {
        --lexer->pos.column;
    }
    else {
        lxl_lexer__recalc_column(lexer);
        --lexer->pos.line;
    }
    return true;
}

bool lxl_lexer__rewind_by(struct lxl_lexer *lexer, size_t n) {
    while (n-- > 0) {
        if (lxl_lexer__is_at_start(lexer)) return false;
        --lexer->current;
        if (*lexer->current == '\n') {
            --lexer->pos.line;
        }
    }
    lxl_lexer__recalc_column(lexer);
    return true;
}

void lxl_lexer__recalc_column(struct lxl_lexer *lexer) {
    lexer->pos.column = 0;
    for (const char *p = lexer->current; p != lexer->start && *p != '\n'; --p) {
        ++lexer->pos.column;
    }
}

const char *lxl_lexer__check_chars(struct lxl_lexer *lexer, const char *chars) {
    while (*chars != '\0') {
        if (*lexer->current == *chars) return chars;
        ++chars;
    }
    return NULL;
}

bool lxl_lexer__check_string(struct lxl_lexer *lexer, const char *s) {
    size_t tail_length = lxl_lexer__tail_length(lexer);
    size_t n = strlen(s);
    if (n > tail_length) return false;
    return memcmp(lexer->current, s, n) == 0;
}

bool lxl_lexer__check_string_n(struct lxl_lexer *lexer, const char *s, size_t n) {
    size_t tail_length = lxl_lexer__tail_length(lexer);
    if (n > tail_length) n = tail_length;
    return strncmp(lexer->current, s, n) == 0;
}

bool lxl_lexer__check_whitespace(struct lxl_lexer *lexer) {
    return lxl_lexer__check_chars(lexer, LXL_WHITESPACE_CHARS);
}

bool lxl_lexer__check_reserved(struct lxl_lexer *lexer) {
    return lxl_lexer__check_whitespace(lexer)
        || lxl_lexer__check_line_comment(lexer)
        || lxl_lexer__check_block_comment(lexer)
        || !!lxl_lexer__check_string_delim(lexer)
        || !!lxl_lexer__check_punct(lexer)
        ;
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

bool lxl_lexer__check_block_comment(struct lxl_lexer *lexer) {
    return lxl_lexer__check_nestable_comment(lexer) || lxl_lexer__check_unnestable_comment(lexer);
}

bool lxl_lexer__check_nestable_comment(struct lxl_lexer *lexer) {
    if (lexer->nestable_comment_delims == NULL) return false;
    for (const struct delim_pair *delims = lexer->nestable_comment_delims;
         delims->opener != NULL;
         ++delims) {
        if (lxl_lexer__check_string(lexer, delims->opener)) return true;
    }
    return false;
}

bool lxl_lexer__check_unnestable_comment(struct lxl_lexer *lexer) {
    if (lexer->unnestable_comment_delims == NULL) return false;
    for (const struct delim_pair *delims = lexer->unnestable_comment_delims;
         delims->opener != NULL;
         ++delims) {
        if (lxl_lexer__check_string(lexer, delims->opener)) return true;
    }
    return false;
}

const char *lxl_lexer__check_string_delim(struct lxl_lexer *lexer) {
    if (lexer->string_delims == NULL) return NULL;
    return lxl_lexer__check_chars(lexer, lexer->string_delims);
}

bool lxl_lexer__check_digit(struct lxl_lexer *lexer, int base) {
    if (base == 0) return false;
    LXL_ASSERT(2 <= base && base <= 26);
    char digits[] = LXL_DIGITS "" LXL_BASIC_MIXED_LATIN_CHARS;
    int end_digit_index = (base <= 10) ? base : 10 + 2*(base - 10);
    digits[end_digit_index] = '\0';  // Truncate array to only contain the needed digits.
    return lxl_lexer__check_chars(lexer, digits);
}

bool lxl_lexer__check_digit_separator(struct lxl_lexer *lexer) {
    if (lexer->digit_separators == NULL) return false;
    return lxl_lexer__check_chars(lexer, lexer->digit_separators);
}

bool lxl_lexer__check_digit_or_separator(struct lxl_lexer *lexer, int base) {
    return lxl_lexer__check_digit(lexer, base) || lxl_lexer__check_digit_separator(lexer);
}

int lxl_lexer__check_int_prefix(struct lxl_lexer *lexer) {
    const char *start = lexer->current;
    // Consume any leading sign to make detecting the prefix easier.
    // We'll rewind the lexer before returning.
    lxl_lexer__match_number_sign(lexer);
    if (lexer->integer_prefixes != NULL) {
        for (int i = 0; lexer->integer_prefixes[i] != NULL; ++i) {
            if (lxl_lexer__check_string(lexer, lexer->integer_prefixes[i])) {
                lexer->current = start;
                return lexer->integer_bases[i];
            }
        }
    }
    lexer->current = start;
    return (lxl_lexer__check_digit(lexer, lexer->default_int_base)) ? lexer->default_int_base : 0;
}

bool lxl_lexer__check_int_suffix(struct lxl_lexer *lexer) {
    if (lexer->integer_suffixes == NULL) return false;
    for (int i = 0; lexer->integer_suffixes[i] != NULL; ++i) {
        if (lxl_lexer__check_string(lexer, lexer->integer_suffixes[i])) return true;
    }
    return false;
}

const char *const *lxl_lexer__check_punct(struct lxl_lexer *lexer) {
    if (lexer->puncts == NULL) return NULL;
    for (const char *const *punct = lexer->puncts; *punct != NULL; ++punct) {
        if (lxl_lexer__check_string(lexer, *punct)) return punct;
    }
    return NULL;
}

const char *lxl_lexer__match_chars(struct lxl_lexer *lexer, const char *chars) {
    const char *p = lxl_lexer__check_chars(lexer, chars);
    if (p != NULL) {
        lxl_lexer__advance(lexer);
    }
    return p;
}

bool lxl_lexer__match_string(struct lxl_lexer *lexer, const char *s) {
    if (lxl_lexer__check_string(lexer, s)) {
        size_t length = strlen(s);
        return lxl_lexer__advance_by(lexer, length);
    }
    return false;
}

bool lxl_lexer__match_string_n(struct lxl_lexer *lexer, const char *s, size_t n) {
    if (lxl_lexer__check_string_n(lexer, s, n)) {
        return lxl_lexer__advance_by(lexer, n);
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

const char *lxl_lexer__match_string_delim(struct lxl_lexer *lexer) {
    if (lexer->string_delims == NULL) return NULL;
    return lxl_lexer__match_chars(lexer, lexer->string_delims);
}

bool lxl_lexer__match_digit(struct lxl_lexer *lexer, int base) {
    if (!lxl_lexer__check_digit(lexer, base)) return false;
    return lxl_lexer__advance(lexer);
}

bool lxl_lexer__match_digit_separator(struct lxl_lexer *lexer) {
    if (!lxl_lexer__check_digit_separator(lexer)) return false;
    return lxl_lexer__advance(lexer);
}

bool lxl_lexer__match_digit_or_separator(struct lxl_lexer *lexer, int base) {
    if (!lxl_lexer__check_digit_or_separator(lexer, base)) return false;
    return lxl_lexer__advance(lexer);
}

int lxl_lexer__match_int_prefix(struct lxl_lexer *lexer) {
    lxl_lexer__match_number_sign(lexer);
    if (lexer->integer_prefixes != NULL) {
        for (int i = 0; lexer->integer_prefixes[i] != NULL; ++i) {
            if (lxl_lexer__match_string(lexer, lexer->integer_prefixes[i])) {
                return lexer->integer_bases[i];
            }
        }
    }
    return (lxl_lexer__check_digit(lexer, lexer->default_int_base)) ? lexer->default_int_base : 0;
}

bool lxl_lexer__match_number_sign(struct lxl_lexer *lexer) {
    if (lexer->number_signs == NULL) return false;
    for (const char *const *sign = lexer->number_signs; *sign != NULL; ++sign) {
        if (lxl_lexer__match_string(lexer, *sign)) return true;
    }
    return false;
}

bool lxl_lexer__match_int_suffix(struct lxl_lexer *lexer) {
    if (lexer->integer_suffixes == NULL) return false;
    for (int i = 0; lexer->integer_suffixes[i] != NULL; ++i) {
        if (lxl_lexer__match_string(lexer, lexer->integer_suffixes[i])) return true;
    }
    return false;
}

const char *const *lxl_lexer__match_punct(struct lxl_lexer *lexer) {
    if (lexer->puncts == NULL) return NULL;
    for (const char *const *punct = lexer->puncts; *punct != NULL; ++punct) {
        if (lxl_lexer__match_string(lexer, *punct)) return punct;
    }
    return NULL;
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
    if (lexer->status == LXL_LSTS_READY) lexer->status = LXL_LSTS_LEXING;
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
    if (lexer->status == LXL_LSTS_LEXING) lexer->status = LXL_LSTS_READY;  // Ready for the next token.
}

struct lxl_token lxl_lexer__create_end_token(struct lxl_lexer *lexer) {
    struct lxl_token token = lxl_lexer__start_token(lexer);
    if (lexer->status != LXL_LSTS_FINISHED_ABNORMAL) {
        token.token_type = LXL_TOKENS_END;
        lexer->status = LXL_LSTS_FINISHED;
    }
    else {
        token.token_type = LXL_TOKENS_END_ABNORMAL;
    }
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

int lxl_lexer__lex_word(struct lxl_lexer *lexer) {
    int count = 0;
    while (!lxl_lexer__is_at_end(lexer) && !lxl_lexer__check_reserved(lexer)) {
        lxl_lexer__advance(lexer);
        ++count;
    }
    return count;
}

int lxl_lexer__lex_string(struct lxl_lexer *lexer, char delim) {
    if (lexer->string_delims == NULL) return 0;  // No strings.
    const char *start = lexer->current;
    while (!lxl_lexer__match_string_n(lexer, &delim, 1)) {
        lxl_lexer__match_chars(lexer, lexer->string_escape_chars);  // Consume escape character.
        // Consume non-delimiter character or escaped delimiter.
        if (!lxl_lexer__advance(lexer)) {
            lexer->error = LXL_LERR_UNCLOSED_STRING;
            return lxl_lexer__length_from(lexer, start);
        }
    }
    return lxl_lexer__length_from(lexer, start);
}

int lxl_lexer__lex_integer(struct lxl_lexer *lexer, int base) {
    const char *start = lexer->current;
    int digit_count = 0;
    for (;;) {
        if (lxl_lexer__match_digit(lexer, base)) {
            ++digit_count;
        }
        else if (lxl_lexer__match_digit_separator(lexer)) {
            /* Do nothing. */
        }
        else {
            // Not a digit or separator.
            break;
        }
    }
    if (digit_count <= 0) {
        // Un-lex token which is not a valid integer literal.
        lexer->current = start;
    }
    return lxl_lexer__length_from(lexer, start);
}

int lxl_lexer__get_word_type(struct lxl_lexer *lexer, const char *word_start) {
    if (lexer->keywords == NULL) return lexer->default_word_type;
    LXL_ASSERT(lexer->keyword_types != NULL);
    ptrdiff_t word_length = lxl_lexer__length_from(lexer, word_start);
    LXL_ASSERT(word_length > 0);  // Length = 0 is invalid.
    for (int i = 0; lexer->keywords[i] != NULL; ++i) {
        const char *keyword = lexer->keywords[i];
        size_t keyword_length = strlen(keyword);
        if (keyword_length != (size_t)word_length) continue;  // No match.
        if (memcmp(word_start, keyword, keyword_length) == 0) {
            return lexer->keyword_types[i];
        }
    }
    return lexer->default_word_type;
}

// END LEXER FUNCTIONS.

// STRING VIEW FUNCTIONS.

struct lxl_string_view lxl_sv_from_string(const char *s) {
    return (struct lxl_string_view) {.start = s, .length = strlen(s)};
}

struct lxl_string_view lxl_sv_from_startend(const char *start, const char *end) {
    LXL_ASSERT(start <= end);
    return (struct lxl_string_view) {.start = start, .length = end - start};
}

// END STRING VIEW FUNCTIONS.

#endif  // LEXEL_IMPLEMENTATION

#endif  // LEXEL_H

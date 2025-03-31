/*
 * Lexel -- a simple, general purpose lexing library in C.
 *
 * Lexel is a single-header ibrary; this file comprises the entirety of the library.
 * To include with function definitions:
 * + #define LEXEL_IMPLEMENTATION
 *   #include "lexel.h"
 * To include only function/type declarations:
 *   // LEXEL_IMPLEMENTATION not defined.
 *   #include "lexel.h"
 *
 * MIT License
 *
 * Copyright (c) 2024 Ninesquared81
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#ifndef LEXEL_H
#define LEXEL_H

#include <assert.h>      // assert(), static_assert()  -- requires C11
#include <limits.h>      // INT_MAX
#include <stdalign.h>    // alignof (C11) -- requires C11
#include <stdarg.h>      // va_list et al.
#include <stdbool.h>     // bool, false, true -- requires C99
#include <stddef.h>      // size_t, ptrdiff_t, max_align_t
#include <stdint.h>      // intptr_t

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
# define LXL_ASSERT_MACRO assert
#endif

#ifndef LXL_REGION_ALIGN
# define LXL_REGION_ALIGN alignof(max_align_t)
#endif

static_assert(((LXL_REGION_ALIGN) & ((LXL_REGION_ALIGN)-1)) == 0, "Alignment must be a power of 2");

// END CUSTOMISATION OPTIONS.

// META-DEFINITIONS.

// These definitions are not part of the lexel interface per se, but have special signficance within this file.

// LXL_ASSERT() is lexel's library assertion macro. It should not be directly customised.
// Use LXL_ASSERT_MACRO and LXL_NO_ASSERT to cusomise instead.
#ifndef LXL_NO_ASSERT
# define LXL_ASSERT(...) LXL_ASSERT_MACRO(__VA_ARGS__)
#else  // Disable library assertions.
# define LXL_ASSERT(...) ((void)0)
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
    LXL_LERR_INVALID_FLOAT = -21,     // A floating-point literal was invalid.
};

// Lexer status.
enum lxl_lexer_status {
    LXL_LSTS_READY,                // Ready to lex next token.
    LXL_LSTS_SKIPPING_WHITESPACE,  // Skipping whitespace (and comments).
    LXL_LSTS_DECIDING_TYPE,        // Deciding the type of the token.
    LXL_LSTS_LEXING_STRING,        // Lexing a string-like literal token.
    LXL_LSTS_LEXING_INTEGER,       // Lexing an integer literal token.
    LXL_LSTS_LEXING_FLOAT,         // Lexing a floating-point literal token.
    LXL_LSTS_LEXING_WORD,          // Lexing a word token.
    LXL_LSTS_TOKEN_LEXED,          // Token has been fully lexed.
    LXL_LSTS_FINISHED,             // Reached the end of tokens.
    LXL_LSTS_FINISHED_ABNORMAL,    // Reached the end of tokens abnormally.
};

enum lxl_word_lexing_rule {
    LXL_LEX_SYMBOLIC,  // Lex all symbolic characters (any non-whitespace).
    LXL_LEX_WORD,      // Lex only word characters (any non-reserved symbolic).
};

// A pair of delimiters for strings and block comments, e.g. "/*" and "*/" for C-style comments.
struct lxl_delim_pair {
    const char *opener;
    const char *closer;
};

// Whether a string should be lexed as single line or multiline.
// Used as an argument of `lxl_lexer__lex_string()`
enum lxl_string_type {
    LXL_STRING_LINE,
    LXL_STRING_MULTILINE,
};

// A lexical token.
// The token's value is stored as a string (via the `start` and `end` pointers).
// Further processing of this value is left to the caller.
// The `kind` determines the type of the token. The meanings of different types
// is left to the caller, but negative types are reserved by lexel and have special
// meanings. For example, a value of -1 (see LXL_TOKENS_END) denotes the end of the
// token stream.
struct lxl_token {
    const char *start;        // The start of the token.
    const char *end;          // The end of the token.
    struct lxl_location loc;  // The location (line, column) of the token in the source.
    int kind;           // The type of the lexical token. Negative values have special meanings.
};

// The main lexer object.
struct lxl_lexer {
    // Lexer state.
    enum lxl_lexer_status status; // Current status of the lexer.
    enum lxl_lex_error error;     // Error code set to the current lexing error.
    const char *start;            // The start of the lexer's source code.
    const char *end;              // The end of the lexer's source code.
    const char *current;          // Pointer to the current character.
    struct lxl_location pos;      // The current position (line, column) in the source.
    struct lxl_token token;  // The next token to be emitted.
    // Customisation flags.
    bool emit_line_endings;       // Should line endings have their own tokens? (default: false)
    bool collect_line_endings;    // Should consecutive line ending tokens be combined? (default: true)
    // Query functions (optional; can be left NULL if not needed).
    bool (*match_whitespace_char)(struct lxl_lexer *self);  // Consume a single whitespace character.
    // --- Default behaviour: return true if next character is in `LXL_WHITESPACE_CHARS`.
    bool (*match_line_comment)(struct lxl_lexer *self);  // Consume the start delimiter of a line comment.
    // --- Default behaviour: always return false.
    bool (*check_punctaution_char)(struct lxl_lexer *self);  // Check if the next character is punctuation.
    // --- Default behavour: always return false.
    bool (*match_word_char)(struct lxl_lexer *self);  // Consume a single word-constituent character.
    // --- Default behaviour: return true for any non-whitespace, non-punctuation character.
    // Hook functions (called at specific times).
    void (*before_token_hook)(struct lxl_lexer *self);      // Called at the start of token lexing.
    void (*after_whitespace_hook)(struct lxl_lexer *self);  // Called after whitespace has been skipped.
    void (*before_integer_hook)(struct lxl_lexer *self);    // Called before integer lexing.*
    void (*after_integer_hook)(struct lxl_lexer *self);     // Called after integer lexing.**
    void (*before_float_hook)(struct lxl_lexer *self);      // Called before floating point lexing.*
    void (*after_float_hook)(struct lxl_lexer *self);       // Called after floating point lexing.**
    void (*on_error_hook)(struct lxl_lexer *self);          // Called as soon as an error occurs.
    void (*before_error_token_hook)(struct lxl_lexer *self);// Called before an error token is finalised.
    void (*after_token_hook)(struct lxl_lexer *self);       // Called just before token is returned.
    // Notes:
    // *  these hooks are only called after the respective token type has been determined/guessed.
    // ** these hooks are called just before the respective lexing functions return to the caller,
    //    meaning they can change the lexer's status for more advanced control.
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

// Region allocator.
// NOTE: the `.data` buffer is allocated by the caller.
struct lxl_region {
    size_t capacity;
    size_t alloc_count;
    char *data;
};

// END LEXEL ADDITIONAL.


// LEXEL MAGIC VALUES (MVs).

// These enums define human-friendly names for the various magic values used by lexel.

enum lxl__token_mvs {
    LXL_TOKENS_END = -1,           // Special token type signifying the end of the token stream.
    LXL_TOKEN_UNINIT = -2,         // Special token type for a token whose type is yet to be determined.
    LXL_TOKENS_END_ABNORMAL = -3,  // Special token type signifying an abnormal end of the token stream.
    LXL_TOKEN_LINE_ENDING = -4,    // Special token type signifying the end of a line.
    LXL_TOKEN_NO_TOKEN = -5,       // Special token type for a non-existant token.
    // See enum lxl_lex_error for token error types.
};


// A string containing all the characters lexel considers whitespace apart from line feed, which has
// special handling in the lexer.
#define LXL_WHITESPACE_CHARS_NO_LF " \t\r\f\v"
// A string contining all the characters lexel considers whitespace (including line feed).
#define LXL_WHITESPACE_CHARS LXL_WHITESPACE_CHARS_NO_LF "\n"

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
    ((tok).kind == LXL_TOKENS_END || (tok).kind == LXL_TOKENS_END_ABNORMAL)

// Return whetehr `tok` is a special error token.
#define LXL_TOKEN_IS_ERROR(tok) ((tok).kind <= LXL_LERR_GENERIC)

// Return the token's value as a string view.
struct lxl_string_view lxl_token_value(struct lxl_token token);

// Return a textual representation of the error code.
const char *lxl_error_message(enum lxl_lex_error error);

// END TOKEN INTERFACE.


// LEXER BUILDER INTERFACE.

// These functions provide an interface for easily building a lexer as defined by lexel.
// This interface is NOT required for using lexel; you can also build a lexer by hand.
// NOTE: the lexer should be created via `lxl_lexer_new()` or `lxl_lexer_from_sv()`
// BEFORE calling any of the functions listed below.

// Make lexer support integer literals. The basic usage of this function/macro is to define
// the token type for integer literals and provide support for unprefixed integer literals,
// which by default are decimal (base-10), but this can be customised by setting
// lexer.default_int_base. Additional prefix--base pairs (of type const char *, int,
// respectively). These will be allocated in the given region. If no additional bases
// are needed, the region parameter may be NULL.
// NOTE: the prefix pointers are assumed to point into memory that persists throughout the entire
// lifetime of the lexer. If this is not the case, the string must first be copied to a persisent
// buffer.
// NOTE 2: the region must live at least as long as the lexer itself.
// All arguments are forwarded to lxl_builder_add_integers_impl().
#define lxl_builder_add_integers(lexer, region, /* kind, */ ...)   \
    lxl_builder_add_integers_impl(lexer, region, __VA_ARGS__, (const char *)NULL);
// Make lexer support integer literals. This function is not intended to be directly called but
// to be forwarded-to by the macro version above. At least one variadic parameter is needed
// including NULL which terminates the argument list.
bool lxl_builder_add_integers_impl(struct lxl_lexer *lexer, struct lxl_region *region, int kind, ...);

// Add the given integer suffixes to the lexer.
// NOTE: the suffix pointers are assumed to point to memory that lives at least as long as the lexer,
// as is the region.
#define lxl_builder_add_integer_suffixes(lexer, /* region, */ ...)       \
    lxl_builder_add_integer_suffixes_impl(lexer, __VA_ARGS__)
// Add the given integer suffixes to the lexer. This function is not intended to be directly called but
// to be forwarded-to by the macro version above. At least one variadic parameter is needed
// incluing NULL which terminates the argument list.
bool lxl_builder_add_integer_suffixes_impl(struct lxl_lexer *lexer, struct lxl_region *region, ...);

// END LEXER BUILDER INTERFACE.


// LEXER EXTERNAL INTERFACE.

// These functions are for communicating with the lexer, e.g. when parsing.

// Create a new `lxl_lexer` object from start and end pointers.
// `start` must be a valid, non_NULL pointer. If `end` is non-NULL, it must point one
// past the end of the the string beginning at `start`. If `end` is NULL, it is inferred
// by the length of the `start` string, which must be null-terminated in this case.
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

// Construct a zero-terminated array to use for setting lexer fields calling for lists.
// Requires at least one element.
#define LXL_LIST(type, ...) ((type[]) {__VA_ARGS__, 0})

// Construct a NULL-terminated list of strings.
#define LXL_LIST_STR(...) LXL_LIST(const char *, __VA_ARGS__)

// Construct a {0}-terminated list of delimiter pairs.
#define LXL_LIST_DELIMS(...) ((struct lxl_delim_pair[]) {__VA_ARGS__, {0}})

// END LEXER EXTERNAL INTERFACE.


// LEXER INTERNAL INTERFACE.

// These functions are used by the lexer to alter its own state.
// They should not be used from a parser, but the interface is exposed to make writing a custom lexer easier.
// Hooks may be used to inject arbitrary logic into the lexer at well-defined stages. Some hooks can change
// the internal status of the lexer, allowing for more complex lexing.

// Call the specified hook on the object pointed to by `self`.
// NOTE: a hook is a member of `self` with type `void (*)(typeof(self))`.
#define LXL_CALL_HOOK(self, hook) \
    do if ((self)->hook) (self)->hook(self); while (0)

// Return the number of characters consumed so far.
ptrdiff_t lxl_lexer__head_length(struct lxl_lexer *lexer);
// Return the number of characters left in the lexer's source.
ptrdiff_t lxl_lexer__tail_length(struct lxl_lexer *lexer);
// Return the number of characters consumed after `start_point`.
ptrdiff_t lxl_lexer__length_from(struct lxl_lexer *lexer, const char *start_point);
// Return the number of characters between now and `end_point`.
ptrdiff_t lxl_lexer__length_to(struct lxl_lexer *lexer, const char *end_point);

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
// Advance the lexer to a future point in its input.
bool lxl_lexer__advance_to(struct lxl_lexer *lexer, const char *future);
// Rewind the lexer to the previous character and return whether the rewind was successful (the lexer
// cannot be rewound beyond its starting point).
bool lxl_lexer__rewind(struct lxl_lexer *lexer);
// Rewind the lexer by up to n characters and return whether all n characters could be rewound.
bool lxl_lexer__rewind_by(struct lxl_lexer *lexer, size_t n);
// Rewind the lexer to a previous point in its input and return whether all characters could be rewound.
bool lxl_lexer__rewind_to(struct lxl_lexer *lexer, const char *prev);

// Un-lex the current token (i.e. reset the lexer to the start of the token).
void lxl_lexer__unlex(struct lxl_lexer *lexer);

// Recalculate the current column in the lexer.
void lxl_lexer__recalc_column(struct lxl_lexer *lexer);

// Return whether the lexer can emit a line ending token when it sees an LF.
bool lxl_lexer__can_emit_line_ending(struct lxl_lexer *lexer);

// Return non-NULL if the current current matches any of those passed but do not consume it, otherwise,
// return NULL. On success, the return value is the pointer to the matching character, i.e., into the
// null-terminated string `chars`.
const char *lxl_lexer__check_chars(struct lxl_lexer *lexer, const char *chars);
// Return whether the next characters match exactly the string passed, but do not consume them.
bool lxl_lexer__check_string(struct lxl_lexer *lexer, const char *s);
// Return whether the next n characters match the first n characters of the string passed,
// but do not consume them.
bool lxl_lexer__check_string_n(struct lxl_lexer *lexer, const char *s, size_t n);
// Return whether the next characters match one of the strings passed, but do not consume the string.
bool lxl_lexer__check_strings(struct lxl_lexer *lexer, const char *const *strings);
// Return whether the current character is whitespace (see LXL_WHITESPACE_CHARS).
bool lxl_lexer__check_whitespace(struct lxl_lexer *lexer);
// Return whether the current character is whitespace including LF (regardless of lexer.emit_line_endings).
bool lxl_lexer__check_whitespace_with_lf(struct lxl_lexer *lexer);
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
// Return non-NULL if the next characters comprise one of the lexer's string openers but do not consume
// them, otherwise, return NULL. On success, the return value is the pointer to the matching opener.
const struct lxl_delim_pair *lxl_lexer__check_string_opener(struct lxl_lexer *lexer,
                                                        enum lxl_string_type string_type);
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
// Return non-zero if the next characters comprise a float literal prefix but do not consume them. The
// return value is the base corresponding to the matched prefix and OUT_exponent_marker is written
// with the corresponding exponent marker.
int lxl_lexer__check_float_prefix(struct lxl_lexer *lexer, const char **OUT_exponent_marker);
// Return whether the next characters comprise a float literal suffix but do not consume them.
bool lxl_lexer__check_float_suffix(struct lxl_lexer *lexer);
// Return whether the next characters comprise a number literal sign (e.g. "+", "-") but do not consume them.
bool lxl_lexer__check_number_sign(struct lxl_lexer *lexer);
// Return whether the next characters comprise a float radix separator (e.g. ".") but do not consume them.
bool lxl_lexer__check_radix_separator(struct lxl_lexer *lexer);
// Return whether the next characters comprise a float exponent sign (e.g. "+", "-") but do not consume them.
bool lxl_lexer__check_exponent_sign(struct lxl_lexer *lexer);
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
// Return whether the next characters match one of the strings passed, and consume the string if so.
bool lxl_lexer__match_strings(struct lxl_lexer *lexer, const char *const *strings);
// Return whether the next characters comprise a line comment, and consume them if so.
bool lxl_lexer__match_line_comment(struct lxl_lexer *lexer);
// Return wheteher the next characters comprise a block comment, and consume them if so.
bool lxl_lexer__match_block_comment(struct lxl_lexer *lexer);
// Return whether the next characters comprise a nestable block comment, and consume them if so.
bool lxl_lexer__match_nestable_comment(struct lxl_lexer *lexer);
// Return whether the next characters comprise an unnestable block comment, and consume them if so.
bool lxl_lexer__match_unnestable_comment(struct lxl_lexer *lexer);
// Return non-NULL if the next characters comprise one of the lexer's string delimiters and consume them
// if so, otherwise, return NULL. On success, the return value is the pointer to the matching opener.
const struct lxl_delim_pair *lxl_lexer__match_string_opener(struct lxl_lexer *lexer,
                                                        enum lxl_string_type string_type);
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
// Return non-zero if the next characters comprise a float literal prefix, and consume them if so. The
// return value is the base corresponding to the matched prefix and OUT_exponent_marker is written
// with the corresponding exponent marker.
int lxl_lexer__match_float_prefix(struct lxl_lexer *lexer, const char **OUT_exponent_marker);
// Return whether the next characters comprise a float literal suffix, and consume them if so.
bool lxl_lexer__match_float_suffix(struct lxl_lexer *lexer);
// Return whether the next characters comprise a number literal sign (e.g. "+", "-"), and consume them if so.
bool lxl_lexer__match_number_sign(struct lxl_lexer *lexer);
// Return whether the next characters comprise a float radix separator (e.g. "."), and consume them if so.
bool lxl_lexer__match_radix_separator(struct lxl_lexer *lexer);
// Return whether the next characters comprise a float exponent sign (e.g. "+", "-"), and consume them if so.
bool lxl_lexer__match_exponent_sign(struct lxl_lexer *lexer);
// Return non-NULL if the next characters comprise an punct and consume them if so, otherwise,
// return NULL. On success, the return value is the pointer to the matching punct in the .puncts list.
const char *const *lxl_lexer__match_punct(struct lxl_lexer *lexer);

// Advance the lexer past any whitespace characters and return the number of characters consumed.
int lxl_lexer__skip_whitespace(struct lxl_lexer *lexer);
// Advance the lexer past the rest of the current line and return the number of characters consumed.
int lxl_lexer__skip_line(struct lxl_lexer *lexer);
// Advance the lexer past the current (possibly nestable) block comment (opener already consumed)
// and return the number of characters consumed.
int lxl_lexer__skip_block_comment(struct lxl_lexer *lexer, struct lxl_delim_pair delims, bool nested);

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
int lxl_lexer__lex_string(struct lxl_lexer *lexer, const char *closer, enum lxl_string_type string_type);
// Consume the digits of an integer literal in the given base (2--36).
int lxl_lexer__lex_integer(struct lxl_lexer *lexer, int base);
// Consume the digits of a floating-point literal with the given base and exponent marker.
int lxl_lexer__lex_float(struct lxl_lexer *lexer, int index, const char *exponent_marker);

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

// Compare two string views in a manner similar to `strcmp()`.
int lxl_sv_compare(struct lxl_string_view a, struct lxl_string_view b);
// Check if two string views have the same contents.
bool lxl_sv_equal(struct lxl_string_view a, struct lxl_string_view b);

// END LEXEL STRING VIEW.


// LEXEL REGION.

// Create a region with a fixed-size array as its backing buffer.
#define REGION_FROM_ARRAY(array)                 \
    ((struct lxl_region) {.capacity = sizeof(array), .alloc_count = 0, .data = (array)})

// Allocate into a region.
void *lxl_region_allocate(size_t size, struct lxl_region *region);
// Reset a region (deallocate all allocations).
void lxl_region_reset(struct lxl_region *region);

// Align a region to the next alignment boundary.
bool lxl_region__align(struct lxl_region *region);

// END LEXEL REGION.


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
    case LXL_LERR_INVALID_FLOAT: return "Invalid floating-point literal";
    }
    LXL_UNREACHABLE();
    return NULL;  // Unreachable.
}

// END TOKEN FUNCTIONS.


// LEXER BUILDER FUNCTIONS

bool lxl_builder_add_integers_impl(struct lxl_lexer *lexer, struct lxl_region *region, int kind, ...) {
    va_list vargs;
    LXL_ASSERT(lexer != NULL);
    lexer->default_int_type = kind;
    lexer->default_int_base = 10;
    int arg_count = 0;
    va_start(vargs, kind);
    for (const char *arg; (arg = va_arg(vargs, const char *)); ++arg_count) {
        (void)va_arg(vargs, int);  // Consume base argument.
    }
    va_end(vargs);
    LXL_ASSERT(arg_count >= 0);
    if (arg_count == 0) return true;
    const char **prefixes = lxl_region_allocate((arg_count + 1) * sizeof *prefixes, region);
    int *bases = lxl_region_allocate(arg_count * sizeof *bases, region);
    if (!prefixes || !bases) return false;  // Failed to allocate in region.
    va_start(vargs, kind);
    for (int i = 0; i < arg_count; ++i) {
        const char *prefix = va_arg(vargs, const char *);
        LXL_ASSERT(prefix != NULL);
        int base = va_arg(vargs, int);
        prefixes[i] = prefix;
        bases[i] = base;
    }
    va_end(vargs);
    prefixes[arg_count] = NULL;
    lexer->integer_prefixes = prefixes;
    lexer->integer_bases = bases;
    return true;
}

bool lxl_builder_add_integer_suffixes_impl(struct lxl_lexer *lexer, struct lxl_region *region, ...) {
    va_list vargs;
    LXL_ASSERT(lexer != NULL);
    int arg_count = 0;
    va_start(vargs, region);
    for (const char *arg; (arg = va_arg(vargs, const char *)); ++arg_count) {
        /* Do nothing. */
    }
    va_end(vargs);
    LXL_ASSERT(arg_count >= 0);
    const char **suffixes = lxl_region_allocate((arg_count + 1) * sizeof *suffixes, region);
    if (!suffixes) return false;
    va_start(vargs, region);
    for (int i = 0; i < arg_count; ++i) {
        const char *suffix = va_arg(vargs, const char *);
        LXL_ASSERT(suffix != NULL);
        suffixes[i] = suffix;
    }
    va_end(vargs);
    suffixes[arg_count] = NULL;
    return true;
}

// END LEXER BUILDER FUNCTIONS.


// LEXER FUNCTIONS.

struct lxl_lexer lxl_lexer_new(const char *start, const char *end) {
    LXL_ASSERT(start != NULL);
    if (end == NULL) end = start + strlen(start);
    static const char *default_exponent_signs[] = {"+", "-", NULL};
    static const char *default_radix_separators[] = {".", NULL};
    return (struct lxl_lexer) {
        // Lexer state.
        .status = LXL_LSTS_READY,
        .error = LXL_LERR_OK,
        .start = start,
        .end = end,
        .current = start,
        .pos = {0, 0},
        .token = {0},
        // Customisation flags.
        .emit_line_endings = false,
        .collect_line_endings = true,
        // Query functions.
        .match_whitespace_char = NULL,
        .match_line_comment = NULL,
        .check_punctuation_char = NULL,
        .match_word_char = NULL,
        // Hook functions.
        .before_token_hook = NULL,
        .after_whitespace_hook = NULL,
        .before_integer_hook = NULL,
        .after_integer_hook = NULL,
        .before_float_hook = NULL,
        .after_float_hook = NULL,
        .on_error_hook = NULL,
        .before_error_token_hook = NULL,
        .after_token_hook = NULL,
    };
}

struct lxl_lexer lxl_lexer_from_sv(struct lxl_string_view sv) {
    return lxl_lexer_new(sv.start, LXL_SV_END(sv));
}

struct lxl_token lxl_lexer_next_token(struct lxl_lexer *lexer) {
    if (lxl_lexer_is_finished(lexer)) {
        return lxl_lexer__create_end_token(lexer);
    }
    LXL__CALL_HOOK(lexer, before_token_hook);
    lxl_lexer__skip_whitespace(lexer);
    LXL__CALL_HOOK(lexer, after_whitespace_hook);
    if (lexer->error) {
        return lxl_lexer__create_error_token(lexer);
    }
    else if (lxl_lexer__is_at_end(lexer)) {
        return lxl_lexer__create_end_token(lexer);
    }
    lxl_lexer__start_token(lexer);
    lexer->status = LXL_LSTS_DECIDING_TYPE;
    while (lexer->status != LXL_LSTS_TOKEN_LEXED) {
        if (lxl_lexer__check_punctuation(lexer)) {
            lxl_lexer__lex_punctuation(lexer);
        }
        LXL_ASSERT(lexer->status != LXL_LSTS_DECIDING_TYPE && "Lexel error: lexer still deciding type");
    }
    return lxl_lexer__finish_token(lexer);
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

ptrdiff_t lxl_lexer__length_to(struct lxl_lexer *lexer, const char *end_point) {
    return end_point - lexer->current;
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

bool lxl_lexer__advance_to(struct lxl_lexer *lexer, const char *future) {
    ptrdiff_t length = lxl_lexer__length_to(lexer, future);
    LXL_ASSERT(length >= 0);
    return lxl_lexer__advance_by(lexer, length);
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

bool lxl_lexer__rewind_to(struct lxl_lexer *lexer, const char *prev) {
    ptrdiff_t length = lxl_lexer__length_from(lexer, prev);
    LXL_ASSERT(length >= 0);
    return lxl_lexer__rewind_by(lexer, length);
}

void lxl_lexer__unlex(struct lxl_lexer *lexer) {
    bool result = lxl_lexer__rewind_to(lexer, lexer->token_start);
    LXL_ASSERT(result && "Lexer could not rewind to the start of the token!");
}

void lxl_lexer__recalc_column(struct lxl_lexer *lexer) {
    lexer->pos.column = 0;
    for (const char *p = lexer->current; p != lexer->start && *p != '\n'; --p) {
        ++lexer->pos.column;
    }
}

bool lxl_lexer__can_emit_line_ending(struct lxl_lexer *lexer) {
    if (!lexer->emit_line_endings) return false;
    if (lexer->previous_kind == LXL_TOKEN_LINE_ENDING) {
        return !lexer->collect_line_endings;
    }
    return true;
}

const char *lxl_lexer__check_chars(struct lxl_lexer *lexer, const char *chars) {
    if (chars == NULL) return NULL;  // Allow NULL.
    while (*chars != '\0') {
        if (*lexer->current == *chars) return chars;
        ++chars;
    }
    return NULL;
}

bool lxl_lexer__check_string(struct lxl_lexer *lexer, const char *s) {
    if (s == NULL) return false;
    size_t tail_length = lxl_lexer__tail_length(lexer);
    size_t n = strlen(s);
    if (n > tail_length) return false;
    return memcmp(lexer->current, s, n) == 0;
}

bool lxl_lexer__check_string_n(struct lxl_lexer *lexer, const char *s, size_t n) {
    if (s == NULL) return false;
    size_t tail_length = lxl_lexer__tail_length(lexer);
    if (n > tail_length) n = tail_length;
    return strncmp(lexer->current, s, n) == 0;
}

bool lxl_lexer__check_strings(struct lxl_lexer *lexer, const char *const *strings) {
    if (strings == NULL) return NULL;
    for (; *strings != NULL; ++strings) {
        if (lxl_lexer__check_string(lexer, *strings)) return true;
    }
    return false;
}

bool lxl_lexer__check_whitespace(struct lxl_lexer *lexer) {
    if (!lxl_lexer__can_emit_line_ending(lexer)) {
        return lxl_lexer__check_chars(lexer, LXL_WHITESPACE_CHARS);
    }
    return lxl_lexer__check_chars(lexer, LXL_WHITESPACE_CHARS_NO_LF);
}

bool lxl_lexer__check_whitespace_with_lf(struct lxl_lexer *lexer) {
    return lxl_lexer__check_chars(lexer, LXL_WHITESPACE_CHARS);
}

bool lxl_lexer__check_reserved(struct lxl_lexer *lexer) {
    return lxl_lexer__check_whitespace_with_lf(lexer)
        || lxl_lexer__check_line_comment(lexer)
        || lxl_lexer__check_block_comment(lexer)
        || !!lxl_lexer__check_string_opener(lexer, LXL_STRING_LINE)
        || !!lxl_lexer__check_string_opener(lexer, LXL_STRING_MULTILINE)
        || !!lxl_lexer__check_punct(lexer)
        ;
}

bool lxl_lexer__check_line_comment(struct lxl_lexer *lexer) {
    return lxl_lexer__check_strings(lexer, lexer->line_comment_openers);
}

bool lxl_lexer__check_block_comment(struct lxl_lexer *lexer) {
    return lxl_lexer__check_nestable_comment(lexer) || lxl_lexer__check_unnestable_comment(lexer);
}

bool lxl_lexer__check_nestable_comment(struct lxl_lexer *lexer) {
    if (lexer->nestable_comment_delims == NULL) return false;
    for (const struct lxl_delim_pair *delims = lexer->nestable_comment_delims;
         delims->opener != NULL;
         ++delims) {
        if (lxl_lexer__check_string(lexer, delims->opener)) return true;
    }
    return false;
}

bool lxl_lexer__check_unnestable_comment(struct lxl_lexer *lexer) {
    if (lexer->unnestable_comment_delims == NULL) return false;
    for (const struct lxl_delim_pair *delims = lexer->unnestable_comment_delims;
         delims->opener != NULL;
         ++delims) {
        if (lxl_lexer__check_string(lexer, delims->opener)) return true;
    }
    return false;
}

const struct lxl_delim_pair *lxl_lexer__check_string_opener(struct lxl_lexer *lexer,
                                                        enum lxl_string_type string_type) {
    const struct lxl_delim_pair *string_delims = (string_type == LXL_STRING_LINE)
        ? lexer->line_string_delims
        : lexer->multiline_string_delims;
    if (string_delims == NULL) return NULL;
    for (const struct lxl_delim_pair *delims = string_delims; delims->opener != NULL; ++delims) {
        if (lxl_lexer__check_chars(lexer, delims->opener)) return delims;
    }
    return NULL;
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
        LXL_ASSERT(lexer->integer_bases != NULL);
        for (int i = 0; lexer->integer_prefixes[i] != NULL; ++i) {
            if (lxl_lexer__check_string(lexer, lexer->integer_prefixes[i])) {
                lxl_lexer__rewind_to(lexer, start);
                return lexer->integer_bases[i];
            }
        }
    }
    int default_base = lexer->default_int_base;
    return (lxl_lexer__check_digit(lexer, default_base)) ? default_base : 0;
}

bool lxl_lexer__check_int_suffix(struct lxl_lexer *lexer) {
    return lxl_lexer__check_strings(lexer, lexer->integer_suffixes);
}

int lxl_lexer__check_float_prefix(struct lxl_lexer *lexer, const char **OUT_exponent_marker) {
    const char *start = lexer->current;
    lxl_lexer__match_number_sign(lexer);
    if (lexer->float_prefixes) {
        LXL_ASSERT(lexer->float_bases != NULL);
        LXL_ASSERT(lexer->exponent_markers != NULL);
        for (int i = 0; lexer->float_prefixes[i] != NULL; ++i) {
            if (lxl_lexer__check_string(lexer, lexer->float_prefixes[i])) {
                lxl_lexer__rewind_to(lexer, start);
                *OUT_exponent_marker = lexer->exponent_markers[i];
                return lexer->float_bases[i];
            }
        }
    }
    lxl_lexer__rewind_to(lexer, start);
    if (!lxl_lexer__check_digit(lexer, lexer->default_float_base)) return 0;
    *OUT_exponent_marker = lexer->default_exponent_marker;
    return lexer->default_float_base;
}

bool lxl_lexer__check_float_suffix(struct lxl_lexer *lexer) {
    return lxl_lexer__check_strings(lexer, lexer->float_suffixes);
}

bool lxl_lexer__check_number_sign(struct lxl_lexer *lexer) {
    return lxl_lexer__check_strings(lexer, lexer->number_signs);
}

bool lxl_lexer__check_radix_separator(struct lxl_lexer *lexer) {
    return lxl_lexer__check_strings(lexer, lexer->radix_separators);
}

bool lxl_lexer__check_exponent_sign(struct lxl_lexer *lexer) {
    return lxl_lexer__check_strings(lexer, lexer->exponent_signs);
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

bool lxl_lexer__match_strings(struct lxl_lexer *lexer, const char *const *strings) {
    if (strings == NULL) return NULL;
    for (; *strings != NULL; ++strings) {
        if (lxl_lexer__match_string(lexer, *strings)) return true;
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
    for (const struct lxl_delim_pair *delims = lexer->nestable_comment_delims;
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
    for (const struct lxl_delim_pair *delims = lexer->unnestable_comment_delims;
         delims->opener != NULL;
         ++delims) {
        if (lxl_lexer__match_string(lexer, delims->opener)) {
            lxl_lexer__skip_block_comment(lexer, *delims, false);
            return true;
        }
    }
    return false;
}

const struct lxl_delim_pair *lxl_lexer__match_string_opener(struct lxl_lexer *lexer,
                                                        enum lxl_string_type string_type) {
    const struct lxl_delim_pair *string_delims = (string_type == LXL_STRING_LINE)
        ? lexer->line_string_delims
        : lexer->multiline_string_delims;
    if (string_delims == NULL) return NULL;
    for (const struct lxl_delim_pair *delims = string_delims; delims->opener != NULL; ++delims) {
        if (lxl_lexer__match_chars(lexer, delims->opener)) return delims;
    }
    return NULL;
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
        LXL_ASSERT(lexer->integer_bases != NULL);
        for (int i = 0; lexer->integer_prefixes[i] != NULL; ++i) {
            if (lxl_lexer__match_string(lexer, lexer->integer_prefixes[i])) {
                return lexer->integer_bases[i];
            }
        }
    }
    return (lxl_lexer__check_digit(lexer, lexer->default_int_base)) ? lexer->default_int_base : 0;
}

bool lxl_lexer__match_int_suffix(struct lxl_lexer *lexer) {
    return lxl_lexer__match_strings(lexer, lexer->integer_suffixes);
}

int lxl_lexer__match_float_prefix(struct lxl_lexer *lexer, const char **OUT_exponent_marker) {
    lxl_lexer__match_number_sign(lexer);
    if (lexer->float_prefixes) {
        LXL_ASSERT(lexer->float_bases != NULL);
        LXL_ASSERT(lexer->exponent_markers != NULL);
        for (int i = 0; lexer->float_prefixes[i]; ++i) {
            if (lxl_lexer__match_string(lexer, lexer->float_prefixes[i])) {
                *OUT_exponent_marker = lexer->exponent_markers[i];
                return lexer->float_bases[i];
            }
        }
    }
    if (!lxl_lexer__check_digit(lexer, lexer->default_float_base)) return 0;
    *OUT_exponent_marker = lexer->default_exponent_marker;
    return lexer->default_float_base;
}

bool lxl_lexer__match_float_suffix(struct lxl_lexer *lexer) {
    return lxl_lexer__match_strings(lexer, lexer->float_suffixes);
}

bool lxl_lexer__match_number_sign(struct lxl_lexer *lexer) {
    return lxl_lexer__match_strings(lexer, lexer->number_signs);
}

bool lxl_lexer__match_radix_separator(struct lxl_lexer *lexer) {
    return lxl_lexer__match_strings(lexer, lexer->radix_separators);
}

bool lxl_lexer__match_exponent_sign(struct lxl_lexer *lexer) {
    return lxl_lexer__match_strings(lexer, lexer->exponent_signs);
}

const char *const *lxl_lexer__match_punct(struct lxl_lexer *lexer) {
    if (lexer->puncts == NULL) return NULL;
    for (const char *const *punct = lexer->puncts; *punct != NULL; ++punct) {
        if (lxl_lexer__match_string(lexer, *punct)) return punct;
    }
    return NULL;
}

int lxl_lexer__skip_whitespace(struct lxl_lexer *lexer) {
    lexer->status = LXL_LSTS_SKIPPING_WHITESPACE;
    const char *whitespace_start = lexer->current;
    for(;;) {
        if (lxl_lexer__check_whitespace(lexer)) {
            // Whitespace, skip.
            if (!lxl_lexer__advance(lexer)) break;
        }
        else if (lxl_lexer__check_string(lexer, "\n")) {
            // LF should have already been considered whitespace if we cannot emit a line ending here.
            LXL_ASSERT(lxl_lexer__can_emit_line_ending(lexer));
            break;
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
    // NOTE: final LF is NOT consumed.
    while (!lxl_lexer__check_chars(lexer, "\n")) {
        if (!lxl_lexer__advance(lexer)) break;
    }
    return lxl_lexer__length_from(lexer, line_start);
}

int lxl_lexer__skip_block_comment(struct lxl_lexer *lexer, struct lxl_delim_pair delims, bool nestable) {
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

void lxl_lexer__start_token(struct lxl_lexer *lexer) {
    lexer->token = (struct lxl_token) {
        .start = lexer->current,
        .end = lexer->current,
        .loc = lexer->pos,
        .kind = LXL_TOKEN_UNINIT,
    };
}

struct lxl_token lxl_lexer__finish_token(struct lxl_lexer *lexer) {
    lexer->token->end = lexer->current;
    if (lexer->error) {
        LXL_CALL_HOOK(lexer, before_error_token_hook);
        lexer->token.kind = lexer->error;  // Set error as token type.
        lexer->error = LXL_LERR_OK;  // Clear error.
    }
    if (!lxl_lexer__is_finished(lexer)) lexer->status = LXL_LSTS_READY;  // Ready for the next token.
    LXL_CALL_HOOK(lexer, after_token_hook);
    return lexer->token;
}

struct lxl_token lxl_lexer__create_end_token(struct lxl_lexer *lexer) {
    lxl_lexer__start_token(lexer);
    if (lexer->status != LXL_LSTS_FINISHED_ABNORMAL) {
        lexer->token.kind = LXL_TOKENS_END;
        lexer->status = LXL_LSTS_FINISHED;
    }
    else {
        lexer->token.kind = LXL_TOKENS_END_ABNORMAL;
    }
    return lxl_lexer__finish_token(lexer);
}

struct lxl_token lxl_lexer__create_error_token(struct lxl_lexer *lexer) {
    lxl_lexer__start_token(lexer);
    if (!lexer->error) lexer->error = LXL_LERR_GENERIC;  // Emit a generic error token if no error is set.
    return lxl_lexer__finish_token(lexer);  // This function handles setting the error type.
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

int lxl_lexer__lex_string(struct lxl_lexer *lexer, const char *closer, enum lxl_string_type string_type) {
    LXL_ASSERT(closer != NULL);
    const char *start = lexer->current;
    while (!lxl_lexer__match_string(lexer, closer)) {
        if (lxl_lexer__match_chars(lexer, lexer->string_escape_chars)) {
            // Consume escaped closer.
            lxl_lexer__match_string(lexer, closer);
        }
        // Consume non-delimiter character.
        char c = lxl_lexer__advance(lexer);
        if (c == '\0' || (c == '\n' && string_type == LXL_STRING_LINE)) {
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
        LXL_LEXER__CALL_HOOK0(lexer, before_unlex_int_hook);
        lxl_lexer__unlex(lexer);
        return 0;
    }
    return lxl_lexer__length_from(lexer, start);
}

int lxl_lexer__lex_float(struct lxl_lexer *lexer, int base, const char *exponent_marker) {
    const char *start = lexer->current;
    int digit_length = lxl_lexer__lex_integer(lexer, base);
    if (lxl_lexer__match_radix_separator(lexer)) {
        // Part after '.' OE.
        digit_length += lxl_lexer__lex_integer(lexer, base);
    }
    if (lxl_lexer__match_string(lexer, exponent_marker)) {
        // Part after 'e' OE.
        lxl_lexer__match_exponent_sign(lexer);  // Consume sign before actual exponent.
        digit_length += lxl_lexer__lex_integer(lexer, base);
    }
    if (digit_length <= 0) {
        // Un-lex token which is not a valid floating-point literal.
        LXL_LEXER__CALL_HOOK0(lexer, before_unlex_float_hook);
        lxl_lexer__unlex(lexer);
        return 0;
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

int lxl_sv_compare(struct lxl_string_view a, struct lxl_string_view b) {
    if (a.length == b.length) return memcmp(a.start, b.start, a.length);
    size_t compare_length = (a.length < b.length) ? a.length : b.length;
    int result1 = memcmp(a.start, b.start, compare_length);
    if (result1 == 0) return (a.length < b.length) ? -1 : 1;
    return result1;
}

bool lxl_sv_equal(struct lxl_string_view a, struct lxl_string_view b) {
    if (a.length != b.length) return false;
    return memcmp(a.start, b.start, a.length) == 0;
}


// END STRING VIEW FUNCTIONS.

// REGION FUNCTIONS.

void *lxl_region_allocate(size_t size, struct lxl_region *region) {
    if (!lxl_region__align(region)) return NULL;
    if (region->alloc_count + size > region->capacity) return NULL;
    void *ptr = &region->data[region->alloc_count];
    region->alloc_count += size;
    return ptr;
}

void lxl_region_reset(struct lxl_region *region) {
    region->alloc_count = 0;
}

bool lxl_region__align(struct lxl_region *region) {
    intptr_t iptr = (intptr_t)&region->data[region->alloc_count];
    int residue = iptr & ((int)LXL_REGION_ALIGN - 1);
    if (residue == 0) return true;
    int offset = (int)LXL_REGION_ALIGN - residue;
    LXL_ASSERT(offset > 0);
    if (iptr + offset >= (intptr_t)&region->data[region->capacity]) return false;
    region->alloc_count += offset;
    return true;
}

// END REGION FUNCTIONS.

#endif  // LEXEL_IMPLEMENTATION

#endif  // LEXEL_H

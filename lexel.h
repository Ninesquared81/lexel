/*
 * Lexel -- a simple, general purpose lexing library in C.
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


// A note on TERMINOLOGY:
// In the definitions below, certain terminology is used in a standardised manner.
// For brevity, the doc-comments for each function, etc., will NOT explain the meaning
// of these terms, so their defintions are listed here.

// MATCH_x functions:
// These functions conditionally consume characters in the lexer based on some pattern
// or attribute, returning `true` if a match was found and `false` otherwise (i.e. whether
// or not characters were consumed).

// x_HOOK functions:
// These are optional functions called at specific points in the lexing process. The exact
// point in time when a particular hook is called is encoded in its name. For example, the
// `after_token_hook()` is called at the very end of lexing, just before the token is
// returned to the caller.
// Hook functions have full access to the lexer's internal state at their specific time, and
// can even modify this state, allowing for fine-grained control over the lexing process.

// WORDs:
// A word is generally either an identifier or keyword. Depending on the language, words may
// include symbolic characters such as `-` or `_`, or may consist of only alphanumeric (or
// even just alphabetic) characters.



#ifndef LEXEL_H
#define LEXEL_H

#include <assert.h>     // assert()
#include <stdbool.h>    // bool, false, true -- requires C99
#include <stddef.h>     // ptrdiff_t
#include <stdint.h>     // int32_t, uint8_t.

// CUSTOMISATION OPTIONS.

// These options customise certain behaviours of lexel.
// They're defined at the top of the file for visibility.

// Customisable options can be given custom definitions before including lexel.h.

// LXL_NO_ASSERT disables lexel library assertions. This setting is independant of NDEBUG.

// This option controls which macro should be used for lexel library assertions.
// The default value is the standard assert() macro.
// NOTE: this macro should not be customised to merely disable assertions. To do that, use LXL_NO_ASSERT.
#ifndef LXL_ASSERT_MACRO
# define LXL_ASSERT_MACRO assert
#endif

// END CUSTOMISATION OPTIONS.

// META-DEFINITIONS.

// These definitions are not part of the lexel interface per se, but have special signficance
// within this file.

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

#define LXL_TODO(TASK) LXL_ASSERT(0 && "TODO: " TASK)

// END META-DEFINITIONS.


// LEXEL ADDITIONAL.

// Additional definitions beyond the core below.

// A string view.
// This is a read-only (non-owning) view into a string, consiting of a `start` pointer and `length`.
struct lxl_string_view {
    const char *start;
    ptrdiff_t length;
};

// A single Unicode codepoint.
typedef uint32_t lxl_UnicodeCodepoint;

// Error codes for Unicode functions.
enum lxl_unicode_error {
    LXL_UNIERR_OK = 0,
    LXL_UNIERR_UNEXPECTED_EOF,          // Unexpected end of input (possibly in multibyte sequence).
    LXL_UNIERR_INVALID_FIRST_BYTE,      // Invalid first byte for UTF-8 sequence.
    LXL_UNIERR_INVALID_CONT_BYTE,       // Invalid continuation byte
    LXL_UNIERR_UNEXPECTED_CONT_BYTE,    // Continuation byte at start of input.
    LXL_UNIERR_MISSING_CONT_BYTE,       // Missing continuation byte(s) before start of next sequence.
    LXL_UNIERR_OUT_OF_RANGE,            // Value correctly encoded but outside of Unicode range.
    LXL_UNIERR_OVERLONG_ENCODING,       // Encoding is longer than necessary, possibly malicious.
};

// UTF-8 character stream
struct lxl_utf8_stream {
    struct lxl_string_view buffer;      // The backing buffer containing the characters for the stream.
    ptrdiff_t cursor;                   // The current position in the stream.
    enum lxl_unicode_error error;       // The latest error, cleared/set when the stream is advanced.
};


// END LEXEL ADDITIONAL.


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
    LXL_LERR_UNICODE = -22,           // A Unicode error.
};

// A pair of delimiters for strings and block comments, e.g. "/*" and "*/" for C-style comments.
struct lxl_delim_pair {
    struct lxl_string_view opener;
    struct lxl_string_view closer;
};

// A lexical token.
// The token's value is stored as a string (via the `start` and `end` pointers).
// Further processing of this value is left to the caller.
// The `kind` determines the type of the token. The meanings of different types
// is left to the caller, but negative types are reserved by lexel and have special
// meanings. For example, a value of -1 (see LXL_TOKENS_END) denotes the end of the
// token stream.
struct lxl_token {
    const char *start;          // The start of the token.
    const char *end;            // The end of the token.
    struct lxl_location loc;    // The location (line, column) of the token in the source.
    int kind;                   // The type of the lexical token. Negative values have special meanings.
};

// The main lexer object.
struct lxl_lexer {
    // Lexer state.
    struct lxl_utf8_stream stream;          // Source code stream.
    struct lxl_token token;                 // The next token to be emitted.
    const char *line_start;                 // Pointer to the beginning of the current line.
    enum lxl_lex_error error;               // Error code set to the current lexing error.
    int line;                               // The current line number.
    bool is_finished;                       // Flag set when the lexer has no more (non-end) tokens to emit.
    void (*next_state)(struct lxl_lexer *self);  // Pointer to the next state function of the lexer.

    // Query functions (determine token type).
    bool (*match_word_init_char)(struct lxl_lexer *self);   // Match the FIRST character of a word.
    // -- Default: forward to `.match_word_char()`.
    bool (*match_word_char)(struct lxl_lexer *self);        // Match a single word-constituent character.
    // -- Default: match any non-whitespace character.
    bool (*match_int_prefix)(struct lxl_lexer *self);       // Match an integer literal prefix (e.g. `0x`).
    // -- Default: always return false.
    bool (*match_int_digit)(struct lxl_lexer *self);        // Match a single integer digit.
    // -- Default: match digits `0`-`9`.
    bool (*match_float_prefix)(struct lxl_lexer *self);     // Match a floating-point literal prefix.
    // -- Default: always return false.
    bool (*match_float_digit)(struct lxl_lexer *self);      // Match a single floating-point digit.
    // -- Default: forward to `.match_integer_digit()`.
    bool (*match_punct_char)(struct lxl_lexer *self);       // Match a single punctuation character.
    // -- Default: always return false.
    int (*get_word_type)(struct lxl_lexer *self);           // Get the type of the current word token.
    int (*get_int_type)(struct lxl_lexer *self);
    int (*get_float_type)(struct lxl_lexer *self);
    int (*get_punct_type)(struct lxl_lexer *self);

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

    // Extensions (not used by lexel directly).
    void *custom_info;  // Pointer to any additional user-defined data. Can be left NULL if unneeded.
};

// END LEXEL CORE.


// LEXEL MAGIC VALUES (MVs).

// These enums define human-friendly names for the various magic values used by lexel.

enum lxl__token_mvs {
    LXL_TOKENS_END = -1,           // Special token type signifying the end of the token stream.
    LXL_TOKEN_UNINIT = -2,         // Special token type for a token whose type is yet to be determined.
    // LXL_TOKENS_END_ABNORMAL = -3,  // Special token type signifying an abnormal end of the token stream.
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


// LEXER PUBLIC INTERFACE.

// These are the functions and definitions designed to be called externally on the lexer.

// Return whether the lexer has reached the end of its input.
bool lxl_lexer_is_finished(const struct lxl_lexer *lexer);

// Return the next token in the lexer's token stream.
// Once the lexer has reached the end of its input, end tokens (token value LXL_TOKENS_END) are returned.
struct lxl_token lxl_lexer_next_token(struct lxl_lexer *lexer);

// END LEXER PUBLIC INTERFACE.


// LEXER STATES.

// These are the standard state functions used by the lexer.
// Lexer state functions encapsulate both the IDENTITY and ACTION of a state.
// They should not be directly called by the lexer (with the exception of the main event loop),
// but instead should set the next state to be entered upon completion. The aforementioned event
// loop (in lxl_lexer_next_token()) will then dispatch to the next state function until the Return
// state is encountered, whence the lexer will finalise the token and return it to the caller.
// N.B., the Return state will leave the lexer in the Ready state while control has returned to the
// caller, making this also the initial state when the lexer is called again.

// Standard lexer state function signifying that the lexer is ready to begin lexing the next token.
void lxl_lstate_Ready(struct lxl_lexer *lexer);

// Standard lexer state function signifying that the lexer should start a new token.
void lxl_lstate_BeginToken(struct lxl_lexer *self);

// Standard lexer state function signifying that the lexer should emit an end token.
void lxl_lstate_EmitEndToken(struct lxl_lexer *self);

// Standard lexer state function signifiying that the lexer should return to the caller.
void lxl_lstate_Return(struct lxl_lexer *lexer);

// END LEXER STATES.


// LEXER INTERNAL INTERFACE.

// These are the functions and definitions used internally by the lexer.
// They are exposed here to make extending the lexer easier.

// Call the given hook function on the lexer if it exists, or do nothing if it doesn't.
#define LXL_LEXER__CALL_HOOK(LEXER, HOOK) \
    (((LEXER)->(HOOK)) ? ((LEXER)->(HOOK)(LEXER)) : ((void)0))

// Advance the lexer by a single character and return the codepoint.
lxl_UnicodeCodepoint lxl_lexer__advance(struct lxl_lexer *lexer);

// Rewind the lexer by a single character.
void lxl_lexer__rewind(struct lxl_lexer *lexer);

// Rewind to the start of the current line.
void lxl_lexer__reset_line(struct lxl_lexer *lexer);

// Get a pointer to the start of the current line.
const char *lxl_lexer__seek_line_start(struct lxl_lexer *lexer);

// Get the distance from the start of the current line.
int lxl_lexer__get_column(struct lxl_lexer *lexer);

// Match one of a set of characters.
bool lxl_lexer__match_chars(struct lxl_lexer *lexer, struct lxl_string_view chars);

// Match a whole string.
bool lxl_lexer__match_string(struct lxl_lexer *lexer, struct lxl_string_view string);

// END LEXER INTERNAL INTERFACE.


// TOKEN INTERFACE.

// These functions are for working with tokens.

// Return whether `tok` is a special end-of-tokens token.
#define LXL_TOKEN_IS_END(tok) \
    ((tok).kind == LXL_TOKENS_END)

// Return whether `tok` is a special error token.
#define LXL_TOKEN_IS_ERROR(tok) ((tok).kind <= LXL_LERR_GENERIC)

// Return the token's value as a string view.
struct lxl_string_view lxl_token_value(struct lxl_token token);

// Return a textual representation of the error code.
struct lxl_string_view lxl_error_message(enum lxl_lex_error error);

// END TOKEN INTERFACE.


// STRING VIEW INTERFACE.

// Get a sting view from a string literal.
#define LXL_SV_FROM_STRLIT(STRLIT)              \
    lxl_sv_from_startlen(STRLIT, sizeof STRLIT - 1)

// Get a string view from a null-terminated string. NOTE: for string literals, use LXL_SV_FROM_STRLIT().
struct lxl_string_view lxl_sv_from_cstring(const char *string);

// Get a string view from a start pointer and length.
struct lxl_string_view lxl_sv_from_startlen(const char *start, ptrdiff_t length);

// Get a string view from a start pointer and an end pointer (one past the end).
struct lxl_string_view lxl_sv_from_startend(const char *start, const char *end);

// Get a pointer to one past the end of a string view.
const char *lxl_sv_end(const struct lxl_string_view *sv);

// Fold a possibly negative index into the range 0 ..<= sv.length (aka the "nominal range").
ptrdiff_t lxl_sv_normalise_index(const struct lxl_string_view *sv, ptrdiff_t index);

// Check if an index is in the nominal range, 0 ..<= sv.length, for a string view.
// Note the inclusion of the index one past the end of the string view.
bool lxl_sv_index_in_nominal_range(const struct lxl_string_view *sv, ptrdiff_t index);

// Check is an index is in the proper range, 0 ..< sv.length, for a string view.
// All elements inside the view fall in this range, with the position one past the end explicitly excluded.
bool lxl_sv_index_in_proper_range(const struct lxl_string_view *sv, ptrdiff_t index);

// Return a slice of the string view in the range from ..< to.
struct lxl_string_view lxl_sv_slice(const struct lxl_string_view *sv, ptrdiff_t from, ptrdiff_t to);

// Return a slice from the given index up to the end of a string view (from ..< sv.length).
struct lxl_string_view lxl_sv_slice_end(const struct lxl_string_view *sv, ptrdiff_t from);

// Return a slice from the start of a string view up to the given index (0 ..< to).
struct lxl_string_view lxl_sv_slice_start(const struct lxl_string_view *sv, ptrdiff_t to);

// END STRING VIEW INTERFACE.


// UNICODE INTERFACE.

// These functions are for working with Unicode/UTF-8, which is what lexel expects.

// Get a pointer to the current position in a UTF-8 stream.
const char *lxl_utf8_stream_position(const struct lxl_utf8_stream *stream);

// Get the tail (unconsumed characters) of a UTF-8 stream as a string view.
struct lxl_string_view lxl_utf8_stream_tail(const struct lxl_utf8_stream *stream);

// Return whether or not the stream is exhausted.
bool lxl_utf8_stream_is_finished(const struct lxl_utf8_stream *stream);

// Count the number of leading ones in a byte.
int lxl_count_leading_ones(uint8_t byte);

// Get the (minimum) number of characters needed to encode a codepoint value in UTF-8.
int lxl_get_utf8_length(lxl_UnicodeCodepoint value);

// Decode the next UTF-8 character in a stream, advance the stream, and return the character that was read.
// The error status is set or cleared based on the first error encuntered during decoding.
// On an error, the stream synchronises itself to the next valid codepoint boundary.
// If either of the errors LXL_UNIERR_OUT_OF_RANGE or LXL_UNIERR_OVERLONG_ENCODING are encountered,
// the decoded value is still returned (although the error status is set appropriately). On any
// other error, a value of 0 is returned
lxl_UnicodeCodepoint lxl_utf8_stream_advance(struct lxl_utf8_stream *stream);

// Rewind a UTF-8 stream by one codepoint. Returns false on error, true otherwise.
bool lxl_utf8_stream_rewind(struct lxl_utf8_stream *stream);

// END UNICODE INTERFACE.


#endif  // LEXEL_H

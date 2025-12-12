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

#ifndef LEXEL_H
#define LEXEL_H

#include <assert.h>     // assert(), static_assert()  -- requires C11
#include <stdbool.h>    // bool, false, true -- requires C99
#include <stddef.h>     // ptrdiff_t

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

// END META-DEFINITIONS.


// LEXEL ADDITIONAL.

// Additional definitions beyond the core below.

// A string view.
// This is a read-only (non-owning) view into a string, consiting of a `start` pointer and `length`.
struct lxl_string_view {
    const char *start;
    ptrdiff_t length;
};

// A single UTF-8 codepoint.
typedef uint32_t lxl_unicode_codepoint;

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
struct lxl_unicode_utf8_stream {
    struct lxl_string_view buffer;      // The backing buffer containing the characters for the stream.
    ptrdiff_t cursor;                   // The current position in the stream.
    enum lxl_unicode_error error;       // The latest error, cleared/set when the stream is advanced.
};


// END LEXEL ADDITIONAL.


// LEXEL CORE.

// These are the core definitions for lexel -- the lexer and token.


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
    LXL_LSTS_LEX_START,            // Lexing has begun.
    LXL_LSTS_LEXING_STRING,        // Lexing a string-like literal token.
    LXL_LSTS_FAIL_LEX_STRING,      // Failed to lex a string.
    LXL_LSTS_LEXING_INTEGER,       // Lexing an integer literal token.
    LXL_LSTS_FAIL_LEX_INTEGER,     // Failed to lex an integer.
    LXL_LSTS_LEXING_FLOAT,         // Lexing a floating-point literal token.
    LXL_LSTS_FAIL_LEX_FLOAT,       // Failed to lex a float.
    LXL_LSTS_LEXING_WORD,          // Lexing a word token.
    LXL_LSTS_FAIL_LEX_WORD,        // Failed to lex a word.
    LXL_LSTS_LEX_END,              // Token has been fully lexed.
    LXL_LSTS_FINISHED,             // Reached the end of tokens.
    LXL_LSTS_FINISHED_ABNORMAL,    // Reached the end of tokens abnormally.
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
    enum lxl_lexer_status status;           // Current status of the lexer.
    enum lxl_lex_error error;               // Error code set to the current lexing error.
    struct lxl_unicode_utf8_stream stream;  // Source code stream.
    const char *line_start;                 // Pointer to the beginning of the current line.
    int line;                               // The current line number.
    struct lxl_token token;                 // The next token to be emitted.

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
    bool (*match_punct_char)(stuct lxl_lexer *self);        // Match a single punctuation character.
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


// LEXER INTERFACE.

// Advance the lexer by a single character and return the codepoint.
lxl_unicode_codepoint lxl_lexer__advance(struct lxl_lexer *lexer);

// Rewind the lexer by a single character.
void lxl_lexer__rewind(struct lxl_lexer *lexer);

// Rewind to the start of the current line.
void lxl_lexer__reset_line(struct lxl_lexer *lexer);

// Match one of a set of characters.
bool lxl_lexer__match_chars(struct lxl_lexer *lexer, struct lxl_string_view chars);

// Match a whole string.
bool lxl_lexer__match_string(struct lxl_lexer *lexer, struct lxl_string_view string);

// END LEXER INTERFACE.


// TOKEN INTERFACE.

// These functions are for working with tokens.

// Return whether `tok` is a special end-of-tokens token.
#define LXL_TOKEN_IS_END(tok) \
    ((tok).kind == LXL_TOKENS_END || (tok).kind == LXL_TOKENS_END_ABNORMAL)

// Return whether `tok` is a special error token.
#define LXL_TOKEN_IS_ERROR(tok) ((tok).kind <= LXL_LERR_GENERIC)

// Return the token's value as a string view.
struct lxl_string_view lxl_token_value(struct lxl_token token);

// Return a textual representation of the error code.
struct lxl_string_view lxl_error_message(enum lxl_lex_error error);

// END TOKEN INTERFACE.


// STRING VIEW INTERFACE.

// Get a string view from a start pointer and length.
struct lxl_string_view lxl_sv_from_startlen(const char *start, ptrdiff_t length);

// Get a string view from a start pointer and an end pointer (one past the end).
struct lxl_string_view lxl_sv_from_startend(const char *start, const char *end);

// Get a pointer to one past the end of a string view.
const char *lxl_sv_end(const struct lxl_string_view *sv);

// Return a slice of the string view in the range [from, to).
struct string_view lxl_sv_slice(const struct lxl_string_view *sv, ptrdiff_t from, ptrdiff_t to);

// Return a slice from the given index up to the end of a string view.
struct string_view lxl_sv_slice_end(const struct lxl_string_view *sv, ptrdiff_t from);

// Return a slice from the start of a string view up to the given index.
struct string_view lxl_sv_slice_start(const struct lxl_strign_view *sv, ptrdiff_t to);

// END STRING VIEW INTERFACE.


// UNICODE INTERFACE.

// These functions are for working with Unicode/UTF-8, which is what lexel expects.

// Get the tail (unconsumed characters) of a UTF-8 stream as a string view.
struct lxl_string_view lxl_unicode_utf8_stream_tail(const struct lxl_unicode_utf8_stream *stream);

// Return whether or not the stream is exhausted.
bool lxl_unicode_utf8_stream_is_finished(const struct lxl_unicode_utf8_stream *stream);

// Count the number of leading ones in a byte.
int lxl_count_leading_ones(uint8_t byte);

// Get the (minimum) number of characters needed to encode a codepoint value in UTF-8.
int lxl_unicode_get_utf8_length(lxl_unicode_codepoint value);

// Decode the next UTF-8 character in a stream, advance the stream, and return the character that was read.
// The error status is set or cleared based on the first error encuntered during decoding.
// On an error, the stream synchronises itself to the next valid codepoint boundary.
// If either of the errors LXL_UNIERR_OUT_OF_RANGE or LXL_UNIERR_OVERLONG_ENCODING are encountered,
// the decoded value is still returned (although the error status is set appropriately). On any
// other error, a value of 0 is returned
lxl_unicode_codepoint lxl_unicode_next_utf8(struct lxl_unicode_utf8_stream *stream);

// END UNICODE INTERFACE.


#endif  // LEXEL_H

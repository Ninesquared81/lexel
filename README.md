# lexel

Lexel is a simple lexing library written in C.

It is designed to be widely applicable and easily extensible.

I am currently rewriting it from the ground up, so please bear with me.

## Quick Start

Lexel comes as a pair of .c and .h files. Only lexel.c and lexel.h are needed to use the library,
although tests and examples are also available if you download the entire repo.

Currently, there are no releases, so to install, either download the raw [lexel.c](https://raw.githubusercontent.com/Ninesquared81/lexel/refs/heads/overhaul/lexel.c) and [lexel.h](https://raw.githubusercontent.com/Ninesquared81/lexel/refs/heads/overhaul/lexel.h) files, or clone the entire repo.

## (Not) Building

Lexel is a simple library and thus does not have any complicated build process.
Just build lexel.c as part of your project. Lexel is designed to allow it to be
built as part of a unity build, so you can even just `#include "lexel.c"` in the
relevant file in your project.

Of course, lexel _can_ be built as a traditional static/dynamic library and subsequently
linked to, if you so desire.

## Library Overview

Lexel is a library for creating lexers. Lexel does not "generate" lexer code for you, but
instead runs a state machine which despatches to user-provided functions to recognise and
lex tokens. It is designed to have a simple core which is easily extended, allowing for
much flexibility in usage.

### The lexer

At the heart of lexel is the lexer object.

```c
struct lxl_lexer {
    // Lexer state.
    struct lxl_utf8_stream stream;
    struct lxl_token token;
    const char *line_start;
    enum lxl_lex_error error;
    int line;
    union {
        struct lxl_string_view last_string_opener;
        struct lxl_string_view last_block_comment_opener;
    };
    int block_comment_level;
    void (*next_state)(struct lxl_lexer *self);

    // Query functions.
    bool (*match_whitespace_char)(struct lxl_lexer *self);
    bool (*match_comment_line_opener)(struct lxl_lexer *self);
    bool (*match_comment_block_opener)(struct lxl_lexer *self);
    bool (*match_comment_block_closer)(struct lxl_lexer *self);
    bool (*match_comment_block_nest_opener)(struct lxl_lexer *self);
    bool (*match_comment_block_nest_closer)(struct lxl_lexer *self);
    bool (*match_word_init_char)(struct lxl_lexer *self);
    bool (*match_word_char)(struct lxl_lexer *self);
    bool (*match_int_prefix)(struct lxl_lexer *self);
    bool (*match_int_digit)(struct lxl_lexer *self);
    bool (*match_int_suffix)(struct lxl_lexer *self);
    bool (*match_float_prefix)(struct lxl_lexer *self);
    bool (*match_float_digit)(struct lxl_lexer *self);
    bool (*match_float_radix_sep)(struct lxl_lexer *self);
    bool (*match_float_exp_sep)(struct lxl_lexer *self);
    bool (*match_float_exp_sign)(struct lxl_lexer *self);
    bool (*match_float_suffix)(struct lxl_lexer *self);
    bool (*match_punct)(struct lxl_lexer *self);
    bool (*match_string_opener)(struct lxl_lexer *self);
    bool (*match_string_closer)(struct lxl_lexer *self);
    bool (*match_string_char)(struct lxl_lexer *self);

    // Token kind functions.
    int (*get_word_kind)(struct lxl_lexer *self);
    int (*get_int_kind)(struct lxl_lexer *self);
    int (*get_float_kind)(struct lxl_lexer *self);
    int (*get_punct_kind)(struct lxl_lexer *self);
    int (*get_string_kind)(struct lxl_lexer *self);

    // Hook functions (called at specific times).
    void (*before_token_hook)(struct lxl_lexer *self);
    void (*on_linefeed_hook)(struct lxl_lexer *self);
    void (*after_whitespace_hook)(struct lxl_lexer *self);
    void (*before_word_hook)(struct lxl_lexer *self);
    void (*after_word_hook)(struct lxl_lexer *self);
    void (*before_integer_hook)(struct lxl_lexer *self);
    void (*after_integer_hook)(struct lxl_lexer *self);
    void (*before_float_hook)(struct lxl_lexer *self);
    void (*before_float_frac_hook)(struct lxl_lexer *self);
    void (*before_float_exp_hook)(struct lxl_lexer *self);
    void (*after_float_hook)(struct lxl_lexer *self);
    void (*after_punct_hook)(struct lxl_lexer *self);
    void (*before_string_hook)(struct lxl_lexer *self);
    void (*after_string_hook)(struct lxl_lexer *self);
    void (*on_error_hook)(struct lxl_lexer *self);
    void (*before_error_token_hook)(struct lxl_lexer *self);
    void (*after_token_hook)(struct lxl_lexer *self);

    // Extensions (not used by lexel directly).
    void *custom_info;
};

```

This might look complicated, but it's quite simple, really.

#### Lexer state

The first group of fields are how the lexer keeps track of its position in the source
code, including enough information to reconstruct an exact line and column reference
for each token. The last of these fields, `.next_state`, is a pointer to a state function,
which is the main machinery of the lexer. The lexer uses this field in its main event loop
to determine which state to enter next, until it reaches the `Return` state, which signifies
that the lexer should return the token it has constructed to the caller. Each state function
sets the `.next_state` field before returning. The lexer comes with standard states which
are used for lexing. Additional user-defined states may be provided, but [hook functions](#hook-functions)
must be used to splice them into the state control flow.

#### Query functions

The next group of fields are called the "query functions". These user-provided functions are
used to determine how to lex certain tokens. All query functions have default behaviours
which may or may not be good enough for a user's needs. Note that the default behaviour for
`.match_word_char()` overshadows later query functions, matchin any non-whitespace tokens.

#### Hook functions

Next come the hook functions. These are the main source of customisation for lexel. Hook
functions are simply functions which are called at certain well-defined points in the
execution of the lexer. They take a pointer to the lexer (which they may modify) and
return `void`. All hook functions are optional. The names of hook functions are of the
form **before**/**on**/**after** _event_ **hook**.

Unlike query functions, hook functions exist soley to allow the user to interact with the
lexer. They do not send any data back to the lexer in and of themselves. See the [C lexer
example](examples/c_lexer.c) for a demonstration of hook functions in action.

#### Extensions

Finally, the last field in the lexer is `.custom_info`. This is a generic pointer not
used by the default lexer at all. It is provided in case a user requires additional
context when extending the lexer. Its meaning is left completely up to he user and
can be safely ignored if not required.

### Built-in lexer extensions

Lexel comes with a few handy built-in extensions covering some of the most common cases,
allowing users to use the built-in version rather than implementing them.

### Tokens

The lexer spits out tokens via calls to `lxl_lexer_next_token()`. Lexel's tokens store
pointers to the start and end of the token's contents, the token's source location
(line and column number), and an integer storing the token's kind.

```c
struct lxl_token {
    const char *start;
    const char *end;
    struct lxl_location loc;
    int kind;
};
```

The user is free to prescribe meanings to kind values of zero or greater, however,
negative kind values are reserved for use by lexel and can have special meanings.

For example, a kind of `-1` is used for the sentinel "end of tokens" token, while
a kind of `-2` represents an uninitalised token. If the user does not specify a
type for a token, it will have a value of `-2` (apart from the aforementioned
"end of tokens" token). A value of `-16` or less represents a lexing error, with
specific values for each error, listed below:

```c
enum lxl_lex_error {
    LXL_LERR_OK = 0,
    LXL_LERR_GENERIC = -16,
    LXL_LERR_EOF = -17,
    LXL_LERR_UNCLOSED_BLOCK_COMMENT = -18,
    LXL_LERR_UNCLOSED_STRING = -19,
    LXL_LERR_INVALID_INTEGER = -20,
    LXL_LERR_INVALID_FLOAT = -21,
    LXL_LERR_UNICODE = -22,
    LXL_LERR_UNRECOGNISED_TOKEN = -23,
    LXL_LERR_INVALID_STRING_CHARACTER = -24,
};
```

A [string view](#string-view-interface) of a token's value can be obtained via the
`lxl_token_value()` function. Note that during lexing, the `.end` field of the token
will likely not be set yet (or rather, it will be set to the token's `.start`). When
in the lexer, the function `lxl_lexer__peek_token()` should be used to obtain a
string view of the current token as it stands up to the current position of the lexer.

### String view interface

Lexel comes with its own string view interface, which is used internally by the library
to store strings. A string view is a read-only pointer to string data along with a
byte length. Below is lexel's definition of a string view:

```c
struct lxl_string_view {
    const char *start;
    ptrdiff_t length;
};
```

You will notice that the length field is signed. This is an intentional design choice
for lexel since signed types generally behave more intuitively, even when they are
representing something which cannot be negative.

### UTF-8 stream interface

Lexel also comes with an interface for working with UTF-8 data, which is used by the
lexer. UTF-8 is the _only_ supported string encoding in lexel (apart from ASCII,
which is a subset of UTF-8). To use data with a different encoding, it must first be
re-encoded into UTF-8 before being fed into lexel.

The stream decodes the UTF-8 data codepoint-by-codepoint, via calls to
`lxl_utf8_stream_advance()`. The stream can also be rewound one codepoint
at a time via `lxl_utf8_stream_rewind()`. Both of these take a pointer to
a UTF-8 stream object, which is defined below:

```c
struct lxl_utf8_stream {
    struct lxl_string_view buffer;
    ptrdiff_t cursor;
    enum lxl_unicode_error error;
};
```

The last field is the latest Unicode decoding error, which is cleared/set when the stream
is advanced or rewinded. The error can be one of:

```c
enum lxl_unicode_error {
    LXL_UNIERR_OK = 0,
    LXL_UNIERR_UNEXPECTED_EOF,
    LXL_UNIERR_INVALID_FIRST_BYTE,
    LXL_UNIERR_INVALID_CONT_BYTE,
    LXL_UNIERR_UNEXPECTED_CONT_BYTE,
    LXL_UNIERR_MISSING_CONT_BYTE,
    LXL_UNIERR_OUT_OF_RANGE,
    LXL_UNIERR_OVERLONG_ENCODING,
};
```

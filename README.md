# lexel

Lexel is a simple lexing library written in C.

It is designed to be widely applicable and easily extensible.

I am currently rewriting it from the ground up, so please bear with me.

## Quick Start

Lexel comes as a pair of .c and .h files. Only lexel.c and lexel.h are needed to use the library,
although tests and examples are also available if you download the entire repo.

Currently, there are no releases, so to install, either download the raw [lexel.c](https://raw.githubusercontent.com/Ninesquared81/lexel/refs/heads/overhaul/lexel.c) and [lexel.h](https://raw.githubusercontent.com/Ninesquared81/lexel/refs/heads/overhaul/lexel.h) files.

## (Not) Building

Lexel is a simple library and thus does not have any complicated build process.
Just build lexel.c as part of your project. Lexel is designed to allow it to be
built as part of a unity build, so you can even just `#include "lexel.c"` in the
relevant file in your project.

## Library Overview

Lexel is a library for creating lexers. Lexel does not "generate" lexer code for you, but
instead runs a state machine which despatches to user-provided functions to recognise and
lex tokens.

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
    bool (*match_word_char)(struct lxl_lexer *self);
    bool (*match_int_prefix)(struct lxl_lexer *self);
    bool (*match_int_digit)(struct lxl_lexer *self);
    bool (*match_float_prefix)(struct lxl_lexer *self);
    bool (*match_float_digit)(struct lxl_lexer *self);
    bool (*match_float_radix_sep)(struct lxl_lexer *self);
    bool (*match_float_exp_sep)(struct lxl_lexer *self);
    bool (*match_float_exp_sign)(struct lxl_lexer *self);
    bool (*match_punct)(struct lxl_lexer *self);
    bool (*match_string_opener)(struct lxl_lexer *self);
    bool (*match_string_closer)(struct lxl_lexer *self);
    bool (*match_string_char)(struct lxl_lexer *self);

    // Token type functions.
    int (*get_word_type)(struct lxl_lexer *self);
    int (*get_int_type)(struct lxl_lexer *self);
    int (*get_float_type)(struct lxl_lexer *self);
    int (*get_punct_type)(struct lxl_lexer *self);
    int (*get_string_kind)(struct lxl_lexer *self);

    // Hook functions (called at specific times).
    void (*before_token_hook)(struct lxl_lexer *self);
    void (*on_linefeed_hook)(struct lxl_lexer *self);
    void (*after_whitespace_hook)(struct lxl_lexer *self);
    void (*before_integer_hook)(struct lxl_lexer *self);
    void (*after_integer_hook)(struct lxl_lexer *self);
    void (*before_float_hook)(struct lxl_lexer *self);
    void (*after_float_hook)(struct lxl_lexer *self);
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
used to determine how to lex certain tokens. All query function  have default behaviours
which may or may not be good enough for a user's needs.

#### Hook functions

Next come the hook functions. These are the main source of customisation for lexel. Hook
functions are simply functions which are called at certain well-defined points in the
execution of the lexer. They take a pointer to the lexer (which they may modify) and
return `void`. All hook functions are optional.

#### Extensions

Finally, the last field in the lexer is `.custom_info`. This is a generic pointer not
used by the default lexer at all. It is provided in case a user requires additional
context when extending the lexer. Its meaning is left completely up to he user and
can be safely ignored if not required.

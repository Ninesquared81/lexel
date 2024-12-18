# lexel
Lexel is a C library for lexing the source code of a (textual) computer languge, as well as an interface
to automatically generate a lexer based on lexing rules. Note that lexel is entirely C-based;
there's no mini-language to learn on top of the interface. Instead, lexers are generated entirely
programmatically through calls to lexel functions.

## Getting Started

Lexel is a header-only library. You should `#define LEXEL_IMPLEMENTATION` in exactly ONE place where
you include the header file, directly before the `#include` directive.

The repo also includes the file example.c, which demonstrates the usage of the library.

The library is currently in its very early days so many features are missing.

## External and internal interfaces

Lexel does not hide any details from the caller, but it separates the lexer API into two interfaces.
The external interface is for working with a lexer, whereas the internal interface is for writing a lexer.

## Other interfaces

Lexel also has a string view interface. A string view is an read-only view of a string with a pointer to
the start and a length. Lexel itself usually uses `start` and `end` pointers internally, but provides the
string view interface due to its utility outside of lexel. There are convenience functions for converting
between string views and `start`/`end` pointers.

## Lexing with lexel

To start using `lexel`, we must first create a lexer object. This can be done through the `lxl_lexer_new()`
function, which takes two pointer arguments: one is a pointer to the start of the source text, while the
other is a pointer to one past the end of the string. Alternatively, we can use `lxl_lexer_from_sv()` to
create the lexer from a string view instead.

As an example, let us take the string `"1 2 +"`, which we want to lex. We can create the lexer like so:

    struct lxl_lexer lexer = lxl_lexer_from_sv(LXL_SV_FROM_STRLIT("1 2 +"));

The macro `LXL_SV_FROM_STRLIT()` creates a string view object from a C string literal. It uses `sizeof` to
calculate the length, so should only be used with an actual string literal.

We can now try out the lexer by calling the `lxl_lexer_next_token()` function:

    struct lxl_token token = lxl_lexer_next_token(&lexer);
    struct lxl_string_view value = lxl_token_value(token);
    printf("Token: '"LXL_SV_FMT_SPEC"' [type = %d]\n", LXL_SV_FMT_ARG(value), token.token_type);

We have used a couple of helper functions and macros here. Firstly, `lxl_token_value()` extracts the value
of a token as a string view, while the macros `LXL_SV_FMT_SPEC` and `LXL_SV_FMT_ARG()` allow use to print
string views in `printf()` and similar functions. The `LXL_SV_FMT_ARG()` macro evaluates its argument
multiple times, so be wary when using it with a complicated argument expression.

Running this code, we should get the output

    Token: '1' [type = -2]

What's going on here, then?

By default, the lexer emits tokens of the type `LXL_TOKEN_UNINIT` (value -2), with the rule that tokens
comprise symbolic (non-whitespace) characters with whitespace characters as token separators.
This may be good enough for the most basic of use cases, but lexel is capable of so much more.
We do, however, need to set up the lexer to recognise certain tokens.
Firstly, though, let's handle comments.

Comments in lexel are treated like whitespace. They can be used to separate tokens and do not themselves
constitute a token. Lexel supports three types of comment. Line comments can start anywhere on a line and run
to the end of the line. For example, `# Python comments` are of this type.
Unnestable block comments are enclosed within a pair of symbols, and, as the name suggests, cannot be nested
within themselves. For example, `/* C comments */` are of this type.
Nestable block comments are, as before, enclosed within a pair of symbols, but, unlike before,
these comments _can_ be nested. For example, `/+ D comments +/` are of this type.

Lexel allows arbitrarily many styles for each type of comment. By default, there are _no_ comments of any type
(lexel does not like to make assumptions), but we can change that by setting the appropriate fields within the
`lexer` structure.

For line comments, we have the field `.line_comment_openers`, which holds a NULL-terminated array
(via pointer) of strings each holding the opening symbol for a particular style of line comment.
For block comments, we have the fields `.nestable_comment_delims` and `.unnestable_comment_delims` for
nestable and unnestable block comments, respectively. These fields each hold a NULL-terminated array
(via pointer) of `delim_pair` structures, which is a pair of strings for the opening and closing delimeter
for a particular blokc comment style. To remove ambiguity, it is important that nestable and unnestable block
comments do not share the same openers or closers.

Each of these three fields may be left as `NULL` if there are to be no comments of that type.

Going back to our example, let's add Python-like line comments starting with `#`. We can simply add the
following line just after we create the lexer:

    lexer.line_comment_openers = (const char *[]){"#", NULL};

If you are unfamiliar with C99's compound literals, we are essentially creating an array of strings on the
fly and using it to set the `.line_comment_openers` field. As described above, we need to make sure this
array is NULL-terminated.

We should also update the source code to use one of these comments:

    struct lxl_lexer lexer = lxl_lexer_from_sv(LXL_SV_FROM_STRLIT("#1 2 +\n3");

Now when we run the code, we get

    Token: '3' [type = -2]

The entire first line (`1 2 +`) is skipped since it is in a comment.

We could also add C-style `/* unnestable */` comments.

    lexer.unnestable_comment_delims = (struct delim_pair[]){{"/*", "*/"}, {0}};

Here, we use `{0}` to NULL-terminate the array.

Let's change the source code again:

    struct lxl_lexer lexer - lxl_lexer_from_sv(LXL_SV_FROM_STRLIT("/*1 /*2*/ +\n3");

Now, we get

    Token: '+' [type = -2]

Notice how the comment is terminated after the 2, even though we have another opener inside the comment.
This is because we used an unnestable comment. What if we changed `/* */` to be nestable instead?

    lexer.nestable_comment_delims = (struct delim_pair[]){{"/*", "*/"}, {0}};

Using the same source code, we now get

    Token: '' [type = -18]

Huh? What's going on here? We seem to have an empty token with the type -18. We've only encountered -2 as
a token type so far. This must have some special meaning.

What we have here is an error token. Error tokens are emitted whenever the lexer encounters an error during
lexing. They're easy to recognise as they token type will always be -16 or lower. The type maps to an error
code defined in the `lxl_lex_error` enum. We could look at this enum to see the mnemonic for the error code,
but we also have the function `lxl_error_message()` available, which gives us a human-readable string
describing the error.

    printf("Error: %s.\n", lxl_error_message(token,token_type));

Now we see

    Error: Unclosed block comment.

This makes sense, since with a nestable comment, each `/*` must have a corresponding `*/`. In our example, we
have two `/*` but only one `*/`, so the lexer scans until the end of its input to find the second `*/`.
Since it finds none, it reports an error.

If we were to call `lxl_lexer_next_token()` again and then print the token as before, we'd get

    Token: '' [type = -1]

A token of type -1 (`LXL_TOKENS_END`) is a special token marking the end of the token stream. If we keep
calling `lxl_lexer_next_token()` now, we will keep receiving an `LXL_TOKENS_END` token.

If ever we need to re-lex the source code, we can reset the lexer to the beginning using `lxl_lexer_reset()`.
We can also rewind the lexer by 1 or more characters using `lxl_lexer__rewind()`/`lxl_lexer__rewind_by()`,
but these are part of the internal interface and since they work on the character level, could set the lexer
to be in the middle of a token instead of at the start/end of a token, leading to possibly confusing results.
It's best to use the external interface when calling `lxl_next_token()` and only poke around in the lexer
internals when absolutely necessary.

Let's consolidate all we've done so far and put the lexing into a loop so whe can lex the whole input.

    struct lxl_lexer lexer = lxl_lexer_from_sv(SV_FROMT_STRLIT("/*1*/ 2 #+\n3 /* unclosed");
    lexer.line_comment_openers = (const char *[]){"#", NULL};
    lexer.nestable_comment_delims = (struct delim_pair[]){{"/*", "*/"}, {0}};
    struct lxl_token token = {0};
    while (!LXL_TOKEN_IS_END(token = lxl_lexer_next_token(&lexer))) {
        struct string_view = lxl_token_value(token);
        printf("Token: '"LXL_SV_FMT_SPEC" [type = %d]\n", LXL_SV_FMT_ARG(value), token.token_type);
        if (LXL_TOKEN_IS_ERROR(token)) {
            printf("Error: %s.\n", lxl_error_message(token.token_type));
        }
    }

This has the output

    Token: '2' [type = -2]
    Token: '3' [type = -2]
    Token: '' [type = -18]
    Error: Unclosed block comment.

## Naming conventions

All lexel identifiers start with the `lxl_` prefix
(macros/enum constants are in capitals, functions are lowercase). Any identifier starting with such a
prefix (case-insensitive) is reserved by lexel so should not be used by callers.

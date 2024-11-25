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

## Naming conventions

All lexel identifiers start with the `lxl_` prefix
(macros/enum constants are in capitals, functions are lowercase). Any identifier starting with such a
prefix (case-insensitive) is reserved by lexel so should not be used by callers.

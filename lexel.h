#ifndef LEXEL_H
#define LEXEL_H


// LEXEL CORE.

// These are the core definitions for lexel -- the lexer and token.

struct lxl_lexer {
    const char *start;  // The start of the lexer's source code.
    const char *end;    // The end of the lexer's source code.
};

struct lxl_token {
    const char *start;  // The start of the token.
    const char *end;    // The end of the token.
    int token_type;     // The type of the lexical token. Negative values have special menanings.
};

// END LEXEL CORE.


// LEXEL MAGIC VALUES (MVs).

// These enums define human-friendly names for the various magic values used by lexel.

enum lxl__token_mvs {
    LXL_TOKENS_END = -1,
};

// END LEXEL MAGIC VALUES.


// LEXER EXTERNAL INTERFACE.

// These functions are for communicating with the lexer, e.g. when parsing.

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

#endif  // LEXEL_H

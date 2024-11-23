#ifndef LEXEL_H
#define LEXEL_H

struct lxl_lexer {
    const char *start;  // The start of the lexer's source code.
    const char *end;    // The end of the lexer's source code.
};

struct lxl_token {
    const char *start;  // The start of the token.
    const char *end;    // The end of the token.
    int token_type;     // The type of the lexical token. Negative values have special menanings.
};

// Lexel magic values (mvs).
// These enums define human-friendly names for the various magic values used by lexel.
enum lxl__token_mvs {
    LXL_TOKENS_END = -1,
};

struct lxl_token lxl_next_token(struct lxl_lexer *lexer);

#endif

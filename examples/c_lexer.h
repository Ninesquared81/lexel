#ifndef LXL_EX_C_LEXER_H
#define LXL_EX_C_LEXER_H

#define C_TOKENS_DECLARE(CTOK, ...)             \
    CTOK __VA_OPT__(= __VA_ARGS__),

#define C_TOKENS_STRING_TABLE(CTOK, ...)        \
    [CTOK] = LXL_SV_FROM_STRLIT_INIT(#CTOK),

#define C_TOKENS(X)                                                                             \
    /* Indeintifer and literals. */                                                             \
    X(CTOK_IDENTIFIER)                    /* Identifer: a, hello_there, ... */                  \
    X(CTOK_LIT_INT)                       /* Integer literal: 42, 0xFF, 0ul, ... */             \
    X(CTOK_LIT_FLOAT)                     /* Floating-point literal: 5.0, 3.14f, ... */         \
    X(CTOK_LIT_CHAR)                      /* Character literal: 'H', 'i', ... */                \
    X(CTOK_LIT_STRING)                    /* String literal: "Hello", "  world \xff", ... */    \
    /* Brackets (equal to their ASCII values). */                                               \
    X(CTOK_BKT_ROUND_LEFT, '(')           /* Left round bracket (parenthesis) */                \
    X(CTOK_BKT_ROUND_RIGHT, ')')          /* Right round bracket (parenthesis) */               \
    X(CTOK_BKT_CURLY_LEFT, '{')           /* Left curly bracket (brace) */                      \
    X(CTOK_BKT_CURLY_RIGHT, '}')          /* Right curly bracket (brace) */                     \
    X(CTOK_BKT_SQUARE_LEFT, '[')          /* Left square bracket */                             \
    X(CTOK_BKT_SQUARE_RIGHT, ']')         /* Right square bracket */                            \
    /* Single-character operators (equal to their ASCII values). */                             \
    X(CTOK_BANG, '!')                     /* Logical not */                                     \
    X(CTOK_PERCENT, '%')                  /* Remainder */                                       \
    X(CTOK_AMPERSAND, '&')                /* Address of, bitwise and */                         \
    X(CTOK_ASTERISK, '*')                 /* Dereference, multiply */                           \
    X(CTOK_PLUS, '+')                     /* Add */                                             \
    X(CTOK_COMMA, ',')                    /* Comma operator */                                  \
    X(CTOK_MINUS, '-')                    /* Subtract */                                        \
    X(CTOK_DOT, '.')                      /* Member access */                                   \
    X(CTOK_SLASH, '/')                    /* Divide */                                          \
    X(CTOK_COLON, ':')                    /* "Else" part of ternary ?:, end of label. */        \
    X(CTOK_SEMICOLON, ';')                /* Terminate statement */                             \
    X(CTOK_LT, '<')                       /* Less than */                                       \
    X(CTOK_EQ, '=')                       /* Assign */                                          \
    X(CTOK_GT, '>')                       /* Greater than */                                    \
    X(CTOK_QMARK, '?')                    /* "Then" part of ternary ?: */                       \
    X(CTOK_BACKSLASH, '\\')               /* Conitnue line */                                   \
    X(CTOK_CARET, '^')                    /* Bitwise exclusive or */                            \
    X(CTOK_VBAR, '|')                     /* Bitwise or */                                      \
    X(CTOK_TILDE, '~')                    /* Bitwise not */                                     \
    /* Multi-character operators. */                                                            \
    X(CTOK_BANG_EQ)                       /* !=  Not equal to */                                \
    X(CTOK_PERCENT_EQ)                    /* %=  Assign by remainder */                         \
    X(CTOK_AMPERSAND_AMPERSAND)           /* &&  Logical and */                                 \
    X(CTOK_AMPERSAND_EQ)                  /* &=  Assign by bitwise and */                       \
    X(CTOK_ASTERISK_EQ)                   /* *=  Assign by multiplication */                    \
    X(CTOK_PLUS_PLUS)                     /* ++  Pre/post increment */                          \
    X(CTOK_PLUS_EQ)                       /* +=  Assign by addition */                          \
    X(CTOK_MINUS_MINUS)                   /* --  Pre-post decrement */                          \
    X(CTOK_MINUS_EQ)                      /* -=  Assign by subtraction */                       \
    X(CTOK_ARROW)                         /* ->  Member access by pointer */                    \
    X(CTOK_ELIPSIS)                       /* ... Variadic arguments */                          \
    X(CTOK_SLASH_EQ)                      /* /=  Assign by division */                          \
    X(CTOK_LT_LT)                         /* <<  Left bit shift */                              \
    X(CTOK_LT_LT_EQ)                      /* <<= Assign by left bit shift */                    \
    X(CTOK_LT_EQ)                         /* <=  Less than or equal to */                       \
    X(CTOK_EQ_EQ)                         /* ==  Equal to */                                    \
    X(CTOK_GT_EQ)                         /* >=  Greater than or equal to */                    \
    X(CTOK_GT_GT)                         /* >>  Rigth bit shift */                             \
    X(CTOK_GT_GT_EQ)                      /* >>= Assign by right bit shift */                   \
    X(CTOK_CARET_EQ)                      /* ^=  Assign by bitwise exclusive or */              \
    X(CTOK_VBAR_EQ)                       /* |=  Assign by bitwise or */                        \
    X(CTOK_VBAR_VBAR)                     /* ||  Logical or */                                  \
    /* Keywords. */                                                                             \
    X(CTOK_KW_ALIGNAS)                    /* alignas, _Alignas */                               \
    X(CTOK_KW_ALIGNOF)                    /* alignof, _Alignof */                               \
    X(CTOK_KW_ATOMIC)                     /* _Atomic */                                         \
    X(CTOK_KW_AUTO)                       /* auto */                                            \
    X(CTOK_KW_BITINT)                     /* _BitInt */                                         \
    X(CTOK_KW_BOOL)                       /* _Bool */                                           \
    X(CTOK_KW_BREAK)                      /* break */                                           \
    X(CTOK_KW_COMPLEX)                    /* _Complex */                                        \
    X(CTOK_KW_CASE)                       /* case */                                            \
    X(CTOK_KW_CHAR)                       /* char */                                            \
    X(CTOK_KW_CONST)                      /* const */                                           \
    X(CTOK_KW_CONSTEXPR)                  /* constexpr */                                       \
    X(CTOK_KW_CONTINUE)                   /* continue */                                        \
    X(CTOK_KW_DECIMAL128)                 /* _Decimal128 */                                     \
    X(CTOK_KW_DECIMAL32)                  /* _Decimal32 */                                      \
    X(CTOK_KW_DECIMAL64)                  /* _Decimal64 */                                      \
    X(CTOK_KW_DEFAULT)                    /* default */                                         \
    X(CTOK_KW_DO)                         /* do */                                              \
    X(CTOK_KW_DOUBLE)                     /* double */                                          \
    X(CTOK_KW_ELSE)                       /* else */                                            \
    X(CTOK_KW_ENUM)                       /* enum */                                            \
    X(CTOK_KW_EXTERN)                     /* extern */                                          \
    X(CTOK_KW_FALSE)                      /* false */                                           \
    X(CTOK_KW_FLOAT)                      /* float */                                           \
    X(CTOK_KW_FOR)                        /* for */                                             \
    X(CTOK_KW_GENERIC)                    /* _Generic */                                        \
    X(CTOK_KW_GOTO)                       /* goto */                                            \
    X(CTOK_KW_IF)                         /* if */                                              \
    X(CTOK_KW_IMAGINARY)                  /* _Imaginary */                                      \
    X(CTOK_KW_INLINE)                     /* inline */                                          \
    X(CTOK_KW_INT)                        /* int */                                             \
    X(CTOK_KW_LONG)                       /* long */                                            \
    X(CTOK_KW_NORETURN)                   /* _Noreturn */                                       \
    X(CTOK_KW_NULLPTR)                    /* nullptr */                                         \
    X(CTOK_KW_REGISTER)                   /* register */                                        \
    X(CTOK_KW_RESTRICT)                   /* restrict */                                        \
    X(CTOK_KW_RETURN)                     /* return */                                          \
    X(CTOK_KW_SHORT)                      /* short */                                           \
    X(CTOK_KW_SIGNED)                     /* signed */                                          \
    X(CTOK_KW_SIZEOF)                     /* sizeof */                                          \
    X(CTOK_KW_STATIC)                     /* static */                                          \
    X(CTOK_KW_STATIC_ASSERT)              /* static_assert, _Static_Assert */                   \
    X(CTOK_KW_STRUCT)                     /* struct */                                          \
    X(CTOK_KW_SWITCH)                     /* switch */                                          \
    X(CTOK_KW_THREAD_LOCAL)               /* thread_local, _Thread_Local */                     \
    X(CTOK_KW_TRUE)                       /* true */                                            \
    X(CTOK_KW_TYPEDEF)                    /* typedef */                                         \
    X(CTOK_KW_TYPEOF)                     /* typeof */                                          \
    X(CTOK_KW_TYPEOF_UNQUAL)              /* typeof_unqual */                                   \
    X(CTOK_KW_UNION)                      /* union */                                           \
    X(CTOK_KW_UNSIGNED)                   /* unsigned */                                        \
    X(CTOK_KW_VOID)                       /* void */                                            \
    X(CTOK_KW_VOLATILE)                   /* volatile */                                        \
    X(CTOK_KW_WHILE)                      /* while */                                           \

enum c_token_type {
    C_TOKENS(C_TOKENS_DECLARE)
};

// Create a lexer capable of lexing C code.
struct lxl_lexer create_c_lexer(struct lxl_string_view src);

// Return a string view of the name for the given C token type.
struct lxl_string_view c_token_kind_name(enum c_token_type type);

// Escape a character.
const char *escape_char(char ch);

// Show a token.
void show_token(struct lxl_token token);

// Match an initial word-constituent character for C tokens -- [A-Za-z_].
bool match_word_init_char(struct lxl_lexer *self);

// Match a word-constituent character for C tokens -- for identifiers and keywords: [A-Za-z0-9_].
bool match_word_char(struct lxl_lexer *self);

// Match an integer literal prefix -- (digit) != 0: decimal, 0: octal, 0x/0X: hexadecimal, 0b/0B: binary.
bool match_int_prefix(struct lxl_lexer *self);

// Match a punct token completely.
bool match_punct(struct lxl_lexer *self);

int get_word_type(struct lxl_lexer *self);
int get_int_type(struct lxl_lexer *self);
int get_float_type(struct lxl_lexer *self);
int get_punct_type(struct lxl_lexer *self);
int get_string_type(struct lxl_lexer *self);

#endif

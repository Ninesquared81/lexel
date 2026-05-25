#ifndef LXL_EX_C_LEXER_H
#define LXL_EX_C_LEXER_H

enum c_token_type {
    // Indeintifer and literals.
    CTOK_IDENTIFIER,                    // Identifer: a, hello_there, ...
    CTOK_LIT_INT,                       // Integer literal: 42, 0xFF, 0ul, ...
    CTOK_LIT_FLOAT,                     // Floating-point literal: 5.0, 3.14f, ...
    CTOK_LIT_CHAR,                      // Character literal: 'H', 'i', ...
    CTOK_LIT_STRING,                    // String literal: "Hello", "  world\xff", ...
    // Brackets (equal to their ASCII values).
    CTOK_BKT_ROUND_LEFT                 = '(',  // Left round bracket (parenthesis)
    CTOK_BKT_ROUND_RIGHT                = ')',  // Right round bracket (parenthesis)
    CTOK_BKT_CURLY_LEFT                 = '{',  // Left curly bracket (brace)
    CTOK_BKT_CURLY_RIGHT                = '}',  // Right curly bracket (brace)
    CTOK_BKT_SQUARE_LEFT                = '[',  // Left square bracket
    CTOK_BKT_SQUARE_RIGHT               = ']',  // Right square bracket
    // Single-character operators (equal to their ASCII values).
    CTOK_BANG                           = '!',  // Logical not
    CTOK_PERCENT                        = '%',  // Remainder
    CTOK_AMPERSAND                      = '&',  // Address of, bitwise and
    CTOK_ASTERISK                       = '*',  // Dereference, multiply
    CTOK_PLUS                           = '+',  // Add
    CTOK_COMMA                          = ',',  // Comma operator
    CTOK_MINUS                          = '-',  // Subtract
    CTOK_DOT                            = '.',  // Member access
    CTOK_SLASH                          = '/',  // Divide
    CTOK_COLON                          = ':',  // "Else" part of ternary ?:, end of label.
    CTOK_SEMICOLON                      = ';',  // Terminate statement
    CTOK_LT                             = '<',  // Less than
    CTOK_EQ                             = '=',  // Assign
    CTOK_GT                             = '>',  // Greater than
    CTOK_QMARK                          = '?',  // "Then" part of ternary ?:
    CTOK_BACKSLASH                      = '\\', // Conitnue line
    CTOK_CARET                          = '^',  // Bitwise exclusive or
    CTOK_VBAR                           = '|',  // Bitwise or
    CTOK_TILDE                          = '~',  // Bitwise not
    // Multi-character operators.
    CTOK_BANG_EQ,                       // !=  Not equal to
    CTOK_PERCENT_EQ,                    // %=  Assign by remainder
    CTOK_AMPERSAND_AMPERSAND,           // &&  Logical and
    CTOK_AMPERSAND_EQ,                  // &=  Assign by bitwise and
    CTOK_ASTERISK_EQ,                   // *=  Assign by multiplication
    CTOK_PLUS_PLUS,                     // ++  Pre/post increment
    CTOK_PLUS_EQ,                       // +=  Assign by addition
    CTOK_MINUS_MINUS,                   // --  Pre-post decrement
    CTOK_MINUS_EQ,                      // -=  Assign by subtraction
    CTOK_ARROW,                         // ->  Member access by pointer
    CTOK_ELIPSIS,                       // ... Variadic arguments
    CTOK_SLASH_EQ,                      // /=  Assign by division
    CTOK_LT_LT,                         // <<  Left bit shift
    CTOK_LT_LT_EQ,                      // <<= Assign by left bit shift
    CTOK_LT_EQ,                         // <=  Less than or equal to
    CTOK_EQ_EQ,                         // ==  Equal to
    CTOK_GT_EQ,                         // >=  Greater than or equal to
    CTOK_GT_GT,                         // >>  Rigth bit shift
    CTOK_GT_GT_EQ,                      // >>= Assign by right bit shift
    CTOK_CARET_EQ,                      // ^=  Assign by bitwise exclusive or
    CTOK_VBAR_EQ,                       // |=  Assign by bitwise or
    CTOK_VBAR_VBAR,                     // ||  Logical or
    // Keywords.
    CTOK_KW_ALIGNAS,                    // alignas, _Alignas
    CTOK_KW_ALIGNOF,                    // alignof, _Alignof
    CTOK_KW_ATOMIC,                     // _Atomic
    CTOK_KW_AUTO,                       // auto
    CTOK_KW_BITINT,                     // _BitInt
    CTOK_KW_BOOL,                       // _Bool
    CTOK_KW_BREAK,                      // break
    CTOK_KW_COMPLEX,                    // _Complex
    CTOK_KW_CASE,                       // case
    CTOK_KW_CHAR,                       // char
    CTOK_KW_CONST,                      // const
    CTOK_KW_CONSTEXPR,                  // constexpr
    CTOK_KW_CONTINUE,                   // continue
    CTOK_KW_DECIMAL128,                 // _Decimal128
    CTOK_KW_DECIMAL32,                  // _Decimal32
    CTOK_KW_DECIMAL64,                  // _Decimal64
    CTOK_KW_DEFAULT,                    // default
    CTOK_KW_DO,                         // do
    CTOK_KW_DOUBLE,                     // double
    CTOK_KW_ELSE,                       // else
    CTOK_KW_ENUM,                       // enum
    CTOK_KW_EXTERN,                     // extern
    CTOK_KW_FALSE,                      // false
    CTOK_KW_FLOAT,                      // float
    CTOK_KW_FOR,                        // for
    CTOK_KW_GENERIC,                    // _Generic
    CTOK_KW_GOTO,                       // goto
    CTOK_KW_IF,                         // if
    CTOK_KW_IMAGINARY,                  // _Imaginary
    CTOK_KW_INLINE,                     // inline
    CTOK_KW_INT,                        // int
    CTOK_KW_LONG,                       // long
    CTOK_KW_NORETURN,                   // _Noreturn
    CTOK_KW_NULLPTR,                    // nullptr
    CTOK_KW_REGISTER,                   // register
    CTOK_KW_RESTRICT,                   // restrict
    CTOK_KW_RETURN,                     // return
    CTOK_KW_SHORT,                      // short
    CTOK_KW_SIGNED,                     // signed
    CTOK_KW_SIZEOF,                     // sizeof
    CTOK_KW_STATIC,                     // static
    CTOK_KW_STATIC_ASSERT,              // static_assert, _Static_Assert
    CTOK_KW_STRUCT,                     // struct
    CTOK_KW_SWITCH,                     // switch
    CTOK_KW_THREAD_LOCAL,               // thread_local, _Thread_Local
    CTOK_KW_TRUE,                       // true
    CTOK_KW_TYPEDEF,                    // typedef
    CTOK_KW_TYPEOF,                     // typeof
    CTOK_KW_TYPEOF_UNQUAL,              // typeof_unqual
    CTOK_KW_UNION,                      // union
    CTOK_KW_UNSIGNED,                   // unsigned
    CTOK_KW_VOID,                       // void
    CTOK_KW_VOLATILE,                   // volatile
    CTOK_KW_WHILE,                      // while
};

// Match a word-constituent character to C tokens -- for identifiers and keywords: [A-Za-z0-9_].
bool match_word_char(struct lxl_lexer *self);

// Match an integer literal prefix -- (digit) != 0: decimal, 0: octal, 0x/0X: hexadecimal, 0b/0B: binary.
bool match_int_prefix(struct lxl_lexer *self);

// Match an integer literal digit -- [0-9A-Fa-f']: this does not care about the prefix.
bool match_int_digit(struct lxl_lexer *self);

// Match a floating-point literal prefix : ., 0x for hexadecimal, or any digit for decimal.
bool match_float_prefix(struct lxl_lexer *self);

// Match a punct token completely.
bool match_punct(struct lxl_lexer *self);

int get_word_type(struct lxl_lexer *self);

int get_punct_type(struct lxl_lexer *self);

#endif

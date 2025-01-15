/*
 * This example demonstrates how to build and use a lexer with lexel for a sample language.
 * The structure of the language is not overly important since we're mostly concerned with the
 * lexical tokens (we're building a lexer not a parser).
 *
 * This language is a simple C-like language with alphanumeric identifiers, basic arithmetic operators,
 * curly brackets for block statements and round brackets for function calls and expression grouping.
 * We also have integer, float and string literals. Additionally, we have Python-style line comments
 * starting with `#`.
 *
 * A "Hello, World!" program in this language looks like this:
 *
 *     println("Hello, World!")
 *
 * Where `println()` is a standard library function.
 *
 * NOTE: at the time of writing, lexel cannot lex the language described above. As lexel becomes more
 * capable, this example will be updated accoringly.
 */

#include <stdio.h>

// Define this so that we get the function definitions as well as their declarations.
#define LEXEL_IMPLEMENTATION
#include "lexel.h"

// Here are our token types. They start at zero. Negative token types have special meanings and are
// reserved for use by lexel itself. Any non-negative token types are fair game, though, so we take
// the simplest approach of assigning them sequentially from zero.
enum token_type {
    TOKEN_ID,
    TOKEN_INT,
    TOKEN_FLOAT,
    TOKEN_STRING,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_COMMA,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_DEF,
};

// This is a helper function to print a token with its value and type.
void print_token(struct lxl_token token, int i) {
    struct lxl_string_view sv = lxl_token_value(token);
    printf("Token %d: '"LXL_SV_FMT_SPEC"' [type = %d]\n", i, LXL_SV_FMT_ARG(sv), token.token_type);
    // In the case of an error token, we also print the associated error message.
    if (LXL_TOKEN_IS_ERROR(token)) {
        printf("Error: %s.\n", lxl_error_message(token.token_type));
    }
}

// This is a helper function to print all the tokens in a lexer.
void print_tokens(struct lxl_lexer *lexer) {
    // This loop will iterate through the token stream once.
    for (int i = 0; !lxl_lexer_is_finished(lexer); ++i) {
        // This function is the heart of the lexer. We call it to get the next token in the stream.
        // The lexer uses the rules defined above the decide the type and length of the token.
        // The lexer may return a token with a negative type to communicate some message to the caller.
        // For instance, if there was an error during lexing, an error token is retured whose type is
        // set to the corresponding error code and whose value is the token being considered up to the
        // point the error ocurred.
        struct lxl_token token = lxl_lexer_next_token(lexer);
        print_token(token, i);
        // We keep looping until we exhaust the token stream.
    }
}

int main(void) {
    // Example source code.
    struct lxl_lexer lexer = lxl_lexer_from_sv(
        LXL_SV_FROM_STRLIT(
            "def hello() {\n"
            "    println(\"Hello, World!\")  # Greet the world.\n"
            "    println(\"2 and 2 are\", 2 + 2)\n"
            "}\n"
            ));
    // We define how our line comments should start in this NULL-terminated list.
    // Lexel supports multiple styles of line comment which can start with any sequence of characters
    // and run to the end of the line. We're keeping things simple, though, and using a single `#` to
    // start our line comments.
    lexer.line_comment_openers = (const char *[]) {"#", NULL};
    // NOTE: we can also use the macro LXL_LIST_STR() to automate this process:
    // lexer.line_comment_openers = LXL_LIST_STR("#");
    // The macro takes care of both the type (const char *) and appending NULL.
    // For the the sake of demonstration, we did it manually the first time, but we'll use
    // the macro approach in the future.

    // If we wanted to, we could also define a style for multiline comments. For now, though, we'll
    // stick to line comments only.

    // Now we handle integer literals. First, the token type.
    lexer.default_int_type = TOKEN_INT;
    // For now, we'll only consider base-10 (decimal) literals. Lexel can handle and base from, 2 to 36.
    // Note that by default, lexel will not try to recognise any sort of integer token, so we have to set
    // this field to "turn on" integer lexing.
    lexer.default_int_base = 10;
    // Strings are next. For now, we'll use double-quotes only to delimit strings.
    lexer.line_string_delims = LXL_LIST_DELIMS({"\"", "\""});
    // We'll allow `\` in string literals to start an escape sequence. Processing these sequences is left
    // up to the parser. The lexer doesn't even verify if the sequence is long enough. It will, however,
    // ignore a closing delimiter if preceeded by an escape character.
    lexer.string_escape_chars = "\\";
    // We set the token type for strings. Lexel allows each delimiter to be associated with a different
    // type. We only have one delimiter, though, so that point is somewhat moot. It is important to know,
    // however, that this array must be the same length as the `.string_delims` string.
    lexer.string_types = (int[]) {TOKEN_STRING};
    // Let's give it a try!
    // See print_tokens() above for the definition of this function.
    print_tokens(&lexer);
    // At the end of the token stream, the lexer emits a special "end of tokens" type token. This sentinal
    // token is emitted upon first reaching the end of the stream and on all subsequent calls to
    // `lxl_lexer__next_token()`. To demonstrate:
    print_token(lxl_lexer_next_token(&lexer), -1);
    print_token(lxl_lexer_next_token(&lexer), -1);
    // If we want to restart the token stream, we can call the following function:
    lxl_lexer_reset(&lexer);
    // If we look again at our output, we'll notice something odd.
    // For a start, the second token, which ought to just be 'hello' is in fact 'hello()'. It
    // has eaten the brackets. What's more, we have the frankly absurd token 'println("Hello,'. The
    // first part of the string (up to the first space) has been absorbed into the token.
    // This is partly becuase lexel doesn't know about '(' yet. It's also partly because lexel falls
    // back to "symbolic" tokens by default, which consist of any non-whitespace characters, including
    // string delimiters. If '"' were at the beginning of a token, everything would be fine, but because
    // it is found after '(', it is absorbed into the token. As long as lexel lexes '(' properly, we can
    // mitigate the issue for now.
    // Okay, let's do just that. We can use the .puncts member of the lexer to define a list of punctuation
    // token values:
    lexer.puncts = LXL_LIST_STR("+", ",", "(", ")", "{", "}");
    // And each has a corresponding type:
    lexer.punct_types = (int[]) {
        TOKEN_PLUS,
        TOKEN_COMMA,
        TOKEN_LPAREN, TOKEN_RPAREN,
        TOKEN_LBRACE, TOKEN_RBRACE,
    };
    // Let's try lexing again!
    print_tokens(&lexer);
    // Okay, we now correctly lex the '{', '+', ')', and '}', but the symbolic fall-back is still
    // getting in our way.
    // The fix is simple: we change the default word lexing stategy from "SYMBOLIC" to "WORD":
    lexer.word_lexing_rule = LXL_LEX_WORD;
    lxl_lexer_reset(&lexer);
    print_tokens(&lexer);
    // Cool, but notice that the token type for 'println' is -2 (LXL_TOKEN_UNINIT). We want it to be an
    // identifier. We can set the .default_word_type member for this.
    lexer.default_word_type = TOKEN_ID;
    lxl_lexer_reset(&lexer);
    print_tokens(&lexer);
    // Nice!
    // We still have one small problem with our lexer.
    // It can correctly recognise words, but it treats 'def' no differently from 'pritnln'. That might be
    // okay, but, as ever, lexel has more to offer: keywords. Keywords are just word tokens except they can
    // have their own unique token types.
    // Perhaps expectedly at this point, keywords are defined by two lists:
    lexer.keywords = LXL_LIST_STR("def");
    lexer.keyword_types = (int[]) {TOKEN_DEF};
    lxl_lexer_reset(&lexer);
    print_tokens(&lexer);
    // Lovely!
    return 0;
}

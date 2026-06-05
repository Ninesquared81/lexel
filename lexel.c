
#include <string.h>         // strlen.

#include "lexel.h"

// LEXER PUBLIC INTERFACE.

struct lxl_lexer lxl_lexer_new(struct lxl_string_view src) {
    struct lxl_lexer new_lexer = {
        .stream = {.buffer = src},
        .line_start = src.start,
        .next_state = lxl_lstate_Ready,
        /* All other fields zero/NULL. */
    };
    return new_lexer;
}

bool lxl_lexer_is_finished(const struct lxl_lexer *lexer) {
    return lxl_utf8_stream_is_finished(&lexer->stream);
}

struct lxl_token lxl_lexer_next_token(struct lxl_lexer *lexer) {
    if (lexer->next_state == NULL) lexer->next_state = lxl_lstate_Ready;
    LXL_ASSERT(lexer->next_state == lxl_lstate_Ready && "Lexer in unexpected state.");
    while (lexer->next_state != lxl_lstate_Return) {
        LXL_ASSERT(lexer->next_state != NULL && "Lexer state must never be null.");
        lexer->next_state(lexer);
    }
    lxl_lstate_Return(lexer);
    return lexer->token;
}

// END_LEXER_PUBLIC_INTERFACE.


// LEXER STATES.

void lxl_lstate_Ready(struct lxl_lexer *self) {
    LXL_LEXER__CALL_HOOK(self, before_token_hook);
    self->next_state = lxl_lstate_SkipWhitespace;
}

void lxl_lstate_SkipWhitespace(struct lxl_lexer *self) {
    if (lxl_lexer__match_comment_line_opener(self)) {
        self->next_state = lxl_lstate_SkipLineComment;
        return;
    }
    const char *opener_start = lxl_lexer__peek(self);
    if (lxl_lexer__match_comment_block_opener(self)) {
        const char *opener_end = lxl_lexer__peek(self);
        self->last_block_comment_opener = lxl_sv_from_startend(opener_start, opener_end);
        self->next_state = lxl_lstate_SkipBlockComment;
        return;
    }
    if (lxl_lexer__match_comment_block_nest_opener(self)) {
        const char *opener_end = lxl_lexer__peek(self);
        self->last_block_comment_opener = lxl_sv_from_startend(opener_start, opener_end);
        self->block_comment_level = 1;
        self->next_state = lxl_lstate_SkipNestableBlockComment;
        return;
    }
    if (!lxl_lexer__skip_whitespace_line(self)) {
        self->next_state = lxl_lstate_BeginToken;
        return;
    }
    if (lxl_lexer__peek(self)[-1] == '\n') {
        LXL_ASSERT(self->stream.cursor > 0);
        LXL_LEXER__CALL_HOOK(self, on_linefeed_hook);
        return;
    }
    // Keep skipping whitespace.
    LXL_ASSERT(self->next_state == lxl_lstate_SkipWhitespace);
}

void lxl_lstate_SkipLineComment(struct lxl_lexer *self) {
    if (lxl_lexer__skip_line(self) > 0 && !lxl_lexer_is_finished(self)) {
        LXL_ASSERT(self->stream.cursor > 0);
        LXL_ASSERT(lxl_lexer__peek(self)[-1] == '\n');
        // Handle newline in whitespace skipping.
        lxl_lexer__rewind(self);
    }
    // Continue skipping whitespace.
    self->next_state = lxl_lstate_SkipWhitespace;
}

void lxl_lstate_SkipBlockComment(struct lxl_lexer *self) {
    while (!lxl_lexer__match_comment_block_closer(self)) {
        if (lxl_lexer_is_finished(self)) {
            self->next_state = lxl_lstate_UnclosedBlockComment;
            return;
        }
        lxl_lexer__advance(self);
    }
    // Continue skipping whitespace.
    self->next_state = lxl_lstate_SkipWhitespace;
}

void lxl_lstate_SkipNestableBlockComment(struct lxl_lexer *self) {
    while (!lxl_lexer__match_comment_block_nest_closer(self)) {
        if (lxl_lexer_is_finished(self)) {
            self->next_state = lxl_lstate_UnclosedBlockComment;
            return;
        }
        if (lxl_lexer__match_comment_block_nest_opener(self)) {
            ++self->block_comment_level;
            return;
        }
        lxl_lexer__advance(self);
    }
    --self->block_comment_level;
    LXL_ASSERT(self->block_comment_level >= 0);
    if (self->block_comment_level == 0) {
        // Go back to skipping whitespace.
        self->next_state = lxl_lstate_SkipWhitespace;
    }
}

void lxl_lstate_UnclosedBlockComment(struct lxl_lexer *self) {
    self->next_state = lxl_lstate_Return;
    lxl_lexer__error(self, LXL_LERR_UNCLOSED_BLOCK_COMMENT);
}

void lxl_lstate_BeginToken(struct lxl_lexer *self) {
    LXL_LEXER__CALL_HOOK(self, after_whitespace_hook);
    lxl_lexer__begin_token(self);
    self->next_state = (!lxl_lexer_is_finished(self))
        ? lxl_lstate_LexWordToken
        : lxl_lstate_EmitEndToken;
}

void lxl_lstate_LexWordToken(struct lxl_lexer *self) {
    if (!lxl_lexer__match_word_init_char(self)) {
        self->next_state = lxl_lstate_LexIntToken;
        return;
    }
    while (lxl_lexer__match_word_char(self)) {
        /* Do nothing. */
    }
    self->next_state = lxl_lstate_EmitWordToken;
}

void lxl_lstate_LexIntToken(struct lxl_lexer *self) {
    if (!lxl_lexer__match_int_prefix(self)) {
        self->next_state = lxl_lstate_LexFloatStart;
        return;
    }
    LXL_LEXER__CALL_HOOK(self, before_integer_hook);
    while (lxl_lexer__match_int_digit(self)) {
        /* Do nothing. */
    }
    if (lxl_lexer__match_float_radix_sep(self)) {
        LXL_LEXER__CALL_HOOK(self, before_float_frac_hook);
        self->next_state = lxl_lstate_LexFloatPartFrac;
        return;
    }
    if (lxl_lexer__match_float_exp_sep(self)) {
        LXL_LEXER__CALL_HOOK(self, before_float_exp_hook);
        self->next_state = lxl_lstate_LexFloatPartExp;
        return;
    }
    lxl_lexer__match_int_suffix(self);
    self->next_state = lxl_lstate_EmitIntToken;
    LXL_LEXER__CALL_HOOK(self, after_integer_hook);
}

void lxl_lstate_LexFloatStart(struct lxl_lexer *self) {
    if (lxl_lexer__match_float_radix_sep(self)) {
        // Allow floats to start `.(digit)`.
        if (!lxl_lexer__match_float_digit(self)) {
            lxl_lexer__unlex(self);
            self->next_state = lxl_lstate_LexPunctToken;
            return;
        }
        self->next_state = lxl_lstate_LexFloatPartFrac;
        LXL_LEXER__CALL_HOOK(self, before_float_frac_hook);
        return;
    }
    if (!lxl_lexer__match_float_prefix(self)) {
        self->next_state = lxl_lstate_LexPunctToken;
        return;
    }
    self->next_state = lxl_lstate_LexFloatPartInt;
    LXL_LEXER__CALL_HOOK(self, before_float_hook);
}

void lxl_lstate_LexFloatPartInt(struct lxl_lexer *self) {
    while (lxl_lexer__match_float_digit(self)) {
        /* Do nothing. */
    }
    if (lxl_lexer__match_float_radix_sep(self)) {
        self->next_state = lxl_lstate_LexFloatPartFrac;
        LXL_LEXER__CALL_HOOK(self, before_float_frac_hook);
        return;
    }
    if (lxl_lexer__match_float_exp_sep(self)) {
        self->next_state = lxl_lstate_LexFloatPartExp;
        LXL_LEXER__CALL_HOOK(self, before_float_exp_hook);
        return;
    }
    self->next_state = lxl_lstate_LexFloatEnd;
}

void lxl_lstate_LexFloatPartFrac(struct lxl_lexer *self) {
    while (lxl_lexer__match_float_digit(self)) {
        /* Do nothing. */
    }
    if (lxl_lexer__match_float_exp_sep(self)) {
        self->next_state = lxl_lstate_LexFloatPartExp;
        LXL_LEXER__CALL_HOOK(self, before_float_exp_hook);
        return;
    }
    self->next_state = lxl_lstate_LexFloatEnd;
}

void lxl_lstate_LexFloatPartExp(struct lxl_lexer *self) {
    lxl_lexer__match_float_exp_sign(self);
    while (lxl_lexer__match_float_digit(self)) {
        /* Do nothing. */
    }
    self->next_state = lxl_lstate_LexFloatEnd;
}

void lxl_lstate_LexFloatEnd(struct lxl_lexer *self) {
    lxl_lexer__match_float_suffix(self);
    self->next_state = lxl_lstate_EmitFloatToken;
    LXL_LEXER__CALL_HOOK(self, after_float_hook);
}

void lxl_lstate_LexPunctToken(struct lxl_lexer *self) {
    bool success = lxl_lexer__match_punct(self);
    self->next_state = (success)
        ? lxl_lstate_EmitPunctToken
        : lxl_lstate_LexStringStart;
}

void lxl_lstate_LexStringStart(struct lxl_lexer *self) {
    const char *opener_start = lxl_lexer__peek(self);
    if (!lxl_lexer__match_string_opener(self)) {
        self->next_state = lxl_lstate_UnrecognisedToken;
        return;
    }
    const char *opener_end = lxl_lexer__peek(self);
    LXL_ASSERT(opener_end > opener_start);
    self->last_string_opener = lxl_sv_from_startend(opener_start, opener_end);
    self->next_state = lxl_lstate_LexStringContents;
}

void lxl_lstate_LexStringContents(struct lxl_lexer *self) {
    while (!lxl_lexer__match_string_closer(self)) {
        if (lxl_lexer_is_finished(self)) {
            self->next_state = lxl_lstate_UnclosedString;
            return;
        }
        if (!lxl_lexer__match_string_char(self)) {
            self->next_state = lxl_lstate_InvalidStringCharacter;
            return;
        }
    }
    self->next_state = lxl_lstate_EmitStringToken;
}

void lxl_lstate_UnrecognisedToken(struct lxl_lexer *self) {
    // Skip a single character.
    lxl_lexer__advance(self);
    self->next_state = lxl_lstate_Return;
    lxl_lexer__error(self, LXL_LERR_UNRECOGNISED_TOKEN);
}

void lxl_lstate_UnclosedString(struct lxl_lexer *self) {
    self->next_state = lxl_lstate_Return;
    lxl_lexer__error(self, LXL_LERR_UNCLOSED_STRING);
}

void lxl_lstate_InvalidStringCharacter(struct lxl_lexer *self) {
    self->next_state = lxl_lstate_Return;
    lxl_lexer__error(self, LXL_LERR_INVALID_STRING_CHARACTER);
}

void lxl_lstate_EmitEndToken(struct lxl_lexer *self) {
    LXL_ASSERT(lxl_lexer_is_finished(self));
    lxl_lexer__begin_token(self);
    self->token.kind = LXL_TOKENS_END;
    self->next_state = lxl_lstate_Return;
}

void lxl_lstate_EmitLineEndingToken(struct lxl_lexer *self) {
    lxl_lexer__begin_token(self);
    LXL_ASSERT(self->token.start > self->stream.buffer.start);
    self->token.start -= 1;
    self->token.kind = LXL_TOKEN_LINE_ENDING;
    self->next_state = lxl_lstate_Return;
}

void lxl_lstate_EmitWordToken(struct lxl_lexer *self) {
    self->token.kind = lxl_lexer__get_word_kind(self);
    self->next_state = lxl_lstate_Return;
}

void lxl_lstate_EmitIntToken(struct lxl_lexer *self) {
    self->token.kind = lxl_lexer__get_int_kind(self);
    self->next_state = lxl_lstate_Return;
}

void lxl_lstate_EmitFloatToken(struct lxl_lexer *self) {
    self->token.kind = lxl_lexer__get_float_kind(self);
    self->next_state = lxl_lstate_Return;
}

void lxl_lstate_EmitPunctToken(struct lxl_lexer *self) {
    self->token.kind = lxl_lexer__get_punct_kind(self);
    self->next_state = lxl_lstate_Return;
}

void lxl_lstate_EmitStringToken(struct lxl_lexer *self) {
    self->token.kind = lxl_lexer__get_string_kind(self);
    self->next_state = lxl_lstate_Return;
}

void lxl_lstate_Return(struct lxl_lexer *self) {
    self->next_state = lxl_lstate_Ready;
    if (self->error) {
        LXL_LEXER__CALL_HOOK(self, before_error_token_hook);
    }
    lxl_lexer__finish_token(self);
    LXL_LEXER__CALL_HOOK(self, after_token_hook);
    self->error = LXL_LERR_OK;  // Clear error.
}

// END LEXER STATES.


// LEXER INTERNAL INTERFACE.

void lxl_lexer__begin_token(struct lxl_lexer *lexer) {
    const char *src_pos = lxl_lexer__peek(lexer);
    lexer->token.start = src_pos;
    lexer->token.end = src_pos;
    lexer->token.loc = lxl_lexer__get_location(lexer);
    lexer->token.kind = LXL_TOKEN_UNINIT;
}

void lxl_lexer__finish_token(struct lxl_lexer *lexer) {
    lexer->token.end = lxl_lexer__peek(lexer);
    if (lexer->error) {
        lexer->token.kind = lexer->error;
    }
}

struct lxl_string_view lxl_lexer__peek_token(struct lxl_lexer *lexer) {
    const char *end = lxl_lexer__peek(lexer);
    return lxl_sv_from_startend(lexer->token.start, end);
}

void lxl_lexer__error(struct lxl_lexer *lexer, enum lxl_lex_error error) {
    lexer->error = error;
    LXL_LEXER__CALL_HOOK(lexer, on_error_hook);
}

const char *lxl_lexer__peek(struct lxl_lexer *lexer) {
    return lxl_utf8_stream_position(&lexer->stream);
}

lxl_UnicodeCodepoint lxl_lexer__advance(struct lxl_lexer *lexer) {
    if (lxl_lexer_is_finished(lexer)) return 0;
    LXL_ASSERT(!lxl_utf8_stream_is_finished(&lexer->stream));
    lxl_UnicodeCodepoint next = lxl_utf8_stream_advance(&lexer->stream);
    if (lexer->stream.error) {
        lxl_lexer__error(lexer, LXL_LERR_UNICODE);
    }
    if (next == '\n') ++lexer->line;
    return next;
}

void lxl_lexer__rewind(struct lxl_lexer *lexer) {
    if (!lxl_utf8_stream_rewind(&lexer->stream)) {
        lxl_lexer__error(lexer, LXL_LERR_UNICODE);
    }
    if (lexer->stream.buffer.start[lexer->stream.cursor] == '\n') {
        --lexer->line;
        lexer->line_start = lxl_lexer__seek_line_start(lexer);
    }
}

void lxl_lexer__unlex(struct lxl_lexer *lexer) {
    while (lxl_lexer__peek(lexer) > lexer->token.start) {
        lxl_lexer__rewind(lexer);
    }
}

void lxl_lexer__reset_line(struct lxl_lexer *lexer) {
    lxl_lexer__seek_line_start(lexer);
}

const char *lxl_lexer__seek_line_start(struct lxl_lexer *lexer) {
    int cursor = lexer->stream.cursor - 1;
    while (cursor >= 0 && lexer->stream.buffer.start[cursor] != '\n') {
        --cursor;
    }
    LXL_ASSERT(cursor + 1 >= 0);
    // Return address of byte one after the previous newline.
    return &lexer->stream.buffer.start[cursor + 1];
}

int lxl_lexer__get_column(struct lxl_lexer *lexer) {
    const char *stream_position = lxl_lexer__peek(lexer);
    return stream_position - lexer->line_start;
}

struct lxl_location lxl_lexer__get_location(struct lxl_lexer *lexer) {
    return (struct lxl_location) {
        .line = lexer->line,
        .column = lxl_lexer__get_column(lexer),
    };
}

ptrdiff_t lxl_lexer__skip_whitespace_line(struct lxl_lexer *lexer) {
    const char *skip_start = lxl_lexer__peek(lexer);
    while (lxl_lexer__match_whitespace_char(lexer)) {
        if (lexer->stream.cursor > 0 && lxl_lexer__peek(lexer)[-1] == '\n') {
            break;
        }
    }
    const char *skip_end = lxl_lexer__peek(lexer);
    return skip_end - skip_start;
}

ptrdiff_t lxl_lexer__skip_line(struct lxl_lexer *lexer) {
    const char *skip_start = lxl_lexer__peek(lexer);
    while (!lxl_lexer__match_string(lexer, LXL_SV_FROM_STRLIT("\n"))) {
        if (lxl_lexer_is_finished(lexer)) break;
        lxl_lexer__advance(lexer);
    }
    const char *skip_end = lxl_lexer__peek(lexer);
    return skip_end - skip_start;
}

bool lxl_lexer__match_chars(struct lxl_lexer *lexer, struct lxl_string_view chars) {
    if (lxl_lexer_is_finished(lexer)) return false;
    lxl_UnicodeCodepoint lexer_next = lxl_lexer__advance(lexer);
    struct lxl_utf8_stream chars_stream = {.buffer = chars};
    while (!lxl_utf8_stream_is_finished(&chars_stream)) {
        lxl_UnicodeCodepoint chars_next = lxl_utf8_stream_advance(&chars_stream);
        if (chars_next == lexer_next) return true;
    }
    lxl_lexer__rewind(lexer);
    return false;
}

bool lxl_lexer__match_string(struct lxl_lexer *lexer, struct lxl_string_view string) {
    if (lxl_lexer_is_finished(lexer)) return false;
    struct lxl_lexer old_state = *lexer;
    struct lxl_utf8_stream string_stream = {.buffer = string};
    while (!lxl_utf8_stream_is_finished(&string_stream)) {
        lxl_UnicodeCodepoint lexer_char = lxl_lexer__advance(lexer);
        lxl_UnicodeCodepoint string_char = lxl_utf8_stream_advance(&string_stream);
        if (string_stream.error) {
            LXL_TODO("error in match string");
        }
        if (lexer_char != string_char) goto fail;
    }
    return true;
fail:
    *lexer = old_state;
    return false;
}

bool lxl_lexer__match_whitespace_char(struct lxl_lexer *self) {
    return LXL_LEXER__CALL_QUERY(self, match_whitespace_char);
}

bool lxl_lexer__match_comment_line_opener(struct lxl_lexer *self) {
    return LXL_LEXER__CALL_QUERY(self, match_comment_line_opener);
}

bool lxl_lexer__match_comment_block_opener(struct lxl_lexer *self) {
    return LXL_LEXER__CALL_QUERY(self, match_comment_block_opener);
}

bool lxl_lexer__match_comment_block_closer(struct lxl_lexer *self) {
    return LXL_LEXER__CALL_QUERY(self, match_comment_block_closer);
}

bool lxl_lexer__match_comment_block_nest_opener(struct lxl_lexer *self) {
    return LXL_LEXER__CALL_QUERY(self, match_comment_block_nest_opener);
}

bool lxl_lexer__match_comment_block_nest_closer(struct lxl_lexer *self) {
    return LXL_LEXER__CALL_QUERY(self, match_comment_block_nest_closer);
}

bool lxl_lexer__match_word_init_char(struct lxl_lexer *self) {
    return LXL_LEXER__CALL_QUERY(self, match_word_init_char);
}

bool lxl_lexer__match_word_char(struct lxl_lexer *self) {
    return LXL_LEXER__CALL_QUERY(self, match_word_char);
}

bool lxl_lexer__match_int_prefix(struct lxl_lexer *self) {
    return LXL_LEXER__CALL_QUERY(self, match_int_prefix);
}

bool lxl_lexer__match_int_digit(struct lxl_lexer *self) {
    return LXL_LEXER__CALL_QUERY(self, match_int_digit);
}

bool lxl_lexer__match_int_suffix(struct lxl_lexer *self) {
    return LXL_LEXER__CALL_QUERY(self, match_int_suffix);
}

bool lxl_lexer__match_float_prefix(struct lxl_lexer *self) {
    return LXL_LEXER__CALL_QUERY(self, match_float_prefix);
}

bool lxl_lexer__match_float_digit(struct lxl_lexer *self) {
    return LXL_LEXER__CALL_QUERY(self, match_float_digit);
}

bool lxl_lexer__match_float_radix_sep(struct lxl_lexer *self) {
    return LXL_LEXER__CALL_QUERY(self, match_float_radix_sep);
}

bool lxl_lexer__match_float_exp_sep(struct lxl_lexer *self) {
    return LXL_LEXER__CALL_QUERY(self, match_float_exp_sep);
}

bool lxl_lexer__match_float_exp_sign(struct lxl_lexer *self) {
    return LXL_LEXER__CALL_QUERY(self, match_float_exp_sign);
}

bool lxl_lexer__match_float_suffix(struct lxl_lexer *self) {
    return LXL_LEXER__CALL_QUERY(self, match_float_suffix);
}

bool lxl_lexer__match_punct(struct lxl_lexer *self) {
    return LXL_LEXER__CALL_QUERY(self, match_punct);
}

bool lxl_lexer__match_string_opener(struct lxl_lexer *self) {
    return LXL_LEXER__CALL_QUERY(self, match_string_opener);
}

bool lxl_lexer__match_string_closer(struct lxl_lexer *self){
    return LXL_LEXER__CALL_QUERY(self, match_string_closer);
}

bool lxl_lexer__match_string_char(struct lxl_lexer *self) {
    return LXL_LEXER__CALL_QUERY(self, match_string_char);
}


bool lxl_lexer__match_whitespace_char_default(struct lxl_lexer *self) {
    return lxl_lexer__match_chars(self, LXL_SV_FROM_STRLIT(LXL_WHITESPACE_CHARS));
}

bool lxl_lexer__match_comment_line_opener_default(struct lxl_lexer *self) {
    (void)self;
    return false;
}

bool lxl_lexer__match_comment_block_opener_default(struct lxl_lexer *self) {
    (void)self;
    return false;
}

bool lxl_lexer__match_comment_block_closer_default(struct lxl_lexer *self) {
    (void)self;
    return false;
}

bool lxl_lexer__match_comment_block_nest_opener_default(struct lxl_lexer *self) {
    (void)self;
    return false;
}

bool lxl_lexer__match_comment_block_nest_closer_default(struct lxl_lexer *self) {
    (void)self;
    return false;
}

bool lxl_lexer__match_word_init_char_default(struct lxl_lexer *self) {
    return lxl_lexer__match_word_char(self);
}

bool lxl_lexer__match_word_char_default(struct lxl_lexer *self) {
    if (lxl_lexer__match_whitespace_char(self)) {
        lxl_lexer__rewind(self);
        return false;
    }
    lxl_lexer__advance(self);
    return true;
}

bool lxl_lexer__match_int_prefix_default(struct lxl_lexer *self) {
    return lxl_lexer__match_int_digit(self);
}

bool lxl_lexer__match_int_digit_default(struct lxl_lexer *self) {
    lxl_UnicodeCodepoint ch = lxl_lexer__advance(self);
    if ('0' <= ch && ch <= '9') return true;
    lxl_lexer__rewind(self);
    return false;
}

bool lxl_lexer__match_int_suffix_default(struct lxl_lexer *self) {
    (void)self;
    return false;
}

bool lxl_lexer__match_float_prefix_default(struct lxl_lexer *self) {
    return lxl_lexer__match_float_digit(self);
}

bool lxl_lexer__match_float_digit_default(struct lxl_lexer *self) {
    return lxl_lexer__match_int_digit_default(self);
}

bool lxl_lexer__match_float_radix_sep_default(struct lxl_lexer *self) {
    return lxl_lexer__match_string(self, LXL_SV_FROM_STRLIT("."));
}

bool lxl_lexer__match_float_exp_sep_default(struct lxl_lexer *self) {
    return lxl_lexer__match_chars(self, LXL_SV_FROM_STRLIT("eE"));
}

bool lxl_lexer__match_float_exp_sign_default(struct lxl_lexer *self) {
    return lxl_lexer__match_chars(self, LXL_SV_FROM_STRLIT("-+"));
}

bool lxl_lexer__match_float_suffix_default(struct lxl_lexer *self) {
    (void)self;
    return false;
}

bool lxl_lexer__match_punct_default(struct lxl_lexer *self) {
    (void)self;
    return false;
}

bool lxl_lexer__match_string_opener_default(struct lxl_lexer *self) {
    return lxl_lexer__match_chars(self, LXL_SV_FROM_STRLIT("\"'"));
}

bool lxl_lexer__match_string_closer_default(struct lxl_lexer *self) {
    return lxl_lexer__match_string(self, self->last_string_opener);
}

bool lxl_lexer__match_string_char_default(struct lxl_lexer *self) {
    const char *mark = lxl_lexer__peek(self);
    if (lxl_lexer__match_string_closer(self)) {
        const char *point = lxl_lexer__peek(self);
        ptrdiff_t bytes_read = point - mark;
        self->stream.cursor -= bytes_read;
        return false;
    }
    lxl_lexer__advance(self);
    return true;
}

bool lxl_lexer__match_whitespace_char_builtin_no_lf(struct lxl_lexer *self) {
    char ch = *lxl_lexer__peek(self);
    if (ch == '\n') {
        return false;
    }
    return lxl_lexer__match_whitespace_char(self);
}

bool lxl_lexer__match_int_digit_builtin_hex(struct lxl_lexer *self) {
    struct lxl_string_view hex_alpha = LXL_SV_FROM_STRLIT("ABCDEFabcdef");
    return lxl_lexer__match_int_digit_default(self) || lxl_lexer__match_chars(self, hex_alpha);
}


int lxl_lexer__get_word_kind(struct lxl_lexer *self) {
    return LXL_LEXER__GET_KIND(self, get_word_kind);
}

int lxl_lexer__get_int_kind(struct lxl_lexer *self) {
    return LXL_LEXER__GET_KIND(self, get_int_kind);
}

int lxl_lexer__get_float_kind(struct lxl_lexer *self) {
    return LXL_LEXER__GET_KIND(self, get_float_kind);
}

int lxl_lexer__get_punct_kind(struct lxl_lexer *self) {
    return LXL_LEXER__GET_KIND(self, get_punct_kind);
}

int lxl_lexer__get_string_kind(struct lxl_lexer *self) {
    return LXL_LEXER__GET_KIND(self, get_string_kind);
}


void lxl_lexer__on_linefeed_hook_builtin_emit_line_ending(struct lxl_lexer *self) {
    self->next_state = lxl_lstate_EmitLineEndingToken;
}

// END LEXER INTERNAL INTERFACE.


// TOKEN INTERFACE.

struct lxl_string_view lxl_token_value(struct lxl_token token) {
    return lxl_sv_from_startend(token.start, token.end);
}

struct lxl_string_view lxl_error_message(enum lxl_lex_error error) {
    switch (error) {
    case LXL_LERR_OK:               return LXL_SV_FROM_STRLIT("No error");
    case LXL_LERR_GENERIC:          return LXL_SV_FROM_STRLIT("Generic error");
    case LXL_LERR_EOF:              return LXL_SV_FROM_STRLIT("Unexpected end of input");
    case LXL_LERR_UNCLOSED_BLOCK_COMMENT: return LXL_SV_FROM_STRLIT("Unclosed block comment");
    case LXL_LERR_UNCLOSED_STRING:  return LXL_SV_FROM_STRLIT("Unclosed string or string-like literal");
    case LXL_LERR_INVALID_INTEGER:  return LXL_SV_FROM_STRLIT("Invalid integer literal");
    case LXL_LERR_INVALID_FLOAT:    return LXL_SV_FROM_STRLIT("Invlaid floating-point literal");
    case LXL_LERR_UNICODE:          return LXL_SV_FROM_STRLIT("Unicode error");
    case LXL_LERR_UNRECOGNISED_TOKEN: return LXL_SV_FROM_STRLIT("Unknown token");
    case LXL_LERR_INVALID_STRING_CHARACTER: return LXL_SV_FROM_STRLIT("Invalid character in string-like literal");
    }
    LXL_UNREACHABLE();
    return (struct lxl_string_view) {0};
}

// END TOKEN INTERFACE.


// STRING VIEW INTERFACE.

struct lxl_string_view lxl_sv_from_string(const char *string) {
    return lxl_sv_from_startlen(string, strlen(string));
}

struct lxl_string_view lxl_sv_from_startlen(const char *start, ptrdiff_t length) {
    LXL_ASSERT(length >= 0);
    return (struct lxl_string_view) {
        .start = start,
        .length = length,
    };
}

struct lxl_string_view lxl_sv_from_startend(const char *start, const char *end) {
    LXL_ASSERT(start <= end);
    return (struct lxl_string_view) {
        .start = start,
        .length = end - start,
    };
}

const char *lxl_sv_end(struct lxl_string_view sv) {
    return &sv.start[sv.length];
}

bool lxl_sv_is_empty(struct lxl_string_view sv) {
    return sv.length == 0;
}

ptrdiff_t lxl_sv_normalise_index(struct lxl_string_view sv, ptrdiff_t index) {
    if (index < 0) index += sv.length;
    if (index < 0) index = 0;
    if (index > sv.length) index = sv.length;
    return index;
}

struct lxl_string_view lxl_sv_slice(struct lxl_string_view sv, ptrdiff_t from, ptrdiff_t to_ex) {
    from = lxl_sv_normalise_index(sv, from);
    to_ex = lxl_sv_normalise_index(sv, to_ex);
    ptrdiff_t length = to_ex - from;
    if (length < 0) length = 0;
    return lxl_sv_from_startlen(&sv.start[from], length);
}

struct lxl_string_view lxl_sv_slice_end(struct lxl_string_view sv, ptrdiff_t from) {
    return lxl_sv_slice(sv, from, sv.length);
}

struct lxl_string_view lxl_sv_slice_start(struct lxl_string_view sv, ptrdiff_t to_ex) {
    return lxl_sv_slice(sv, 0, to_ex);
}

bool lxl_sv_eq(struct lxl_string_view a, struct lxl_string_view b) {
    if (a.length != b.length) return false;
    if (a.length == 0) return true;
    LXL_ASSERT(a.start != NULL && b.start != NULL);
    return memcmp(a.start, b.start, a.length) == 0;
}

bool lxl_sv_eq_strings_impl(struct lxl_string_view sv, ...) {
    va_list vargs;
    va_start(vargs, sv);
    bool success = lxl_sv_eq_strings_impl_vargs(sv, vargs);
    va_end(vargs);
    return success;
}

bool lxl_sv_eq_strings_impl_vargs(struct lxl_string_view sv, va_list vargs) {
    for (const char *str_arg; (str_arg = va_arg(vargs, const char *));) {
        if (lxl_sv_eq(sv, lxl_sv_from_string(str_arg))) {
            return true;
        }
    }
    return false;
}

bool lxl_sv_has_prefix(struct lxl_string_view sv, struct lxl_string_view prefix) {
    return lxl_sv_eq(lxl_sv_slice_start(sv, prefix.length), prefix);
}

bool lxl_sv_has_prefix_strings_impl(struct lxl_string_view sv, ...) {
    va_list vargs;
    va_start(vargs, sv);
    bool success = lxl_sv_has_prefix_strings_impl_vargs(sv, vargs);
    va_end(vargs);
    return success;
}

bool lxl_sv_has_prefix_strings_impl_vargs(struct lxl_string_view sv, va_list vargs) {
    const char *string = NULL;
    while ((string = va_arg(vargs, const char *))) {
        struct lxl_string_view prefix = lxl_sv_from_string(string);
        if (lxl_sv_has_prefix(sv, prefix)) return true;
    }
    return false;
}

bool lxl_sv_has_suffix(struct lxl_string_view sv, struct lxl_string_view suffix) {
    return lxl_sv_eq(lxl_sv_slice_end(sv, sv.length - suffix.length), suffix);
}

bool lxl_sv_has_suffix_strings_impl(struct lxl_string_view sv, ...) {
    va_list vargs;
    va_start(vargs, sv);
    bool success = lxl_sv_has_suffix_strings_impl_vargs(sv, vargs);
    va_end(vargs);
    return success;
}

bool lxl_sv_has_suffix_strings_impl_vargs(struct lxl_string_view sv, va_list vargs) {
    const char *string = NULL;
    while ((string = va_arg(vargs, const char *))) {
        struct lxl_string_view suffix = lxl_sv_from_string(string);
        if (lxl_sv_has_suffix(sv, suffix)) return true;
    }
    return false;
}

struct lxl_string_view lxl_sv_remove_predicate_left(struct lxl_string_view sv, int (*pred)(int ch)) {
    while (!lxl_sv_is_empty(sv) && pred(sv.start[0])) {
        sv = lxl_sv_slice_end(sv, 1);
    }
    return sv;
}

struct lxl_string_view lxl_sv_remove_predicate_right(struct lxl_string_view sv, int (*pred)(int ch)) {
    while (!lxl_sv_is_empty(sv) && pred(lxl_sv_end(sv)[-1])) {
        sv = lxl_sv_slice_start(sv, -1);
    }
    return sv;
}


// END STRING VIEW INTERFACE.


// UNICODE INTERFACE.

const char *lxl_utf8_stream_position(const struct lxl_utf8_stream *stream) {
    return &stream->buffer.start[stream->cursor];
}

struct lxl_string_view lxl_utf8_stream_tail(const struct lxl_utf8_stream *stream) {
    LXL_ASSERT(0 <= stream->cursor && stream->cursor <= stream->buffer.length);
    return lxl_sv_slice_end(stream->buffer, stream->cursor);
}

bool lxl_utf8_stream_is_finished(const struct lxl_utf8_stream *stream) {
    return stream->cursor >= stream->buffer.length;
}

int lxl_count_leading_ones(uint8_t byte) {
    int count = 0;
    for (; byte & 0x80; byte <<= 1) {
        count += 1;
    }
    return count;
}

int lxl_get_utf8_length(lxl_UnicodeCodepoint value) {
    if (value <= 0x7F) return 1;
    if (value <= 0x7FF) return 2;
    if (value <= 0xFFFF) return 3;
    return 4;
}

lxl_UnicodeCodepoint lxl_utf8_stream_advance(struct lxl_utf8_stream *stream) {
    stream->error = LXL_UNIERR_OK;
    if (lxl_utf8_stream_is_finished(stream)) {
        stream->error = LXL_UNIERR_UNEXPECTED_EOF;
        return 0;
    }
    unsigned first_byte = stream->buffer.start[stream->cursor++];
    int n_ones = lxl_count_leading_ones(first_byte);
    if (n_ones == 0) return first_byte;  // ASCII.
    if (n_ones == 1) {
        // Byte starts 10xxxxxx.
        stream->error = LXL_UNIERR_UNEXPECTED_CONT_BYTE;
        return 0;
    }
    if (n_ones > 4) {
        stream->error = LXL_UNIERR_INVALID_FIRST_BYTE;
        return 0;
    }
    int n_cont_bytes = n_ones - 1;
    unsigned mask = (1 << (8 - n_ones)) - 1;
    lxl_UnicodeCodepoint value = first_byte & mask;
    for (int i = 0; i < n_cont_bytes; ++i) {
        if (lxl_utf8_stream_is_finished(stream)) {
            stream->error = LXL_UNIERR_UNEXPECTED_EOF;
            return 0;
        }
        unsigned cont_byte = stream->buffer.start[stream->cursor++];
        n_ones = lxl_count_leading_ones(cont_byte);
        if (n_ones == 0 || (2 <= n_ones && n_ones <= 4)) {
            // `cont_byte` is a valid first byte.
            --stream->cursor;
            stream->error = LXL_UNIERR_MISSING_CONT_BYTE;
            return 0;
        }
        else if (n_ones != 1) {
            stream->error = LXL_UNIERR_INVALID_CONT_BYTE;
            continue;
        }
        value <<= 6;
        value |= cont_byte & 0x3F;
    }
    if (stream->error) {
        return 0;
    }
    if (value > 0x10FFFF || (0xD800 <= value && value <= 0xDFFF)) {
        // Outside of valid Unicode range.
        stream->error = LXL_UNIERR_OUT_OF_RANGE;
    }
    else if (lxl_get_utf8_length(value) < 1 + n_cont_bytes) {
        // Overlong encoding.
        stream->error = LXL_UNIERR_OVERLONG_ENCODING;
    }
    return value;
}

bool lxl_utf8_stream_rewind(struct lxl_utf8_stream *stream) {
    stream->error = LXL_UNIERR_OK;
    int n_cont_bytes = 0;
    for (; lxl_count_leading_ones(stream->buffer.start[stream->cursor]) == 1; --stream->cursor) {
        if (stream->cursor <= 0) {
            stream->error = LXL_UNIERR_UNEXPECTED_EOF;
            return false;
        }
        ++n_cont_bytes;
        if (n_cont_bytes > 3) {
            stream->error = LXL_UNIERR_UNEXPECTED_CONT_BYTE;
            return false;
        }
    }
    LXL_ASSERT(0 <= n_cont_bytes && n_cont_bytes <= 3);
    --stream->cursor;
    uint8_t first_byte = stream->buffer.start[stream->cursor];
    int n_ones_first = lxl_count_leading_ones(first_byte);
    if (n_ones_first == 0 && n_cont_bytes == 0) return true;
    if (n_ones_first + 1 == n_cont_bytes) return true;
    stream->error = LXL_UNIERR_INVALID_FIRST_BYTE;
    return false;
}

// END UNICODE INTERFACE.

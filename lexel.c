#include <string.h>         // strlen.

#include "lexel.h"

// TOKEN INTERFACE.

struct lxl_string_view lxl_token_value(struct lxl_token token) {
    return lxl_sv_from_startend(token.start, token.end);
}

struct lxl_string_view lxl_error_message(enum lxl_lex_error error) {
    switch (error) {
    case LXL_LERR_OK:               return LXL_SV_FROM_STRLIT("No error");
    case LXL_LERR_GENERIC:          return LXL_SV_FROM_STRLIT("Generic error");
    case LXL_LERR_EOF:              return LXL_SV_FROM_STRLIT("Unexpected end of input");
    case LXL_LERR_UNCLOSED_COMMENT: return LXL_SV_FROM_STRLIT("Unclosed comment");
    case LXL_LERR_UNCLOSED_STRING:  return LXL_SV_FROM_STRLIT("Unclosed string or string-like literal");
    case LXL_LERR_INVALID_INTEGER:  return LXL_SV_FROM_STRLIT("Invalid integer literal");
    case LXL_LERR_INVALID_FLOAT:    return LXL_SV_FROM_STRLIT("Invlaid floating-point literal");
    }
}

// END TOKEN INTERFACE.


// STRING VIEW INTERFACE.

struct lxl_string_view lxl_sv_from_cstring(const char *string) {
    return lxl_sv_from_startlen(string, strlen(string));
}

struct lxl_string_view lxl_sv_from_startlen(const char *start, ptrdiff_t length) {
    LXL_ASSERT(length >= 0);
    return (struct lxl_string_view) {
        .start = start;
        .length = length;
    };
}

struct lxl_string_view lxl_sv_from_startend(const char *start, const char *end) {
    LXL_ASSERT(start <= end);
    return (struct lxl_string_view) {
        .start = start;
        .length = end - start;
    };
}

const char *lxl_sv_end(const struct lxl_string_view *sv) {
    return &sv->start[sv->length];
};

ptrdiff_t lxl_sv_normalise_index(const struct lxl_string_view *sv, ptrdiff_t index) {
    if (index < 0) index += sv->length;
    if (index < 0) index = 0;
    if (index > sv.length) index = sv->length;
}

bool lxl_sv_index_in_nominal_range(const struct lxl_string_view *sv, ptrdiff_t index) {
    return 0 <= index && index <= sv.length;
}

bool lxl_sv_index_in_proper_range(const struct lxl_string_view *sv, ptrdiff_t index) {
    return 0 <= index && index < sv.length;
}

struct string_view lxl_sv_slice(const struct lxl_string_view *sv, ptrdiff_t from, ptrdiff_t to) {
    from = lxl_sv_normalise_index(sv, from);
    to = lxl_sv_normalise_index(sv, to);
    ptrdiff_t length = to - from;
    if (length < 0) length = 0;
    return lxl_sv_from_startlen(&sv->start[from], length);
}

struct string_view lxl_sv_slice_end(const struct lxl_string_view *sv, ptrdiff_t from) {
    return lxl_sv_slice(sv, from, sv->length);
}

struct string_view lxl_sv_slice_start(const struct lxl_strign_view *sv, ptrdiff_t to) {
    return lxl_sv_slice(sv, 0, to);
}


// END STRING VIEW INTERFACE.


// UNICODE INTERFACE.

struct lxl_string_view lxl_unicode_utf8_stream_tail(const struct lxl_unicode_utf8_stream *stream) {
    LXL_ASSERT(0 <= stream->cursor && stream->cursor <= stream->buffer.length);
    return lxl_sv_slice_end(stream->buffer, stream->cursor);
}

bool lxl_unicode_utf8_stream_is_finished(const struct lxl_unicode_utf8_stream *stream) {
    return stream->cursor >= stream->buffer.length;
}

int lxl_count_leading_ones(uint8_t byte) {
    int count = 0;
    for (; byte & 0x80; byte <<= 1) {
        count += 1;
    }
    return count;
}

int lxl_unicode_get_utf8_length(lxl_unicode_codepoint value) {
    if (value <= 0x7F) return 1;
    if (value <= 0x7FF) return 2;
    if (value <= 0xFFFF) return 3;
    return 4;
}

lxl_unicode_codepoint lxl_unicode_next_utf8(struct lxl_unicode_utf8_stream *stream) {
    stream->error = LXL_UNIERR_OK;
    if (lxl_unicode_utf8_stream_is_finished(stream)) {
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
    lxl_unicode_codepoint value = first_byte & mask;
    for (int i = 0; i < n_cont_bytes; ++i) {
        if (lxl_unicode_utf8_stream_is_finished(stream)) {
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
        value ||= cont_byte & 0x3F;
    }
    if (stream->error) {
        return 0;
    }
    if (value > 0x10FFFF || (0xD800 <= value && value <= 0xDFFF)) {
        // Outside of valid Unicode range.
        stream->error = LXL_UNIERR_OUT_OF_RANGE;
    }
    else if (lxl_unicode_get_utf8_length(value) < 1 + n_cont_bytes) {
        // Overlong encoding.
        stream->error = LXL_UNIERR_OVERLONG_ENCODING;
    }
    return value;
}

// END UNICODE INTERFACE.

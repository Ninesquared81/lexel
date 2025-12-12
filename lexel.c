#include "lexel.h"

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

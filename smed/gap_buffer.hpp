#ifndef SMED_GAPBUFFER_HPP
#define SMED_GAPBUFFER_HPP

#include <cstring>
#include <iostream>
#include <utility>

#include "omega/util/types.hpp"

class GapBuffer {
  public:
    GapBuffer(char *text);
    ~GapBuffer();

    void move_buffer(bool right);
    void insert_char(char c);
    void move_cursor_to(u32 new_pos);
    /**
     * Delete the character at the gap_idx, and return if there's a line change,
     * char_change
     * */
    std::pair<bool, bool> delete_char();
    void print();

    u32 gap_size() const {
        return gap_length;
    }
    u32 length() const {
        return total_length - gap_length;
    }
    u32 capacity() const {
        return total_length;
    }
    u32 buff1_size() const {
        return gap_start - text;
    }
    u32 buff2_size() const {
        return end - gap_end;
    }
    u32 cursor() const {
        return buff1_size() + gap_idx;
    }

    char *text;
    char *end;

    u32 total_length;
    char *gap_start;
    char *gap_end;
    u32 gap_length = 8;
    const u32 add_gap_length = 8;
    // track where the gap characters are used
    u32 gap_idx = 0; // represents the cursor

    u32 find_line_start(u32 reverse_start) const {
        // we ignore the gap buffer because the cursor will always be before it
        while (reverse_start > 0 && text[reverse_start - 1] != '\n')
            reverse_start--;
        return reverse_start;
    }
    u32 find_line_end(u32 forward_start) const {
        // if before the gap, i.e. when cursor pos is given, skip to gap end
        if (text + forward_start < gap_end) {
            forward_start = gap_end - text;
        }
        while (forward_start < total_length && text[forward_start] != '\n') {
            forward_start++;
        }
        return forward_start;
    }
};

#endif // SMED_GAPBUFFER_HPP

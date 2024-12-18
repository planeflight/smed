#ifndef SMED_GAPBUFFER_HPP
#define SMED_GAPBUFFER_HPP

#include <cstring>
#include <utility>

#include "omega/util/types.hpp"

class GapBuffer {
  public:
    GapBuffer(char *text);
    ~GapBuffer();

    void move_buffer(bool right);
    void insert_char(char c);
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
};

#endif // SMED_GAPBUFFER_HPP

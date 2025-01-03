#ifndef SMED_GAPBUFFER_HPP
#define SMED_GAPBUFFER_HPP

#include <cstring>
#include <iostream>
#include <omega/util/types.hpp>
#include <utility>

class GapBuffer {
  public:
    GapBuffer(const char *text);
    ~GapBuffer();

    void move_buffer(bool right);
    void insert_char(char c);
    void move_cursor_to(u32 new_pos);
    /**
     * Delete the character at the gap_idx, and return if there's a line change,
     * char_change
     * */
    std::pair<bool, bool> backspace_char();
    void delete_char();
    void print();

    u32 gap_size() const {
        return gap_length;
    }
    u32 length() const {
        return buff1_size() + buff2_size() + gap_idx;
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
    const char *head() const {
        return text;
    }

    const char *buff2() const {
        return gap_end;
    }
    const char *tail() const {
        return end;
    }
    bool is_keyword(char *keyword, u32 start, u32 len) const {
        if (start + len < total_length) {
            i32 res = strncmp(keyword, text + start, len);
            return res == 0;
        }
        return false;
    }

    char &get(u32 i) const {
        if (i < buff1_size() + gap_idx) {
            return text[i];
        }
        return text[i + (gap_length - gap_idx)];
    }

    std::string substr(u32 i, u32 l) {
        std::string s;
        for (u32 x = i; x < i + l; ++x) {
            s += get(x);
        }
        return s;
    }

    bool compare(size_t start_index, const char *s, size_t n) const {
        if (start_index + n >= length()) {
            return false;
        }
        for (size_t i = 0; i < n; ++i) {
            if (get(start_index + i) != s[i]) {
                return false;
            }
        }
        return true;
    }

    i32 search(u32 start, const std::string &string) const {
        // check if it's not out of bounds
        if (start + string.length() >= length()) {
            return -1;
        }
        for (u32 i = start; i < length(); ++i) {
            if (compare(i, string.c_str(), string.length())) {
                return i;
            }
        }
        return -1;
    }

    u32 get_index_from_pointer(const char *ptr) const {
        u32 i = ptr - text;
        if (i > cursor()) {
            i -= gap_length - gap_idx;
        }
        return i;
    }

    u32 find_line_start(u32 reverse_start) const {
        while (reverse_start > 0 && get(reverse_start - 1) != '\n')
            reverse_start--;
        return reverse_start;
    }
    u32 find_line_end(u32 forward_start) const {
        while (forward_start < length() && get(forward_start) != '\n') {
            forward_start++;
        }
        return forward_start;
    }
    u32 find_next_word(u32 i) const {
        // check if start is a word sequence or a special character sequence
        if (isalnum(get(i))) {
            while (i < length() && isalnum(get(i))) {
                i++;
            }
            return i;
        }
        while (i < length() && !isalnum(get(i))) {
            i++;
        }
        return i;
    }

    u32 find_prev_word(i32 i) const {
        if (isalnum(get(i))) {
            while (i > 0 && isalnum(get(i))) {
                i--;
            }
            return i + 1;
        }
        while (i > 0 && !isalnum(get(i))) {
            i--;
        }
        return i + 1;
    }

  private:
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

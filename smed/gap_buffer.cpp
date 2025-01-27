#include "gap_buffer.hpp"

#include <iostream>

GapBuffer::GapBuffer(const char *text) {
    open(text);
}

GapBuffer::~GapBuffer() {
    delete[] text;
}

void GapBuffer::open(const char *text) {
    if (this->text != nullptr) {
        delete[] this->text;
    }

    u32 text_length = strlen(text);
    this->text = new char[text_length + gap_length];
    total_length = text_length + gap_length;

    // the cursor is automatically at the end
    this->end = this->text + total_length;
    strncpy(this->text + gap_length, text, text_length);
    gap_start = this->text;
    gap_end = gap_start + gap_length;
}

void GapBuffer::move_buffer(bool right) {
    if (right) {
        gap_start++;
        *(gap_start + gap_idx - 1) = *gap_end;
        gap_end++;
    } else {
        gap_start--;
        gap_end--;
        *gap_end = gap_start[gap_idx];
    }
}

void GapBuffer::insert_char(char c) {
    if (gap_idx < gap_length) {
        gap_start[gap_idx++] = c;
    } else {
        // TODO: use amortized constant time with 2^N
        gap_length = add_gap_length;
        char *new_text = new char[total_length + gap_length];
        // copy everything from the buff1, gap buffer into the new buff1
        u32 buff1_size = gap_end - text;
        strncpy(new_text, text, buff1_size);
        // copy everything from buff2, after the new gap buffer
        strncpy(new_text + buff1_size + gap_length, gap_end, end - gap_end);
        delete[] text;
        text = new_text;
        // update counters
        total_length += gap_length;
        end = text + total_length;
        // update gap pointers
        gap_start = text + buff1_size;
        gap_end = gap_start + gap_length;
        gap_idx = 0;
        gap_start[gap_idx++] = c;
    }
}

void GapBuffer::move_cursor_to(u32 new_pos) {
    i32 steps = (i32)new_pos - cursor();
    if (steps > 0) {
        for (int i = 0; i < steps; ++i) {
            move_buffer(true);
        }
    } else {
        steps *= -1;
        for (int i = 0; i < steps; ++i) {
            move_buffer(false);
        }
    }
}

std::pair<bool, bool> GapBuffer::backspace_char() {
    bool line_change = false;
    bool char_change = false;
    // if there's text in the gap buffer, simply decrement the gap_idx
    if (gap_idx > 0) {
        line_change = gap_start[gap_idx] == '\n';
        gap_idx--;
        char_change = true;
    } // otherwise decrease the gap_start pointer and increase
      // if the beginning of the string isn't yet reached
    else if (gap_start > text) {
        line_change = *(gap_start - 1) == '\n';
        gap_start--;
        gap_length++;
        char_change = true;
    }
    return {line_change, char_change};
}

void GapBuffer::delete_char() {
    // swallow the last character if possible
    if (gap_end < end) {
        gap_end++;
        gap_length++;
    }
}

void GapBuffer::print() {
    char *i = text;
    for (; i < gap_start; ++i) {
        std::cout << *i;
    }
    std::cout << "[";
    for (; i < gap_start + gap_idx; ++i) {
        std::cout << *i;
    }
    for (; i < gap_end; ++i) {
        std::cout << "_";
    }
    std::cout << "]";
    for (; i < end; ++i) {
        std::cout << *i;
    }
    std::cout << std::endl;
}

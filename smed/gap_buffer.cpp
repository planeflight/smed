#include "gap_buffer.hpp"

#include <iostream>

GapBuffer::GapBuffer(char *text) {
    u32 text_length = strlen(text);
    this->text = new char[text_length + gap_length];
    total_length = text_length + gap_length;

    // the cursor is automatically at the end
    this->end = this->text + total_length;
    strncpy(this->text, text, text_length);
    gap_start = this->text + text_length;
    gap_end = gap_start + gap_length;
}

GapBuffer::~GapBuffer() {
    delete[] text;
}

void GapBuffer::move_buffer(bool right) {
    print();
    if (right) {
        gap_start++;
        *(gap_start + gap_idx - 1) = *gap_end;
        gap_end++;
    } else {
        gap_start--;
        gap_end--;
        *gap_end = gap_start[gap_idx];
    }
    print();
}

void GapBuffer::insert_char(char c) {
    if (gap_idx < gap_length) {
        gap_start[gap_idx++] = c;
    } else {
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

std::pair<bool, bool> GapBuffer::delete_char() {
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

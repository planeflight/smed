#include "editor.hpp"

#include <cstring>
#include <fstream>

#include "omega/core/error.hpp"
#include "omega/events/event.hpp"
#include "omega/gfx/shader.hpp"
#include "omega/gfx/sprite_batch.hpp"
#include "omega/ui/font.hpp"
#include "omega/util/color.hpp"
#include "smed/font_renderer.hpp"
#include "smed/gap_buffer.hpp"
#include "smed/lexer.hpp"

Editor::Editor(omega::gfx::Shader *shader, Font *font, const std::string &text)
    : text(text.c_str()), lexer(&this->text, font), font_renderer(shader) {
    retokenize();
    cursor_idx = text.length() - 1;
}

void Editor::render(Font *font,
                    const omega::math::mat4 &view_proj,
                    omega::gfx::ShapeRenderer &shape) {
    int i = 0;
    f32 height = 25.0f;
    f32 scale_factor = height / font->get_font_size();
    font_renderer.begin(view_proj);
    auto pos = font_renderer.render(font,
                                    text,
                                    tokens,
                                    {20, 800},
                                    {20, 800},
                                    height,
                                    omega::util::color::white);
    cursor_pos.bottom = {pos.x, pos.y};
    cursor_pos.top = {pos.x, pos.y + height * 0.8};

    font_renderer.end();

    shape.begin();
    shape.set_view_projection_matrix(view_proj);
    shape.color = omega::util::color::white;
    shape.line(cursor_pos.bottom, cursor_pos.top);

    // draw the shift part
    if (selection_start != -1) {
        shape.color.a = 0.5f;
        f32 width = cursor_pos.bottom.x - selection_start_pos.bottom.x;
        shape.rect({
            selection_start_pos.bottom.x,
            selection_start_pos.bottom.y,
            width,
            height * 0.8f,
        });
    }
    shape.end();
}

void Editor::save(const std::string &file) {
    std::ofstream of;
    of.open(file);
    if (of.fail()) {
        omega::util::err("Failed to open file: '{}'", file);
    }

    of.write(text.text, text.buff1_size() + text.gap_idx);
    of.write(text.gap_end, text.buff2_size());
    of.close();
}

void Editor::handle_text(char c) {
    text.insert_char(c);
    retokenize();
}

void Editor::handle_input(omega::events::InputManager &input) {
    // for any keys that are not up/down, reset the vertical pos
    auto &keys = input.key_manager;
    if (keys[omega::events::Key::k_backspace]) {
        vertical_pos = -1;
        // TODO: add a small input delay
        auto result = text.backspace_char();
        bool delete_line = std::get<0>(result);
        bool delete_char = std::get<1>(result);
        if (delete_line || delete_char) {
            cursor_idx--;
        }
        retokenize();
    }
    if (keys[omega::events::Key::k_delete]) {
        vertical_pos = -1;
        text.delete_char();
        retokenize();
    }
    if (keys[omega::events::Key::k_enter]) {
        vertical_pos = -1;
        text.insert_char('\n');
        retokenize();
    }
    if (keys.key_just_pressed(omega::events::Key::k_tab)) {
        vertical_pos = -1;
        for (int i = 0; i < 4; ++i) {
            handle_text(' ');
        }
        retokenize();
    }
    // arrow keys
    if (keys.key_just_pressed(omega::events::Key::k_left)) {
        vertical_pos = -1;
        if (text.cursor() > 0) {
            text.move_buffer(false);
            if (selection_start != -1) selection_size--;
        }
        retokenize();
    }
    if (keys.key_just_pressed(omega::events::Key::k_right)) {
        vertical_pos = -1;
        if (text.gap_end < text.end) {
            text.move_buffer(true);
            if (selection_start != -1) selection_size++;
        }
        retokenize();
    }
    if (keys.key_just_pressed(omega::events::Key::k_up)) {
        // calcute the current line start and the previous line start
        u32 line_start = text.find_line_start(text.cursor());
        u32 prev_line_start = line_start; // both are 0 when line_start = 0
        if (line_start > 0) {
            prev_line_start = text.find_line_start(line_start - 1);
        }
        // calculate the current column and previous column
        u32 current_col = text.cursor() - line_start;
        // ensures that if the previous vertical_pos > current_col, the cursor
        // should move there
        u32 prev_col_idx =
            prev_line_start + omega::math::max((i32)current_col, vertical_pos);

        // if this doesn't cause the cursor to overflow into this current line
        if (prev_col_idx < line_start) {
            text.move_cursor_to(prev_col_idx);
        } // when the previous line is shorter
        else if (line_start > 0) {
            text.move_cursor_to(line_start - 1);
        } else if (line_start == 0) {
            text.move_cursor_to(0);
        }

        // track the vertical position, if this is the first up/down keystroke
        if (vertical_pos == -1) {
            vertical_pos = current_col;
        }
        retokenize();
    }
    if (keys.key_just_pressed(omega::events::Key::k_down)) {
        u32 idx = text.find_line_end(text.cursor());
        if (idx == text.capacity()) {
            text.move_cursor_to(idx);
            return;
        }
        u32 next_line_end = text.find_line_end(idx + 1);
        // calculate current column
        u32 line_start = text.find_line_start(text.cursor());
        u32 current_col = text.cursor() - line_start;

        u32 next_col_idx =
            idx + 1 + omega::math::max((i32)current_col, vertical_pos);
        if (next_col_idx <= next_line_end) {
            text.move_cursor_to(next_col_idx);
        } else {
            text.move_cursor_to(next_line_end);
        }
        // track the vertical position, if this is the first up/down keystroke
        if (vertical_pos == -1) {
            vertical_pos = current_col;
        }
        retokenize();
    }
    if (keys[omega::events::Key::k_l_ctrl] && keys[omega::events::Key::k_s]) {
        save("./output.txt");
    }
    // just pressed shift
    if (keys.key_just_pressed(omega::events::Key::k_l_shift)) {
        selection_start = cursor_idx;
        selection_start_pos = cursor_pos;
    }
    if (!keys[omega::events::Key::k_l_shift] && selection_size == 0) {
        selection_start = -1;
    }
}

void Editor::retokenize() {
    tokens.clear();

    lexer.retokenize();
    Token token = lexer.next();
    while (token.type != TokenType::END) {
        tokens.push_back(token);
        u32 idx = text.get_index_from_pointer(token.text);
        token = lexer.next();
    }
}

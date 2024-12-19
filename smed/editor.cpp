#include "editor.hpp"

#include <cstring>

#include "omega/events/event.hpp"
#include "omega/gfx/shader.hpp"
#include "omega/gfx/sprite_batch.hpp"
#include "omega/ui/font.hpp"
#include "omega/util/color.hpp"
#include "smed/font_renderer.hpp"
#include "smed/gap_buffer.hpp"

Editor::Editor(omega::gfx::Shader *shader)
    : text("#include <stdio.h>"), font_renderer(shader) {
    cursor_idx = text.length() - 1;
}

void Editor::render(Font *font,
                    const omega::math::mat4 &view_proj,
                    omega::gfx::ShapeRenderer &shape) {
    int i = 0;
    f32 height = 25.0f;
    f32 scale_factor = height / font->get_font_size();
    font_renderer.begin(view_proj);
    auto pos = font_renderer.render(
        font, text, {20, 800}, {20, 800}, height, omega::util::color::white);

    font_renderer.end();

    shape.begin();
    shape.set_view_projection_matrix(view_proj);
    shape.color = omega::util::color::white;
    shape.line({pos.x, pos.y}, {pos.x, pos.y + height * 0.8});
    shape.end();
}

void Editor::save(const std::string &file) {}

void Editor::handle_text(char c) {
    text.insert_char(c);
}

void Editor::handle_input(omega::events::InputManager &input) {
    // for any keys that are not up/down, reset the vertical pos
    auto &keys = input.key_manager;
    if (keys[omega::events::Key::k_backspace]) {
        vertical_pos = -1;
        // TODO: add a small input delay
        auto result = text.delete_char();
        bool delete_line = std::get<0>(result);
        bool delete_char = std::get<1>(result);
        if (delete_line || delete_char) {
            cursor_idx--;
        }
    }
    if (keys[omega::events::Key::k_enter]) {
        vertical_pos = -1;
        text.insert_char('\n');
    }
    if (keys.key_just_pressed(omega::events::Key::k_tab)) {
        vertical_pos = -1;
        for (int i = 0; i < 4; ++i) {
            handle_text(' ');
        }
    }
    if (keys.key_just_pressed(omega::events::Key::k_left)) {
        vertical_pos = -1;
        if (text.cursor() > 0) {
            text.move_buffer(false);
        }
    }
    if (keys.key_just_pressed(omega::events::Key::k_right)) {
        vertical_pos = -1;
        if (text.gap_end < text.end) {
            text.move_buffer(true);
        }
    }
    if (keys.key_just_pressed(omega::events::Key::k_up)) {
        // TODO: Remember initial column when starting the down/up sequence
        // so that we can go like
        // text_
        // _
        // initial text_ for the cursor position

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
    }
}

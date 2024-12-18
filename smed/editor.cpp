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
    cursor = {text.length() - 1, 0};
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

void Editor::shape_render(omega::gfx::ShapeRenderer &shape) {
    shape.color = omega::util::color::white;
    shape.line({20 + (cursor.x + 1) * 30, 800 - (cursor.y - 1) * 30},
               {20 + (cursor.x + 1) * 30, 800 - cursor.y * 30});
}

void Editor::save(const std::string &file) {}

void Editor::handle_text(char c) {
    text.insert_char(c);
    cursor.x++;
}

void Editor::handle_input(omega::events::InputManager &input) {
    auto &keys = input.key_manager;
    if (keys[omega::events::Key::k_backspace]) {
        // TODO: add a small input delay
        auto result = text.delete_char();
        bool delete_line = std::get<0>(result);
        bool delete_char = std::get<1>(result);
        if (delete_line) {
            cursor.y--;
            cursor.x = -1; // TODO: CALCULATE NEW CURSOR.X POS
            cursor_idx--;
        } else if (delete_char) {
            cursor.x--;
            cursor_idx--;
        }
    }
    if (keys[omega::events::Key::k_enter]) {
        text.insert_char('\n');
    }
    if (keys.key_just_pressed(omega::events::Key::k_tab)) {
        for (int i = 0; i < 4; ++i) {
            handle_text(' ');
        }
    }
    if (keys.key_just_pressed(omega::events::Key::k_left)) {
        omega::util::debug("left");
        if (text.cursor() > 0) {
            text.move_buffer(false);
        }
    }
    if (keys.key_just_pressed(omega::events::Key::k_right)) {
        omega::util::debug("right");
        if (text.gap_end < text.end) {
            text.move_buffer(true);
        }
    }
}

#include "editor.hpp"

#include <cstring>

#include "omega/events/event.hpp"
#include "omega/gfx/sprite_batch.hpp"
#include "omega/ui/font.hpp"
#include "smed/gap_buffer.hpp"

Editor::Editor() : text("#include <stdio.h>") {
    cursor = {text.length() - 1, 0};
    cursor_idx = text.length() - 1;
}

void Editor::render(omega::ui::Font *font, omega::gfx::SpriteBatch &batch) {
    int i = 0;
    font->render(batch,
                 text.text,
                 text.buff1_size() + text.gap_idx,
                 20,
                 800 - i * 30,
                 30,
                 omega::util::color::white);
    font->render(batch,
                 text.text + text.buff1_size() + text.gap_size(),
                 text.buff2_size(),
                 20 + (text.buff1_size() + text.gap_idx) * 30,
                 800 - i * 30,
                 30,
                 omega::util::color::white);
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
        } else if (delete_char) {
            cursor.x--;
        }
    }
    if (keys[omega::events::Key::k_enter]) {
        text.insert_char('\n');
        cursor.y++;
        cursor.x = -1;
    }
    if (keys.key_just_pressed(omega::events::Key::k_tab)) {
        for (int i = 0; i < 4; ++i) {
            handle_text(' ');
        }
    }
    if (keys.key_just_pressed(omega::events::Key::k_left)) {
        if (cursor.x >= 0) {
            cursor.x--;
            text.move_buffer(-1);
        }
    }
    if (keys.key_just_pressed(omega::events::Key::k_right)) {
        if (text.gap_end < text.end) {
            cursor.x++;
            text.move_buffer(1);
        }
    }
}

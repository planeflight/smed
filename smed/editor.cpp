#include "editor.hpp"

#include "omega/events/event.hpp"
#include "omega/gfx/sprite_batch.hpp"
#include "omega/ui/font.hpp"

Editor::Editor() {
    text.lines.push_back("#include <stdio.h>");
    cursor = {text.lines.front().size() - 1, 0};
    current_line = text.lines.begin();
}

void Editor::render(omega::ui::Font *font, omega::gfx::SpriteBatch &batch) {
    int i = 0;
    for (auto line = text.lines.begin(); line != text.lines.end();
         ++line, ++i) {
        font->render(
            batch, *line, 20, 800 - i * 30, 30, omega::util::color::white);
    }
    font->render(batch,
                 std::to_string(cursor.x) + "," + std::to_string(cursor.y),
                 1500,
                 800,
                 30);
}

void Editor::shape_render(omega::gfx::ShapeRenderer &shape) {
    shape.color = omega::util::color::white;
    shape.line({20 + (cursor.x + 1) * 30, 800 - (cursor.y - 1) * 30},
               {20 + (cursor.x + 1) * 30, 800 - cursor.y * 30});
}

void Editor::save(const std::string &file) {}

void Editor::handle_text(char c) {
    current_line->push_back(c);
    cursor.x++;
}

void Editor::handle_input(omega::events::InputManager &input) {
    auto &keys = input.key_manager;
    if (keys[omega::events::Key::k_backspace]) {
        // TODO: add a small input delay
        if (current_line->size() > 0) {
            current_line->pop_back();
            cursor.x--;
        } else if (cursor.y > 0) {
            current_line--;
            cursor.y--;
            cursor.x = current_line->size() - 1;
        }
    }
    if (keys[omega::events::Key::k_enter]) {
        text.lines.push_back("");
        cursor.y++;
        current_line++;
        cursor.x = -1;
    }
    if (keys.key_just_pressed(omega::events::Key::k_tab)) {
        for (int i = 0; i < 4; ++i) {
            handle_text(' ');
        }
    }
    if (keys.key_just_pressed(omega::events::Key::k_left)) {
        cursor.x--;
        if (cursor.x < -1) {
            current_line--;
            cursor.x = current_line->size() - 1;
            cursor.y = 0;
        }
    }
}

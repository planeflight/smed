#include "editor.hpp"

#include <cstring>
#include <fstream>

#include "omega/core/error.hpp"
#include "omega/events/event.hpp"
#include "omega/gfx/shader.hpp"
#include "omega/gfx/sprite_batch.hpp"
#include "omega/ui/font.hpp"
#include "omega/util/color.hpp"
#include "omega/util/log.hpp"
#include "omega/util/time.hpp"
#include "smed/font_renderer.hpp"
#include "smed/gap_buffer.hpp"
#include "smed/key_lag.hpp"
#include "smed/lexer.hpp"

Editor::Editor(omega::gfx::Shader *shader, Font *font, const std::string &text)
    : text(text.c_str()), lexer(&this->text, font), font_renderer(shader) {
    retokenize();
    using namespace omega::events;

    register_key(Key::k_backspace, [&](InputManager &input) {
        vertical_pos = -1;
        if (selection_start > -1) {
            if (selection_start < this->text.cursor()) {
                while (this->text.cursor() != selection_start) {
                    this->text.backspace_char();
                }
                selection_start = -1;
            } else if (selection_start > this->text.cursor()) {
                while (this->text.cursor() != selection_start) {
                    this->text.delete_char();
                    selection_start--;
                }
                selection_start = -1;
            }
        } else {
            this->text.backspace_char();
        }
        retokenize();
    });

    // for any keys that are not up/down, reset the vertical pos
    register_key(Key::k_delete, [&](InputManager &input) {
        vertical_pos = -1;
        if (selection_start > -1) {
            if (selection_start < this->text.cursor()) {
                while (this->text.cursor() != selection_start) {
                    this->text.backspace_char();
                }
                selection_start = -1;
            } else if (selection_start > this->text.cursor()) {
                while (this->text.cursor() != selection_start) {
                    this->text.delete_char();
                    selection_start--;
                }
                selection_start = -1;
            }
        } else {
            this->text.delete_char();
        }
        retokenize();
    });
    register_key(Key::k_enter, [&](InputManager &input) {
        vertical_pos = -1;
        this->text.insert_char('\n');
        retokenize();
    });
    // INFO: need to retokenize because the buffer moves, so pointers need to
    // change
    register_key(Key::k_left, [&](InputManager &input) {
        vertical_pos = -1;
        if (this->text.cursor() > 0) {
            this->text.move_buffer(false);
        }
        // stop selecting
        if (!input.key_manager.key_pressed(Key::k_l_shift)) {
            selection_start = -1;
        }
        retokenize();
    });
    register_key(Key::k_right, [&](InputManager &input) {
        vertical_pos = -1;
        if (this->text.gap_end < this->text.end) {
            this->text.move_buffer(true);
        }
        // stop selecting
        if (!input.key_manager.key_pressed(Key::k_l_shift)) {
            selection_start = -1;
        }
        retokenize();
    });
    register_key(Key::k_up, [&](InputManager &input) {
        // calcute the current line start and the previous line start
        u32 line_start = this->text.find_line_start(this->text.cursor());
        u32 prev_line_start = line_start; // both are 0 when line_start = 0
        if (line_start > 0) {
            prev_line_start = this->text.find_line_start(line_start - 1);
        }
        // calculate the current column and previous column
        u32 current_col = this->text.cursor() - line_start;
        // ensures that if the previous vertical_pos > current_col, the cursor
        // should move there
        u32 prev_col_idx =
            prev_line_start + omega::math::max((i32)current_col, vertical_pos);

        // if this doesn't cause the cursor to overflow into this current line
        if (prev_col_idx < line_start) {
            this->text.move_cursor_to(prev_col_idx);
        } // when the previous line is shorter
        else if (line_start > 0) {
            this->text.move_cursor_to(line_start - 1);
        } else if (line_start == 0) {
            this->text.move_cursor_to(0);
        }

        // track the vertical position, if this is the first up/down keystroke
        if (vertical_pos == -1) {
            vertical_pos = current_col;
        }
        // stop selecting
        if (!input.key_manager.key_pressed(Key::k_l_shift)) {
            selection_start = -1;
        }
        retokenize();
    });
    register_key(Key::k_down, [&](InputManager &input) {
        u32 idx = this->text.find_line_end(this->text.cursor());
        if (idx == text.capacity()) {
            this->text.move_cursor_to(idx);
            return;
        }
        u32 next_line_end = this->text.find_line_end(idx + 1);
        // calculate current column
        u32 line_start = this->text.find_line_start(this->text.cursor());
        u32 current_col = this->text.cursor() - line_start;

        u32 next_col_idx =
            idx + 1 + omega::math::max((i32)current_col, vertical_pos);
        if (next_col_idx <= next_line_end) {
            this->text.move_cursor_to(next_col_idx);
        } else {
            this->text.move_cursor_to(next_line_end);
        }
        // track the vertical position, if this is the first up/down keystroke
        if (vertical_pos == -1) {
            vertical_pos = current_col;
        }
        // stop selecting
        if (!input.key_manager.key_pressed(Key::k_l_shift)) {
            selection_start = -1;
        }

        retokenize();
    });
}

void Editor::render(Font *font,
                    omega::scene::OrthographicCamera &camera,
                    omega::gfx::ShapeRenderer &shape) {
    int i = 0;
    f32 height = font_render_height;
    f32 scale_factor = height / font->get_font_size();

    static omega::math::vec2 pos;

    // some camera panning action! Recalculate the vp from the last frame pos
    omega::math::vec3 target_cam{0.0f};
    target_cam.x = pos.x - camera.get_width() * 0.25f;
    target_cam.y = pos.y - camera.get_height() * 0.5f;
    camera.position += (target_cam - camera.position) * 0.025f;
    camera.recalculate_view_matrix();

    font_renderer.begin();
    pos = font_renderer.render(font,
                               camera.get_view_projection_matrix(),
                               text,
                               tokens,
                               {20, 800},
                               {20, 800},
                               height,
                               omega::util::color::white);
    // calculate cursor pos
    omega::math::vec2 cursor_pos_bottom = {pos.x, pos.y};
    omega::math::vec2 cursor_pos_top = {pos.x, pos.y + height * 0.8};

    // render the text batch finally
    font_renderer.end(camera.get_view_projection_matrix());

    // render cursor
    shape.begin();
    shape.set_view_projection_matrix(camera.get_view_projection_matrix());
    shape.color = omega::util::color::white;
    shape.line(cursor_pos_bottom, cursor_pos_top);

    // draw the selected text
    shape.color.a = 0.5f;
    font_renderer.render_selected(
        shape, font, text, selection_start, {20, 800}, height);
    shape.end();
}

void Editor::save(const std::string &file) {
    std::ofstream of;
    of.open(file);
    if (of.fail()) {
        OMEGA_ERROR("Failed to open file: '{}'", file);
    }

    of.write(text.text, text.buff1_size() + text.gap_idx);
    of.write(text.gap_end, text.buff2_size());
    of.close();
}

void Editor::handle_text(omega::events::InputManager &input, char c) {
    auto &keys = input.key_manager;
    // lock the text when ctrl is pressed
    if (!keys[omega::events::Key::k_l_ctrl]) {
        text.insert_char(c);
        retokenize();
    }
}

void Editor::handle_input(omega::events::InputManager &input) {
    using namespace omega::events;
    auto &keys = input.key_manager;

    if (keys[Key::k_l_ctrl] && keys[Key::k_s]) {
        save("./output.txt");
    }

    if (keys[Key::k_l_shift]) {
        if (keys[Key::k_left] || keys[Key::k_right] || keys[Key::k_up] ||
            keys[Key::k_down]) {
            if (selection_start == -1) {
                selection_start = text.cursor();
            }
        }
    }
    // update the keys that require a lag
    // INFO: update_keys AFTER querying selection state to track the old cursor
    // before selecting even started
    update_keys(input);

    // search functionality
    if (keys[Key::k_l_ctrl] && keys[Key::k_f]) {
        u32 s = text.search(text.cursor(), "void");
        OMEGA_DEBUG("Found {} at {}", "void", s);
    }
    // zooming
    if (keys[Key::k_l_ctrl] && keys[Key::k_minus]) {
        font_render_height -= 5;
        if (font_render_height < 10.0f) {
            font_render_height = 10.0f;
        }
    }
    if (keys[Key::k_l_ctrl] && keys[Key::k_plus]) {
        font_render_height += 5;
        if (font_render_height > 80.0f) {
            font_render_height = 80.0f;
        }
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

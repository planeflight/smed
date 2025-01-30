#include "editor.hpp"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <omega/core/error.hpp>
#include <omega/events/event.hpp>
#include <omega/gfx/shader.hpp>
#include <omega/gfx/sprite_batch.hpp>
#include <omega/ui/font.hpp>
#include <omega/util/color.hpp>
#include <omega/util/log.hpp>
#include <omega/util/time.hpp>

#include "smed/buffer_renderer.hpp"
#include "smed/gap_buffer.hpp"
#include "smed/key_lag.hpp"
#include "smed/lexer.hpp"

bool ctrl_char(omega::events::KeyManager &keys, omega::events::Key k) {
    using namespace omega::events;
    return (keys.key_just_pressed(Key::k_l_ctrl) && keys[k]) ||
           (keys.key_just_pressed(k) && keys[Key::k_l_ctrl]);
}

Editor::Editor(omega::gfx::Shader *shader,
               omega::gfx::Shader *shader_search,
               Font *font,
               std::string path)
    : text(""),
      lexer(&this->text, font),
      buffer_renderer(shader),
      font_renderer(shader_search),
      file_explorer(".") {
    using namespace omega::events;

    // add a ./ if there isn't already one
    if (path != "." && !path.starts_with("./")) {
        path.insert(0, "./");
    }

    // determine the root and set the file explorer root
    std::string root;
    root = std::filesystem::path(path).parent_path() / "";
    if (std::filesystem::is_directory(path)) {
        root = path;
    }
    file_explorer.set_root(root);
    std::string out; // unused
    file_explorer.open(path, out);

    // open the file
    if (path != root) {
        // refill the gap buffer and retokenize
        std::ifstream ifs(path);
        std::string text((std::istreambuf_iterator<char>(ifs)),
                         (std::istreambuf_iterator<char>()));
        this->text.open(text.c_str());
        retokenize();
    } else {
        // otherwise set the mode to FILE_EXPLORER
        mode = Mode::FILE_EXPLORER;
    }

    // register all the keys that depend on a lag
    register_key(Key::k_backspace, [&](InputManager &input) {
        if (mode == Mode::EDITING) {
            vertical_pos = -1;
            if (selection_start > -1) {
                backspace();
            } else {
                this->text.backspace_char();
            }
            retokenize();
        } else if (mode == Mode::SEARCHING) {
            if (search_text.length() > 0) search_text.pop_back();
        } else if (mode == Mode::NEW_FILE) {
            if (new_file_text.length() > 0) new_file_text.pop_back();
        }
    });

    // for any keys that are not up/down, reset the vertical pos
    register_key(Key::k_delete, [&](InputManager &input) {
        vertical_pos = -1;
        if (selection_start > -1) {
            backspace();
        } else {
            this->text.delete_char();
        }
        retokenize();
    });
    register_key(Key::k_enter, [&](InputManager &input) {
        if (mode == Mode::EDITING) {
            vertical_pos = -1;
            if (selection_start > -1) {
                backspace();
            }
            this->text.insert_char('\n');
            retokenize();
        } else if (mode == Mode::FILE_EXPLORER) {
            open(file_explorer.get_cwd_ls()[selected_idx]);

        } else if (mode == Mode::SEARCHING) {
            i32 s = this->text.search(this->text.cursor(), search_text);
            // re-search if it's the 2nd time we're searching
            if (s == this->text.cursor() &&
                this->text.cursor() < this->text.length()) {
                s = this->text.search(this->text.cursor() + 1, search_text);
            }
            if (s != -1) {
                this->text.move_cursor_to(s);
                retokenize();
            }
        } else if (mode == Mode::NEW_FILE) {
            new_file();
        }
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
        // ctrl jumping
        if (input.key_manager[Key::k_l_ctrl]) {
            // find next word
            Token *res = find_prev_token(this->text.cursor());
            if (res != nullptr) {
                u32 new_pos = this->text.get_index_from_pointer(res->text);
                this->text.move_cursor_to(new_pos);
            }
        }

        retokenize();
    });
    register_key(Key::k_right, [&](InputManager &input) {
        vertical_pos = -1;
        if (this->text.buff2() < this->text.tail()) {
            this->text.move_buffer(true);
        }
        // stop selecting
        if (!input.key_manager.key_pressed(Key::k_l_shift)) {
            selection_start = -1;
        }
        // ctrl jumping
        if (input.key_manager[Key::k_l_ctrl]) {
            // find next word
            Token *res = find_next_token(this->text.cursor());
            if (res != nullptr) {
                u32 new_pos = this->text.get_index_from_pointer(res->text);
                this->text.move_cursor_to(new_pos);
            }
        }
        retokenize();
    });
    register_key(Key::k_up, [&](InputManager &input) {
        if (mode == Mode::FILE_EXPLORER) {
            selected_idx++;
            if (selected_idx >= file_explorer.get_cwd_size()) {
                selected_idx = file_explorer.get_cwd_size() - 1;
            }
            return;
        }
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
        if (mode == Mode::FILE_EXPLORER) {
            if (selected_idx >= 1) {
                selected_idx--;
            }
            return;
        }
        u32 idx = this->text.find_line_end(this->text.cursor());
        if (idx == text.length()) {
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
    register_key(Key::k_tab, [&](InputManager &input) {
        for (u32 i = 0; i < 4; ++i) {
            this->text.insert_char(' ');
        }
        retokenize();
    });
}

void Editor::render(Font *font,
                    omega::scene::OrthographicCamera &camera,
                    omega::gfx::SpriteBatch &batch,
                    omega::gfx::ShapeRenderer &shape) {
    f32 height = font_render_height;
    f32 scale_factor = height / font->get_font_size();

    static omega::math::vec2 pos;
    if (mode == Mode::FILE_EXPLORER || mode == Mode::NEW_FILE) {
        font_renderer.set_view_proj_matrix(camera.get_projection_matrix());
        font_renderer.begin();
        i32 i = 0;
        for (auto path : file_explorer.get_cwd_ls()) {
            f32 render_height = height;
            omega::math::vec4 color{0.9f, 0.9f, 0.9f, 1.0f};
            if (selected_idx == i) {
                color = omega::util::color::white;
                render_height *= 1.3f;
            }
            if (std::filesystem::is_directory(path)) {
                path += "/";
            }
            font_renderer.render(
                font, path, {20.0f, 100 + 30.0f * i++}, render_height, color);
        }
        font_renderer.end();
        if (mode == Mode::NEW_FILE) {
            auto corner = omega::math::vec2{camera.get_width() - 300.0f,
                                            camera.get_height() - 40.0f};
            shape.begin();
            shape.set_view_projection_matrix(camera.get_projection_matrix());
            shape.color = omega::util::color::black;
            shape.rect({corner.x, corner.y, 300.0f, 60.0f});
            shape.end();

            font_renderer.set_view_proj_matrix(camera.get_projection_matrix());
            font_renderer.begin();
            font_renderer.render(font,
                                 "New File: ",
                                 {corner.x - 75.0f, corner.y + 10.0f},
                                 15.0f,
                                 {0.5f, 0.5f, 0.5f, 1.0f});
            font_renderer.render(font,
                                 new_file_text,
                                 {corner.x + 10.0f, corner.y + 10.0f},
                                 20.0f,
                                 omega::util::color::white);
            font_renderer.end();
        }
        return;
    }

    // some camera panning action! Recalculate the vp from the last frame pos
    omega::math::vec3 target_cam{0.0f};
    target_cam.x = pos.x - camera.get_width() * 0.25f;
    target_cam.y = pos.y - camera.get_height() * 0.5f;
    camera.position += (target_cam - camera.position) * 0.025f;
    camera.recalculate_view_matrix();

    buffer_renderer.begin();
    pos = buffer_renderer.render(font,
                                 camera.get_view_projection_matrix(),
                                 text,
                                 tokens,
                                 {0, 0},
                                 {0, 0},
                                 height,
                                 omega::util::color::white);
    // calculate cursor pos
    omega::math::vec2 cursor_pos_bottom = {pos.x, pos.y};
    omega::math::vec2 cursor_pos_top = {pos.x, pos.y + height * 0.8};

    // render the text batch finally
    buffer_renderer.end(camera.get_view_projection_matrix());

    // render cursor
    shape.begin();
    shape.set_view_projection_matrix(camera.get_view_projection_matrix());
    shape.color = omega::util::color::white;
    shape.line(cursor_pos_bottom, cursor_pos_top);

    // draw the selected text
    shape.color.a = 0.5f;
    buffer_renderer.render_selected(
        shape, font, text, selection_start, {0, 0}, height);
    shape.end();

    // render the file name
    font_renderer.set_view_proj_matrix(camera.get_projection_matrix());
    font_renderer.begin();
    font_renderer.render(
        font, file_explorer.get_current_file(), {10.0f, 10.0f}, 20.0f);
    font_renderer.end();

    // render find/replace box
    if (mode == Mode::SEARCHING) {
        auto corner = omega::math::vec2{camera.get_width() - 300.0f,
                                        camera.get_height() - 40.0f};
        shape.begin();
        shape.set_view_projection_matrix(camera.get_projection_matrix());
        shape.color = omega::util::color::black;
        shape.rect({corner.x, corner.y, 300.0f, 60.0f});
        shape.end();

        font_renderer.set_view_proj_matrix(camera.get_projection_matrix());
        font_renderer.begin();
        font_renderer.render(font,
                             "Search: ",
                             {corner.x - 75.0f, corner.y + 10.0f},
                             15.0f,
                             {0.5f, 0.5f, 0.5f, 1.0f});
        font_renderer.render(font,
                             search_text,
                             {corner.x + 10.0f, corner.y + 10.0f},
                             20.0f,
                             omega::util::color::white);
        font_renderer.end();
    }
}

void Editor::save(const std::string &file) {
    std::ofstream of;
    of.open(file);
    if (of.fail()) {
        OMEGA_ERROR("Failed to open file: '{}'", file);
    }

    of.write(text.head(), text.cursor());
    of.write(text.buff2(), text.buff2_size());
    of.close();
}

void Editor::handle_text(omega::events::InputManager &input, char c) {
    auto &keys = input.key_manager;
    if (mode == Mode::SEARCHING) {
        search_text.push_back(c);
    } else if (mode == Mode::NEW_FILE) {
        new_file_text.push_back(c);
    } else {
        // lock the text when ctrl is pressed
        if (!keys[omega::events::Key::k_l_ctrl]) {
            if (selection_start > -1) {
                backspace();
            }
            text.insert_char(c);
            retokenize();
        }
    }
}

void Editor::handle_input(omega::events::InputManager &input) {
    using namespace omega::events;
    auto &keys = input.key_manager;

    // save only in editing mode
    if (ctrl_char(keys, Key::k_s)) {
        if (mode == Mode::EDITING) {
            save(file_explorer.get_current_file());
        }
    }

    if (keys[Key::k_l_shift]) {
        if (keys[Key::k_left] || keys[Key::k_right] || keys[Key::k_up] ||
            keys[Key::k_down]) {
            if (selection_start == -1) {
                selection_start = text.cursor();
            }
        }
    }
    // INFO: update_keys that require a lag AFTER querying selection state to
    // track the old cursor (selection_start) before selecting even started
    update_keys(input);

    // search functionality
    if (keys[Key::k_l_ctrl] && keys[Key::k_f]) {
        mode = Mode::SEARCHING;
    }
    // escape search functionality
    if (keys[Key::k_escape] && mode == Mode::SEARCHING) {
        mode = Mode::EDITING;
    }

    // zooming
    if (ctrl_char(keys, Key::k_minus)) {
        font_render_height -= 5;
        if (font_render_height < 10.0f) {
            font_render_height = 10.0f;
        }
    }
    if (ctrl_char(keys, Key::k_plus)) {
        font_render_height += 5;
        if (font_render_height > 80.0f) {
            font_render_height = 80.0f;
        }
    }
    // clipboard functionality
    // copy
    if (ctrl_char(keys, Key::k_c)) {
        copy_to_clipboard();
    }
    // cut
    if (ctrl_char(keys, Key::k_x)) {
        copy_to_clipboard();
        if (selection_start > -1) {
            backspace();
            retokenize();
        }
    }
    // paste
    if (ctrl_char(keys, Key::k_v)) {
        char *paste = SDL_GetClipboardText();
        u32 len = strlen(paste);
        for (u32 i = 0; i < len; ++i) {
            const auto &c = paste[i];
            text.insert_char(c);
        }
        SDL_free(paste);
        retokenize();
    }
    // open file
    if (ctrl_char(keys, Key::k_o)) {
        mode = Mode::FILE_EXPLORER;
    }
    // new file
    if (ctrl_char(keys, Key::k_n)) {
        if (mode == Mode::FILE_EXPLORER) {
            mode = Mode::NEW_FILE;
        }
    }
}

void Editor::retokenize() {
    tokens.clear();

    lexer.retokenize();
    Token token = lexer.next();
    while (token.type != TokenType::END) {
        tokens.push_back(token);
        token = lexer.next();
    }
}

void Editor::backspace() {
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
}

void Editor::copy_to_clipboard() {
    // save substring to clipboard
    if (selection_start != -1) {
        if (selection_start > text.cursor()) {
            SDL_SetClipboardText(
                text.substr(text.cursor(), selection_start - text.cursor())
                    .c_str());
        } else {
            SDL_SetClipboardText(
                text.substr(selection_start, text.cursor() - selection_start)
                    .c_str());
        }
    }
}

void Editor::open(const std::string &file) {
    std::string content;
    bool is_directory = file_explorer.open(file, content);
    if (!is_directory) {
        // reset all the vars
        vertical_pos = -1;
        selection_start = 0;
        mode = Mode::EDITING;
        selected_idx = 0;
        this->text.open(content.c_str());
        retokenize();
    }
    // otherwise, this is a CHANGE DIRETORY OPERATION
    else {
        // update the selected index to stay the same or wrap to the new
        // current directory size
        selected_idx =
            omega::math::min(selected_idx, file_explorer.get_cwd_size() - 1);
    }
}

void Editor::new_file() {
    // WARN: Please just use a valid file/directory name, no funny
    // business :)
    if (new_file_text == "") {
        return;
    }
    std::filesystem::path new_path = file_explorer.get_cwd() / new_file_text;
    // directory
    if (new_file_text.ends_with("/")) {
        std::filesystem::create_directory(new_path);
        file_explorer.change_directory(new_path);
        mode = Mode::FILE_EXPLORER;
    } else {
        if (!std::filesystem::exists(new_path)) {
            // set the default text to ""
            this->text.open("");
            save(new_path.string());
            std::string unused;
            file_explorer.open(new_path.string(), unused);
            retokenize();
        } else {
            // just open the file otherwise
            open(new_path.string());
        }
        // reset the new file text
        new_file_text.clear();
        mode = Mode::EDITING;
    }
}

Token *Editor::find_prev_token(u32 i) {
    const char *c = &text.get(i);
    // handle edge case where c == tokens[0].text
    if (c == tokens[0].text) {
        return &tokens[0];
    }
    // perform a binary search
    i32 start = 0, end = tokens.size() - 1;
    Token *res = nullptr;
    while (start <= end) {
        i32 mid = (start + end) / 2;
        Token &token = tokens[mid];

        if (token.text < c) {
            res = &token;
            start = mid + 1;
        } else {
            end = mid - 1;
        }
    }
    return res;
}

Token *Editor::find_next_token(u32 i) {
    const char *c = &text.get(i);
    // check edge case where c == tokens.back().text + token.len
    if (c == text.tail()) {
        return &tokens.back();
    }
    // perform a binary search
    i32 start = 0, end = tokens.size() - 1;
    Token *res = nullptr;
    while (start <= end) {
        i32 mid = (start + end) / 2;
        Token &token = tokens[mid];

        if (token.text + token.len > c) {
            res = &token;
            end = mid - 1;
        } else {
            start = mid + 1;
        }
    }
    return res;
}

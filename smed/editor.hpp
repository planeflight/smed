#ifndef SMED_EDITOR_HPP
#define SMED_EDITOR_HPP

#include <cstring>
#include <vector>

#include "omega/core/globals.hpp"
#include "omega/events/input_manager.hpp"
#include "omega/gfx/shader.hpp"
#include "omega/gfx/shape_renderer.hpp"
#include "omega/gfx/sprite_batch.hpp"
#include "omega/scene/orthographic_camera.hpp"
#include "omega/util/types.hpp"
#include "smed/font.hpp"
#include "smed/font_renderer.hpp"
#include "smed/gap_buffer.hpp"
#include "smed/lexer.hpp"

class Editor {
  public:
    Editor(omega::gfx::Shader *shader, Font *font, const std::string &text);

    void render(Font *font,
                omega::scene::OrthographicCamera &camera,
                omega::gfx::ShapeRenderer &shape);
    void save(const std::string &file);

    void handle_text(char c);
    void handle_input(omega::events::InputManager &input);

  private:
    void retokenize();
    void search(const std::string &text);

    GapBuffer text;
    Lexer lexer;
    std::vector<Token> tokens;
    u32 cursor_idx;
    i32 vertical_pos = -1; // represents the initial up/down cursor column, -1
                           // when none has been initiated
    FontRenderer font_renderer;

    struct CursorPos {
        omega::math::vec2 bottom, top;
    } cursor_pos, selection_start_pos;

    i32 selection_start = -1; // -1 represents no selection
    i32 selection_size = 0;   // can be forwards/backwards
};

#endif // SMED_EDITOR_HPP

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

    void handle_text(omega::events::InputManager &input, char c);
    void handle_input(omega::events::InputManager &input);

  private:
    void retokenize();

    GapBuffer text;
    Lexer lexer;
    std::vector<Token> tokens;
    i32 vertical_pos = -1; // represents the initial up/down cursor column, -1
                           // when none has been initiated
    FontRenderer font_renderer;

    i32 selection_start = -1; // -1 represents no selection
    enum class Mode { EDITING = 0, SEARCHING } mode = Mode::EDITING;
    f32 font_render_height = 25.0f;
};

#endif // SMED_EDITOR_HPP

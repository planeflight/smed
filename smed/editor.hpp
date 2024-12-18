#ifndef SMED_EDITOR_HPP
#define SMED_EDITOR_HPP

#include <cstring>
#include <vector>

#include "omega/core/globals.hpp"
#include "omega/events/input_manager.hpp"
#include "omega/gfx/shader.hpp"
#include "omega/gfx/shape_renderer.hpp"
#include "omega/gfx/sprite_batch.hpp"
#include "omega/util/types.hpp"
#include "smed/font.hpp"
#include "smed/font_renderer.hpp"
#include "smed/gap_buffer.hpp"

class Editor {
  public:
    Editor(omega::gfx::Shader *shader);

    void render(Font *font,
                const omega::math::mat4 &view_proj,
                omega::gfx::ShapeRenderer &shape);
    void shape_render(omega::gfx::ShapeRenderer &shape);
    void save(const std::string &file);

    void handle_text(char c);
    void handle_input(omega::events::InputManager &input);

  private:
    GapBuffer text;
    omega::math::ivec2 cursor{0};
    char *current_line = nullptr;
    u32 cursor_idx;
    FontRenderer font_renderer;
};

#endif // SMED_EDITOR_HPP

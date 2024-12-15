#ifndef SMED_EDITOR_HPP
#define SMED_EDITOR_HPP

#include <cstring>
#include <vector>

#include "omega/core/globals.hpp"
#include "omega/events/input_manager.hpp"
#include "omega/gfx/shape_renderer.hpp"
#include "omega/gfx/sprite_batch.hpp"
#include "omega/ui/font.hpp"
#include "omega/util/types.hpp"
#include "smed/gap_buffer.hpp"

class Editor {
  public:
    Editor();

    void render(omega::ui::Font *font, omega::gfx::SpriteBatch &batch);
    void shape_render(omega::gfx::ShapeRenderer &shape);
    void save(const std::string &file);

    void handle_text(char c);
    void handle_input(omega::events::InputManager &input);

  private:
    GapBuffer text;
    omega::math::ivec2 cursor{0};
    char *current_line = nullptr;
    u32 cursor_idx;
};

#endif // SMED_EDITOR_HPP

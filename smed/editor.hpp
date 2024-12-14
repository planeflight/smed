#ifndef SMED_EDITOR_HPP
#define SMED_EDITOR_HPP

#include "omega/core/globals.hpp"
#include "omega/events/input_manager.hpp"
#include "omega/gfx/shape_renderer.hpp"
#include "omega/gfx/sprite_batch.hpp"
#include "omega/ui/font.hpp"
#include "omega/util/types.hpp"

struct GapBuffer {
    i32 default_gap = 16;
    char *start = nullptr;
    char *gap_start;
    i32 gap_size = default_gap;
    char *end = nullptr;
};

struct Text {
    std::list<std::string> lines;
};

class Editor {
  public:
    Editor();

    void render(omega::ui::Font *font, omega::gfx::SpriteBatch &batch);
    void shape_render(omega::gfx::ShapeRenderer &shape);
    void save(const std::string &file);

    void handle_text(char c);
    void handle_input(omega::events::InputManager &input);

  private:
    Text text;
    omega::math::ivec2 cursor{0};
    std::list<std::string>::iterator current_line;
};

#endif // SMED_EDITOR_HPP

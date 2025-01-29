#ifndef SMED_EDITOR_HPP
#define SMED_EDITOR_HPP

#include <cstring>
#include <omega/core/globals.hpp>
#include <omega/events/input_manager.hpp>
#include <omega/gfx/shader.hpp>
#include <omega/gfx/shape_renderer.hpp>
#include <omega/gfx/sprite_batch.hpp>
#include <omega/scene/orthographic_camera.hpp>
#include <omega/util/types.hpp>
#include <string>
#include <vector>

#include "smed/buffer_renderer.hpp"
#include "smed/files.hpp"
#include "smed/font.hpp"
#include "smed/font_renderer.hpp"
#include "smed/gap_buffer.hpp"
#include "smed/lexer.hpp"

class Editor {
  public:
    Editor(omega::gfx::Shader *shader,
           omega::gfx::Shader *shader_search,
           Font *font,
           std::string path);

    void render(Font *font,
                omega::scene::OrthographicCamera &camera,
                omega::gfx::SpriteBatch &batch,
                omega::gfx::ShapeRenderer &shape);
    void save(const std::string &file);

    void handle_text(omega::events::InputManager &input, char c);
    void handle_input(omega::events::InputManager &input);

  private:
    void retokenize();
    void backspace();
    void copy_to_clipboard();
    void open(const std::string &file);
    void new_file();

    GapBuffer text;
    Lexer lexer;
    std::vector<Token> tokens;
    i32 vertical_pos = -1; // represents the initial up/down cursor column, -1
                           // when none has been initiated

    BufferRenderer font_renderer;

    i32 selection_start = -1; // -1 represents no selection
    f32 font_render_height = 25.0f;

    // searching
    enum class Mode {
        EDITING = 0,
        SEARCHING,
        FILE_EXPLORER,
        NEW_FILE
    } mode = Mode::EDITING;
    std::string search_text;
    FontRenderer search_renderer;

    // directory/file management
    FileExplorer file_explorer;
    u32 selected_idx = 0;
    std::string new_file_text;
};

#endif // SMED_EDITOR_HPP

#ifndef SMED_BUFFERRENDERER_HPP
#define SMED_BUFFERRENDERER_HPP

#include <cstring>
#include <vector>

#include "omega/core/platform.hpp"
#include "omega/gfx/gl.hpp"
#include "omega/gfx/shader.hpp"
#include "omega/gfx/shape_renderer.hpp"
#include "omega/gfx/vertex_array.hpp"
#include "omega/gfx/vertex_buffer.hpp"
#include "omega/gfx/vertex_buffer_layout.hpp"
#include "omega/util/color.hpp"
#include "omega/util/std.hpp"
#include "omega/util/time.hpp"
#include "smed/font.hpp"
#include "smed/gap_buffer.hpp"
#include "smed/lexer.hpp"

class BufferRenderer {
  public:
    BufferRenderer(omega::gfx::Shader *shader) : shader(shader) {
        // create the Vertex buffer and array
        vbo = omega::util::create_uptr<omega::gfx::VertexBuffer>(
            sizeof(Vertex) * 6 * quad_count);
        vao = omega::util::create_uptr<omega::gfx::VertexArray>();

        omega::gfx::VertexBufferLayout layout;
        layout.push(OMEGA_GL_FLOAT, 2);
        layout.push(OMEGA_GL_FLOAT, 2);
        layout.push(OMEGA_GL_FLOAT, 4);
        vao->add_buffer(*vbo, layout);
    }

    void begin() {
        quads_rendered = 0;
    }

    omega::math::vec2 render(Font *font,
                             const omega::math::mat4 &view_proj,
                             GapBuffer &gap_buffer,
                             const std::vector<Token> &tokens,
                             omega::math::vec2 origin,
                             omega::math::vec2 pos,
                             f32 height,
                             const omega::math::vec4 &color) {
        // TODO: add more fonts
        font->get_texture()->bind(0);
        Vertex vertices[6];
        f32 scale_factor = height / font->get_font_size();

        // track the space width, so pos.x can be increased appropriately
        u32 space_width = font->get_glyph('a').advance.x;

        // WARN: Messy solution to rendering each character without a separate
        // helper function
        const auto token_render = [&](const Token &token) {
            auto col = color;
            switch (token.type) {
                case TokenType::KEYWORD:
                    col = {0.9f, 0.2f, 0.3f, 1.0f};
                    break;
                case TokenType::STRING:
                    col = {0.4f, 0.8f, 0.2f, 1.0f};
                    break;
                case TokenType::TYPE:
                    col = {1.0f, 0.85f, 0.2f, 1.0f};
                    break;
                case TokenType::NUMBER:
                    col = {1.0f, 0.4f, 1.0f, 1.0f};
                    break;
                case TokenType::PREPROCESSOR:
                    col = {0.7f, 0.6f, 0.7f, 1.0f};
                    break;
                case TokenType::COMMENT:
                    col = {0.5f, 0.5f, 0.5f, 1.0f};
                    break;
                case TokenType::CLOSE_PAREN:
                case TokenType::OPEN_PAREN:
                case TokenType::OPEN_CURLY:
                case TokenType::CLOSE_CURLY:
                    col = {0.6f, 0.7f, 0.7f, 1.0f};
                    break;
                case TokenType::EQUALS:
                case TokenType::LT:
                case TokenType::GT:
                case TokenType::ASSIGNMENT:
                case TokenType::GOT:
                case TokenType::LOT:
                case TokenType::NOT_EQUAL:
                case TokenType::NOT:
                case TokenType::PLUS:
                case TokenType::MINUS:
                case TokenType::MUL:
                case TokenType::DIV:
                case TokenType::MOD:
                case TokenType::SCOPE:
                case TokenType::AND:
                case TokenType::OR:
                    col = {0.4f, 0.6f, 0.85f, 1.0f};
                    break;
                default:
                    col = color;
                    break;
            }

            for (u32 i = 0; i < token.len; ++i) {
                // get adjusted start index
                u32 start_idx = gap_buffer.get_index_from_pointer(token.text);
                char c = gap_buffer.get(start_idx + i);
                const Glyph &glyph = font->get_glyph(c);

                // actual render pos
                omega::math::rectf src{(f32)glyph.tex_coords.x,
                                       (f32)glyph.tex_coords.y,
                                       (f32)glyph.size.x,
                                       (f32)glyph.size.y};
                // normalize the src rectangle (texture coordinates)
                src.x = src.x / font->get_texture()->get_width();
                src.y = src.y / font->get_texture()->get_height();
                src.w = src.w / font->get_texture()->get_width();
                src.h = src.h / font->get_texture()->get_height();

                // compute the destination rectangle
                omega::math::rectf dest{
                    origin.x +
                        (token.pos.x + i * glyph.advance.x) * scale_factor +
                        glyph.offset.x * scale_factor,
                    origin.y - (token.pos.y * scale_factor) -
                        (glyph.size.y - glyph.offset.y) * scale_factor,
                    glyph.size.x * scale_factor,
                    glyph.size.y * scale_factor};

                // create the vertices, inverting y up
                vertices[0] = {{dest.x, dest.y}, {src.x, src.y + src.h}, col};
                vertices[1] = {{dest.x + dest.w, dest.y},
                               {src.x + src.w, src.y + src.h},
                               col};
                vertices[2] = {{dest.x + dest.w, dest.y + dest.h},
                               {src.x + src.w, src.y},
                               col};
                vertices[3] = vertices[2];
                vertices[4] = {{dest.x, dest.y + dest.h}, {src.x, src.y}, col};
                vertices[5] = vertices[0];

                // send the new Quad to the gpu
                vbo->bind();
                vbo->sub_data(sizeof(Vertex) * 6 * quads_rendered,
                              sizeof(Vertex) * 6,
                              vertices);
                vbo->unbind();
                quads_rendered++;
                if (quads_rendered == quad_count) {
                    end(view_proj);
                    begin();
                }
            }
        };

        // render the text
        for (const auto &token : tokens) {
            // TODO: only render what will actually be rendered
            token_render(token);
        }

        // calculate cursor pos
        omega::math::vec2 cursor = pos;
        for (u32 i = 0; i < gap_buffer.buff1_size() + gap_buffer.gap_idx; ++i) {
            char c = gap_buffer.text[i];
            if (c == '\n') {
                cursor.y -= font->get_font_height() * scale_factor;
                cursor.x = origin.x;
                continue;
            }
            const auto &glyph = font->get_glyph(c);
            cursor.x += glyph.advance.x * scale_factor;
        }
        return cursor;
    }

    void end(const omega::math::mat4 &view_proj) {
        // render the character batch
        shader->bind();
        vbo->bind();
        vao->bind();
        shader->set_uniform_mat4f("u_view_proj", view_proj);
        shader->set_uniform_1i("u_texture", 0);
        shader->set_uniform_1f("u_time", omega::util::time::get_time<f32>());
        omega::gfx::draw_arrays(OMEGA_GL_TRIANGLES, 0, quads_rendered * 6);

        vao->unbind();
        vbo->unbind();
        shader->unbind();
    }

    void render_selected(omega::gfx::ShapeRenderer &shape,
                         Font *font,
                         GapBuffer &gap_buffer,
                         i32 selection_start,
                         const omega::math::vec2 &pos,
                         f32 height) {
        auto origin = pos;

        f32 scale_factor = height / font->get_font_size();
        // render the highlighted bits
        omega::math::vec2 render_pos = pos;
        if (selection_start > -1) {
            i32 cursor_idx = gap_buffer.cursor();
            // cursor is after selection start
            if (cursor_idx > selection_start) {
                for (i32 i = 0; i < cursor_idx; i++) {
                    char c = gap_buffer.get(i);
                    if (c == '\n') {
                        render_pos.y -= font->get_font_height() * scale_factor;
                        render_pos.x = origin.x;
                        continue;
                    }
                    const auto &glyph = font->get_glyph(c);
                    if (i >= selection_start) {
                        shape.rect(
                            {render_pos.x,
                             render_pos.y -
                                 font->get_font_height() * 0.2f * scale_factor,
                             glyph.advance.x * scale_factor,
                             (f32)font->get_font_height() * scale_factor});
                    }
                    render_pos.x += glyph.advance.x * scale_factor;
                }
            } else if (cursor_idx < selection_start) {
                for (i32 i = 0; i < selection_start; i++) {
                    char c = gap_buffer.get(i);
                    if (c == '\n') {
                        render_pos.y -= font->get_font_height() * scale_factor;
                        render_pos.x = origin.x;
                        continue;
                    }
                    const auto &glyph = font->get_glyph(c);
                    if (i >= cursor_idx) {
                        shape.rect(
                            {render_pos.x,
                             render_pos.y -
                                 font->get_font_height() * 0.2f * scale_factor,
                             glyph.advance.x * scale_factor,
                             (f32)font->get_font_height() * scale_factor});
                    }
                    render_pos.x += glyph.advance.x * scale_factor;
                }
            }
        }
    }

  private:
    struct Vertex {
        omega::math::vec2 pos;
        omega::math::vec2 tex_coords;
        omega::math::vec4 color;
    };
    u32 quad_count = 500; // so we can draw lots of chars
    u32 quads_rendered = 0;

    omega::util::uptr<omega::gfx::VertexBuffer> vbo = nullptr;
    omega::util::uptr<omega::gfx::VertexArray> vao = nullptr;
    omega::gfx::Shader *shader = nullptr;
};

#endif // SMED_BUFFERRENDERER_HPP

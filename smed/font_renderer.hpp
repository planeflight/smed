#ifndef SMED_FONTRENDERER_HPP
#define SMED_FONTRENDERER_HPP

#include "omega/core/platform.hpp"
#include "omega/gfx/gl.hpp"
#include "omega/gfx/shader.hpp"
#include "omega/gfx/vertex_array.hpp"
#include "omega/gfx/vertex_buffer.hpp"
#include "omega/gfx/vertex_buffer_layout.hpp"
#include "omega/util/std.hpp"
#include "smed/font.hpp"
#include "smed/gap_buffer.hpp"

class FontRenderer {
  public:
    FontRenderer(omega::gfx::Shader *shader) : shader(shader) {
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

    void begin(const omega::math::mat4 &view_proj) {
        shader->bind();
        shader->set_uniform_mat4f("u_view_proj", view_proj);
        quads_rendered = 0;
    }

    omega::math::vec2 render(Font *font,
                             const GapBuffer &gap_buffer,
                             omega::math::vec2 origin,
                             omega::math::vec2 pos,
                             f32 height,
                             const glm::vec4 &color) {
        // TODO: add more fonts
        font->get_texture()->bind(0);
        Vertex vertices[6];
        f32 scale_factor = height / font->get_font_size();
        omega::math::vec2 cursor = pos;

        // track the space width, so pos.x can be increased appropriately
        u32 space_width = font->get_glyph('a').advance.x;

        // WARN: Messy solution to rendering each character without a separate
        // helper function
        const auto character = [&](const char &c) {
            if (c == '\n') {
                pos.y -= font->get_font_height() *
                         scale_factor; // subtract for inverted y axis
                pos.x = origin.x;
                return;
            }
            if (c == ' ') {
                pos.x += space_width * scale_factor;
                return;
            }
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
                pos.x + glyph.offset.x * scale_factor,
                pos.y - (glyph.size.y - glyph.offset.y) * scale_factor,
                glyph.size.x * scale_factor,
                glyph.size.y * scale_factor};

            // create the vertices, inverting y up
            vertices[0] = {{dest.x, dest.y}, {src.x, src.y + src.h}, color};
            vertices[1] = {{dest.x + dest.w, dest.y},
                           {src.x + src.w, src.y + src.h},
                           color};
            vertices[2] = {{dest.x + dest.w, dest.y + dest.h},
                           {src.x + src.w, src.y},
                           color};
            vertices[3] = vertices[2];
            vertices[4] = {{dest.x, dest.y + dest.h}, {src.x, src.y}, color};
            vertices[5] = vertices[0];

            // send the new Quad to the gpu
            vbo->bind();
            vbo->sub_data(sizeof(Vertex) * 6 * quads_rendered,
                          sizeof(Vertex) * 6,
                          vertices);
            vbo->unbind();
            quads_rendered++;

            // advange the position
            pos.x += glyph.advance.x * scale_factor;
        };

        // first iterate through all the characters up to the cursor
        for (u32 i = 0; i < gap_buffer.buff1_size() + gap_buffer.gap_idx; ++i) {
            character(gap_buffer.text[i]);
        }
        // store the cursor location
        cursor = pos;
        // draw the rest of the characters
        for (u32 i = 0; i < gap_buffer.buff2_size(); ++i) {
            character(gap_buffer.gap_end[i]);
        }
        return cursor;
    }

    void end() {
        // render the character batch
        shader->bind();
        vbo->bind();
        vao->bind();
        shader->set_uniform_1i("u_texture", 0);
        omega::gfx::draw_arrays(OMEGA_GL_TRIANGLES, 0, quads_rendered * 6);

        vao->unbind();
        vbo->unbind();
        shader->unbind();
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

#endif // SMED_FONTRENDERER_HPP

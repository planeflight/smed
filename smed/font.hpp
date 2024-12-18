#ifndef SMED_FONT_HPP
#define SMED_FONT_HPP

#include <iostream>
#include <string>

#include "ft2build.h"
#include "omega/core/platform.hpp"
#include "omega/gfx/sprite_batch.hpp"
#include "omega/gfx/texture/texture.hpp"
#include "omega/util/std.hpp"
#include FT_FREETYPE_H
#include "omega/math/math.hpp"
#include "omega/util/log.hpp"
#include "omega/util/types.hpp"

struct Glyph {
    omega::math::vec2 offset;
    omega::math::vec2 advance; // pixels skipped till next character
    omega::math::ivec2 tex_coords;
    omega::math::ivec2 size;
};

class Font {
  public:
    // Taken from Cakez: https://www.youtube.com/watch?v=23x0nGzHQgY
    Font(const std::string &path, u32 height = 64) : font_size(height) {
        FT_Face face;
        FT_Error error = FT_New_Face(library, path.c_str(), 0, &face);
        if (error == FT_Err_Unknown_File_Format) {
            omega::util::err("Unknown file format for '{}'", path);
        } else if (error) {
            omega::util::err("Error opening file '{}'", path);
        }
        FT_Set_Pixel_Sizes(face, 0, height);
        i32 padding = 1; // space between chars
        i32 row = 0;
        i32 col = padding;
        const i32 texture_width = 512;
        char texture_buffer[texture_width * texture_width] = {0};

        for (i8 char_idx = 32; char_idx < 127; ++char_idx) {
            FT_UInt glyph_index = FT_Get_Char_Index(face, char_idx);
            FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
            FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
            // check if glyph will fit in current row
            if (col + face->glyph->bitmap.width + padding >= texture_width) {
                col = padding;
                row += height;
            }
            // update the actual font height
            font_height = omega::math::max((face->size->metrics.ascender -
                                            face->size->metrics.descender) >>
                                               6,
                                           (i64)font_height);
            for (u32 y = 0; y < face->glyph->bitmap.rows; ++y) {
                for (u32 x = 0; x < face->glyph->bitmap.width; ++x) {
                    texture_buffer[(row + y) * texture_width + col + x] =
                        face->glyph->bitmap
                            .buffer[y * face->glyph->bitmap.width + x];
                }
            }
            Glyph *glyph = &glyphs[char_idx];
            glyph->tex_coords = {col, row};
            glyph->size = {face->glyph->bitmap.width, face->glyph->bitmap.rows};
            glyph->advance = {face->glyph->advance.x >> 6,
                              face->glyph->advance.y >> 6};
            glyph->offset = {face->glyph->bitmap_left, face->glyph->bitmap_top};
            col += face->glyph->bitmap.width + padding;
        }
        FT_Done_Face(face);
        u32 id = 0;
        glGenTextures(1, &id);
        texture = omega::gfx::texture::Texture::create_wrapper(
            id, texture_width, texture_width);
        texture->bind(0);
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RED,
                     texture_width,
                     texture_width,
                     0,
                     GL_RED,
                     GL_UNSIGNED_BYTE,
                     texture_buffer);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    ~Font() {}

    static void init() {
        FT_Error error = FT_Init_FreeType(&library);
        if (error) {
            omega::util::err("Failed to initialize FreeType.\n");
        }
    }

    static void quit() {
        FT_Done_FreeType(library);
    }

    void render(omega::gfx::SpriteBatch &batch,
                const char *text,
                u32 length,
                omega::math::vec2 pos,
                f32 height,
                const glm::vec4 &color) {
        f32 scale_factor = height / font_size;

        omega::math::vec2 origin = pos;
        for (u32 i = 0; i < length; ++i) {
            const auto &c = text[i];
            if (c == '\n') {
                pos.y -=
                    font_height * scale_factor; // subtract for inverted y axis
                pos.x = origin.x;
                continue;
            }
            if (c == ' ') {
                pos.x += font_size * scale_factor;
                continue;
            }
            const Glyph &glyph = glyphs[c];

            // actual render pos
            omega::math::rectf src{(f32)glyph.tex_coords.x,
                                   (f32)glyph.tex_coords.y,
                                   (f32)glyph.size.x,
                                   (f32)glyph.size.y};

            omega::math::rectf dest{
                pos.x + glyph.offset.x * scale_factor,
                pos.y - (glyph.size.y - glyph.offset.y) * scale_factor,
                glyph.size.x * scale_factor,
                glyph.size.y * scale_factor};

            batch.render_texture(texture.get(), src, dest);
            pos.x += glyph.advance.x * scale_factor;
        }
    }

    u32 get_font_size() const {
        return font_size;
    }
    u32 get_font_height() const {
        return font_height;
    }
    const Glyph &get_glyph(char c) const {
        return glyphs[c];
    }

    omega::gfx::texture::Texture *get_texture() {
        return texture.get();
    }

  private:
    omega::util::sptr<omega::gfx::texture::Texture> texture = nullptr;
    Glyph glyphs[127];
    static FT_Library library;
    u32 font_height = 0; // total spacing factor
    u32 font_size = 0;   // size in pixels
};

#endif // SMED_FONT_HPP

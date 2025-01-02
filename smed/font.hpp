#ifndef SMED_FONT_HPP
#define SMED_FONT_HPP

#include <iostream>
#include <omega/core/platform.hpp>
#include <omega/gfx/sprite_batch.hpp>
#include <omega/gfx/texture/texture.hpp>
#include <omega/util/std.hpp>
#include <string>

#include "ft2build.h"
#include FT_FREETYPE_H
#include <omega/math/math.hpp>
#include <omega/util/log.hpp>
#include <omega/util/types.hpp>

struct Glyph {
    omega::math::vec2 offset;
    omega::math::vec2 advance; // pixels skipped till next character
    omega::math::ivec2 tex_coords;
    omega::math::ivec2 size;
};

class Font {
  public:
    // Taken from Cakez: https://www.youtube.com/watch?v=23x0nGzHQgY
    Font(const std::string &path, u32 height = 64);

    ~Font() = default;

    static void init() {
        FT_Error error = FT_Init_FreeType(&library);
        if (error) {
            OMEGA_ERROR("Failed to initialize FreeType.\n");
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
                const glm::vec4 &color);

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

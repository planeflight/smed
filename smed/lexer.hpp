#ifndef SMED_LEXER_HPP
#define SMED_LEXER_HPP

#include <cstddef>
#include <omega/math/math.hpp>
#include <string>

#include "smed/font.hpp"
#include "smed/gap_buffer.hpp"

// The following initial implementation is Tsoding's:
// https://www.youtube.com/watch?v=AqyZztKlSGQ&list=PLpM-Dvs8t0VZVshbPeHPculzFFBdQWIFu&index=15
enum class TokenType {
    END = 0,
    INVALID,
    PREPROCESSOR,
    SYMBOL,
    OPEN_PAREN,
    CLOSE_PAREN,
    OPEN_CURLY,
    CLOSE_CURLY,
    SEMICOLON,
    KEYWORD,
    TYPE,
    NUMBER,
    STRING
};

struct Token {
    TokenType type;
    const char *text;
    size_t len = 0;

    std::string to_string();

    // INFO: the next part is only for rendering
    omega::math::vec2 pos{0.0f};
};

struct Lexer {
    Lexer(GapBuffer *text, Font *font);

    Token next();
    void retokenize();

    GapBuffer *text;

    size_t idx;
    size_t line;
    size_t line_start;

  private:
    void trim_left();
    char chop_char();
    bool starts_with(const char *prefix, size_t prefix_len);

    // INFO: the next part is only for rendering
    Font *font = nullptr;
    omega::math::vec2 pos{0.0f};
};

#endif // SMED_LEXER_HPP

#ifndef SMED_LEXER_HPP
#define SMED_LEXER_HPP

#include <cstddef>
#include <string>

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
    NUMBER
};

struct Token {
    TokenType type;
    const char *text;
    size_t len = 0;

    std::string to_string();
};

struct Lexer {
    Lexer(const char *text, size_t len);

    Token next();

    const char *text;
    size_t len;

    size_t idx;
    size_t line;
    size_t line_start;

  private:
    void trim_left();
    char chop_char();
    bool starts_with(const char *prefix, size_t prefix_len);
};

#endif // SMED_LEXER_HPP

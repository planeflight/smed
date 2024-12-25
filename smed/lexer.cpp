#include "lexer.hpp"

#include <cctype>
#include <cstring>

constexpr static char *keywords[] = {
    "auto",
    "break",
    "case",
    "char",
    "const",
    "continue",
    "default",
    "do",
    "double",
    "else",
    "enum",
    "extern",
    "for",
    "goto",
    "if",
    "register",
    "return",
    "signed",
    "sizeof",
    "static",
    "struct",
    "switch",
    "typedef",
    "union",
    "unsigned",
    "volatile",
    "while",
    "alignas",
    "alignof",
    "and",
    "and_eq",
    "asm",
    "atomic_cancel",
    "atomic_commit",
    "atomic_noexcept",
    "bitand",
    "bitor",
    "bool",
    "catch",
    "class",
    "co_await",
    "co_return",
    "co_yield",
    "compl",
    "concept",
    "const_cast",
    "consteval",
    "constexpr",
    "constinit",
    "decltype",
    "delete",
    "dynamic_cast",
    "explicit",
    "export",
    "friend",
    "inline",
    "mutable",
    "namespace",
    "new",
    "noexcept",
    "not",
    "not_eq",
    "nullptr",
    "operator",
    "or",
    "or_eq",
    "private",
    "protected",
    "public",
    "reflexpr",
    "reinterpret_cast",
    "requires",
    "static_assert",
    "static_cast",
    "synchronized",
    "template",
    "this",
    "thread_local",
    "throw",
    "try",
    "typeid",
    "typename",
    "using",
    "virtual",
    "wchar_t",
    "xor",
    "xor_eq",
    "#include",
    "#ifndef",
    "#ifdef",
    "#endif",
    "#else",
};

constexpr static char *builtin_types[] =
    {"int", "short", "unsigned", "float", "char", "long", "double", "void"};

std::string Token::to_string() {
    switch (type) {
        case TokenType::END:
            return "End of Content";
        case TokenType::INVALID:
            return "Invalid";
        case TokenType::PREPROCESSOR:
            return "Preprocessor directive";
        case TokenType::SYMBOL:
            return "Symbol";
        case TokenType::OPEN_PAREN:
            return "Open parentheses";
        case TokenType::CLOSE_PAREN:
            return "Close paratheses";
        case TokenType::OPEN_CURLY:
            return "Open curly";
        case TokenType::CLOSE_CURLY:
            return "Close curly";
        case TokenType::SEMICOLON:
            return "Semicolon";
        case TokenType::KEYWORD:
            return "Keyword";
        case TokenType::TYPE:
            return "Type";
        case TokenType::NUMBER:
            return "Number";
        case TokenType::STRING:
            return "String";
    }
    return "";
}

Lexer::Lexer(GapBuffer *gap_buffer, Font *font)
    : text(gap_buffer), idx(0), line(0), line_start(0), font(font) {}

void Lexer::retokenize() {
    idx = 0;
    line = 0;
    line_start = 0;
    pos = {0.0f, 0.0f};
}

void Lexer::trim_left() {
    while (idx < text->length() && isspace(text->get(idx))) {
        chop_char();
    }
}

bool is_symbol_start(char x) {
    // can't start with a num
    return isalpha(x) || x == '_';
}

bool is_symbol(char x) {
    // can be a num
    return isalnum(x) || x == '_';
}

bool is_num(char x) {
    return isdigit(x) || x == '.' || x == 'f' || x == 'u' || x == 'b' ||
           x == 'x';
}

bool is_num_start(char x) {
    return isdigit(x);
}

char Lexer::chop_char() {
    char x = text->get(idx);
    idx++;
    if (x == '\n') {
        line++;
        line_start = idx;
        pos.y += font->get_font_height();
        pos.x = 0.0f;
    } else {
        pos.x += font->get_glyph(x).advance.x;
    }
    return x;
}

bool Lexer::starts_with(const char *prefix, size_t prefix_len) {
    if (prefix_len == 0) {
        return true;
    }
    if (idx + prefix_len - 1 >= text->length()) {
        return false;
    }

    // check if the prefix is equal to the current idx text
    for (size_t i = 0; i < prefix_len; ++i) {
        if (prefix[i] != text->get(idx + i)) {
            return false;
        }
    }
    return true;
}

Token Lexer::next() {
    trim_left();
    Token token{TokenType::END, &text->get(idx), 0, pos};
    if (idx >= text->length()) return token;

    size_t len = text->length();

    if (text->get(idx) == '#') {
        token.type = TokenType::PREPROCESSOR;
        while (idx < len && text->get(idx) != '\n') {
            token.len++;
            chop_char();
        }
        // get last piece
        if (idx < len) {
            chop_char();
        }
        return token;
    }

    const char literal_tokens[] = {'(', ')', '{', '}', ';'};
    const TokenType types[] = {TokenType::OPEN_PAREN,
                               TokenType::CLOSE_PAREN,
                               TokenType::OPEN_CURLY,
                               TokenType::CLOSE_CURLY,
                               TokenType::SEMICOLON};
    for (size_t i = 0; i < 5; ++i) {
        char c = literal_tokens[i];
        if (c == text->get(idx)) {
            token.type = types[i];
            token.len++;
            chop_char();
            return token;
        }
    }

    // strings
    if (text->get(idx) == '"') {
        token.type = TokenType::STRING;
        // get first piece
        chop_char();
        token.len++;
        while (idx < len && text->get(idx) != '"') {
            chop_char();
            token.len++;
        }
        // get last "
        if (idx < len) {
            chop_char();
            token.len++;
        }
        return token;
    }

    // all symbols
    if (is_symbol_start(text->get(idx))) {
        token.type = TokenType::SYMBOL;
        while (idx < len && is_symbol(text->get(idx))) {
            chop_char();
            token.len++;
        }

        // find keywords
        for (const auto &kwd : keywords) {
            u32 start_index = text->get_index_from_pointer(token.text);
            if (text->compare(start_index, kwd, strlen(kwd))) {
                token.type = TokenType::KEYWORD;
                return token;
            }
        }

        // find types
        for (const auto &type : builtin_types) {
            u32 start_index = text->get_index_from_pointer(token.text);
            if (text->compare(start_index, type, strlen(type))) {
                token.type = TokenType::TYPE;
                return token;
            }
        }
        return token;
    }
    // find numbers
    if (is_num_start(text->get(idx))) {
        token.type = TokenType::NUMBER;
        while (idx < len && is_num(text->get(idx))) {
            chop_char();
            token.len++;
        }
        return token;
    }

    // find comments
    if (starts_with("//", 2)) {
        token.type = TokenType::COMMENT;
        while (idx < len && text->get(idx) != '\n') {
            chop_char();
            token.len++;
        }
        return token;
    }

    token.type = TokenType::INVALID;
    token.len = 1;
    chop_char();

    return token;
}

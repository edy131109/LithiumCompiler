#pragma once

#include <string>
#include <vector>
#include <optional>
#include <iostream>

enum class TokenType {
    _exit,
    int_lit,
    semi,
    open_paren,
    close_paren,
    ident,
    let,
    eq,
    plus,
    star,
    minus,
    fslash,
    pluseq,
    stareq,
    minuseq,
    fslasheq,
    open_curly,
    close_curly,
    if_,
    else_
};

inline std::optional<int> bin_prec(const TokenType type) {
    switch (type) {
        case TokenType::plus:
        case TokenType::minus:
            return 0;
        case TokenType::fslash:
        case TokenType::star:
            return 1;
        default:
            return {};
    }
}

struct Token {
    TokenType type;
    int line;
    int col;
    std::optional<std::string> value {};
};

class Tokenizer {
public:
    explicit Tokenizer(std::string src, std::string srcName)
        : m_src(std::move(src)), m_srcName(srcName)
    {

    }

    std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        std::string buf;
        int line_count = 1;
        int col_count = 1;

        while (peek().has_value()) {
            if (std::isalpha(peek().value())) {
                buf.push_back(consume());
                while (peek().has_value() && std::isalnum(peek().value())) {
                    buf.push_back(consume());
                    col_count++;
                }
                if (buf == "exit") {
                    tokens.push_back({ .type = TokenType::_exit, .line = line_count, .col = col_count });
                    buf.clear();
                }
                else if (buf == "let") {
                    tokens.push_back({ .type = TokenType::let, .line = line_count, .col = col_count });
                    buf.clear();
                }
                else if (buf == "if") {
                    tokens.push_back({ .type = TokenType::if_, .line = line_count, .col = col_count });
                    buf.clear();
                }
                else if (buf == "else") {
                    tokens.push_back({ .type = TokenType::else_, .line = line_count, .col = col_count });
                    buf.clear();
                }
                else {
                    tokens.push_back({ .type = TokenType::ident, .line = line_count, .col = col_count, .value = buf });
                    buf.clear();
                }
            }
            else if (std::isdigit(peek().value())) {
                buf.push_back(consume());
                while (peek().has_value() && std::isdigit(peek().value())) {
                    buf.push_back(consume());
                }
                tokens.push_back({ .type = TokenType::int_lit, .line = line_count, .col = col_count, .value = buf });
                buf.clear();
            }
            else if (peek().value() == '/' && peek(1).has_value() && peek(1).value() == '/') {
                consume();
                consume();
                while(peek().has_value() && peek().value() != '\n') {
                    consume();
                }
            }
            else if (peek().value() == '/' && peek(1).has_value() && peek(1).value() == '*') {
                consume();
                consume();
                while(peek().has_value()) {
                    if (peek().value() == '*' && peek(1).has_value() && peek(1).value() == '/') {
                        break;
                    }
                    consume();
                }
                if (peek().has_value()) {
                    consume();
                }
                if (peek().has_value()) {
                    consume();
                }
            }
            else if (peek().value() == '(') {
                consume();
                tokens.push_back({ .type = TokenType::open_paren, .line = line_count, .col = col_count });
            }
            else if (peek().value() == ')') {
                consume();
                tokens.push_back({ .type = TokenType::close_paren, .line = line_count, .col = col_count });
            }
            else if (peek().value() == ';') {
                consume();
                tokens.push_back({ .type = TokenType::semi, .line = line_count, .col = col_count });
            }
            else if (peek().value() == '=') {
                consume();
                tokens.push_back({ .type = TokenType::eq, .line = line_count, .col = col_count });
            }
            else if (peek().value() == '+') {
                if (peek(1).has_value() && peek(1).value() == '=') {
                    consume();
                    consume();
                    tokens.push_back({ .type = TokenType::pluseq, .line = line_count, .col = col_count });
                    continue;
                }
                consume();
                tokens.push_back({ .type = TokenType::plus, .line = line_count, .col = col_count });
            }
            else if (peek().value() == '*') {
                if (peek(1).has_value() && peek(1).value() == '=') {
                    consume();
                    consume();
                    tokens.push_back({ .type = TokenType::stareq, .line = line_count, .col = col_count });
                    continue;
                }
                consume();
                tokens.push_back({ .type = TokenType::star, .line = line_count, .col = col_count });
            }
            else if (peek().value() == '-') {
                if (peek(1).has_value() && peek(1).value() == '=') {
                    consume();
                    consume();
                    tokens.push_back({ .type = TokenType::minuseq, .line = line_count, .col = col_count });
                    continue;
                }
                consume();
                tokens.push_back({ .type = TokenType::minus, .line = line_count, .col = col_count });
            }
            else if (peek().value() == '/') {
                if (peek(1).has_value() && peek(1).value() == '=') {
                    consume();
                    consume();
                    tokens.push_back({ .type = TokenType::fslasheq, .line = line_count, .col = col_count });
                    continue;
                }
                consume();
                tokens.push_back({ .type = TokenType::fslash, .line = line_count, .col = col_count });
            }
            else if (peek().value() == '{') {
                consume();
                tokens.push_back({ .type = TokenType::open_curly, .line = line_count, .col = col_count });
            }
            else if (peek().value() == '}') {
                consume();
                tokens.push_back({ .type = TokenType::close_curly, .line = line_count, .col = col_count });
            }
            else if (peek().value() == '\n') {
                consume();
                line_count++;
                col_count = 0;
            }
            else if (std::isspace(peek().value())) {
                consume();
            }
            else {
                std::cerr << m_srcName << ":" << line_count << ":" << col_count << ": lex_error: Unexpected character '" << consume() << "'" << std::endl;
                exit(EXIT_FAILURE);
            }
            col_count++;
        }
        m_index = 0;
        return tokens;
    }
private:

    [[nodiscard]] std::optional<char> peek(size_t offset = 0) const {
        if (m_index + offset >= m_src.length()) {
            return {};
        }
        return m_src.at(m_index + offset);
    }

    char consume() {
        return m_src.at(m_index++);
    }

    const std::string m_src;
    const std::string m_srcName;
    size_t m_index = 0;
};
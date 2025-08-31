#pragma once

#include <string>
#include <vector>
enum class TokenType {
    exit,
    int_lit,
    semi,
    open_paren,
    close_paren,
    ident,
    let,
    eq,
    plus

};
struct Token {
    TokenType type;
    std::optional<std::string> value;
};
class Tokenizer {
public:
    inline explicit Tokenizer(std::string& src) : m_src(std::move(src))
    {
    }

    inline std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        std::string buf;
        while (peak().has_value()) {
            if (std::isalpha(peak().value())) {
                buf.push_back(consume());
                while (peak().has_value() && std::isalnum(peak().value())) {
                    buf.push_back(consume());
                }
                if (buf == "exit") {
                    tokens.push_back({.type = TokenType::exit});
                    buf.clear();
                } else if (buf == "let") {
                    tokens.push_back({.type =  TokenType::let});
                    buf.clear();
                }
                else {
                    tokens.push_back({.type = TokenType::ident, .value = buf});
                    buf.clear();
                }
            }
            else if (std::isdigit(peak().value())) {
                buf.push_back(consume());
                while (peak().has_value() && std::isdigit(peak().value())) {
                    buf.push_back(consume());
                }
                tokens.push_back({.type = TokenType::int_lit, .value = buf});
                buf.clear();
            }
            else if (peak().value() == '(') {
                consume();
                tokens.push_back({.type = TokenType::open_paren});
            }
            else if (peak().value() == '=') {
                consume();
                tokens.push_back({.type = TokenType::eq});
            }
            else if (peak().value() == ')') {
                consume();
                tokens.push_back({.type = TokenType::close_paren});
            }
            else if (peak().value() == ';') {
                consume();
                tokens.push_back({.type = TokenType::semi});
                buf.clear();
            } else if (peak().value() == '+') {
                consume();
                tokens.push_back({.type = TokenType::plus});
                buf.clear();
            }

            else if (std::isspace(peak().value())) {
                consume();
            }
            else {
                std::cerr << "Messed up" << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        m_index =  0;
        return tokens;
    }
private:

    [[nodiscard]] inline std::optional<char> peak(int offset = 0) const {
        if (m_index + offset >= m_src.length()) {
            return {};
        }
        else {
            return m_src.at(m_index + offset);
        }
    }

    inline char consume() {
        return m_src.at(m_index++);
    }

    const std::string m_src;
    size_t m_index=0;
};
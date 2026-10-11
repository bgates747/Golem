#pragma once

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <utility>

namespace golem::syntax {
struct Error : std::runtime_error { using std::runtime_error::runtime_error; };
struct Position { size_t offset = 0, line = 1, column = 1; };
enum class Kind { End, Identifier, Number, String, Symbol };
struct Token {
    Kind kind = Kind::End;
    std::string text;
    Position begin, end; // Half-open source span; string text is decoded data.
};
inline std::string lower(std::string word) {
    for (char &c : word) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return word;
}
inline bool reserved(const std::string &word) {
    const auto name = lower(word);
    for (const char *keyword : {"print", "uint16", "for", "to", "next", "float32",
            "sint16", "range", "sub", "endsub", "floor", "str", "if", "then",
            "elseif", "else", "endif", "do", "while", "until", "loop", "switch",
            "case", "default", "endswitch"})
        if (name == keyword) return true;
    return false;
}
[[noreturn]] inline void fail(const Position &at, const std::string &message) {
    throw Error("line " + std::to_string(at.line) + ", column " +
                std::to_string(at.column) + ": " + message);
}
class Reader {
    std::string source_;
    Position cursor_;
    Token token_;
    static bool digit(char c) { return c >= '0' && c <= '9'; }
    static bool start(char c) { return std::isalpha(static_cast<unsigned char>(c)) || c == '_'; }
    static bool cont(char c) { return start(c) || digit(c); }
    char current() const { return cursor_.offset < source_.size() ? source_[cursor_.offset] : '\0'; }
    char following() const { return cursor_.offset + 1 < source_.size() ? source_[cursor_.offset + 1] : '\0'; }
    void advance() {
        if (current() == '\n') { ++cursor_.line; cursor_.column = 1; }
        else ++cursor_.column;
        ++cursor_.offset;
    }
    Token scan() {
        for (;;) {
            while (cursor_.offset < source_.size() && std::isspace(static_cast<unsigned char>(current()))) advance();
            if (current() != '/' || following() != '/') break;
            while (cursor_.offset < source_.size() && current() != '\n') advance();
        }
        Token token; token.begin = cursor_;
        if (cursor_.offset == source_.size()) { token.end = cursor_; return token; }
        char c = current();
        if (c == ';') fail(cursor_, "statement terminators are not supported; remove ';'");
        if (start(c)) {
            token.kind = Kind::Identifier;
            do { advance(); } while (cont(current()));
            token.text = source_.substr(token.begin.offset, cursor_.offset - token.begin.offset);
        } else if (digit(c) || (c == '.' && digit(following()))) {
            token.kind = Kind::Number;
            while (digit(current())) advance();
            if (current() == '.') { advance(); while (digit(current())) advance(); }
            if (current() == 'e' || current() == 'E') {
                advance();
                if (current() == '+' || current() == '-') advance();
                if (!digit(current())) fail(token.begin, "malformed numeric literal: expected exponent digits");
                while (digit(current())) advance();
            }
            if (cont(current()) || current() == '.') fail(token.begin, "malformed numeric literal");
            token.text = source_.substr(token.begin.offset, cursor_.offset - token.begin.offset);
        } else if (c == '"') {
            token.kind = Kind::String; advance();
            while (current() != '"') {
                if (cursor_.offset == source_.size()) fail(token.begin, "unterminated string literal");
                c = current();
                if (c == '\\' && (following() == '"' || following() == '\\' || following() == 'n')) {
                    advance(); c = current() == 'n' ? '\n' : current();
                }
                token.text.push_back(c); advance();
            }
            advance();
        } else {
            // Keep meaningful punctuation as individual tokens. Parsers reject
            // unsupported operators/constructs rather than guessing boundaries.
            token.kind = Kind::Symbol; token.text.push_back(c); advance();
        }
        token.end = cursor_; return token;
    }
public:
    explicit Reader(std::string source) : source_(std::move(source)) { token_ = scan(); }
    const Token &peek() const { return token_; }
    bool end() const { return token_.kind == Kind::End; }
    [[noreturn]] void error(const std::string &message) const { fail(token_.begin, message); }
    Token pop() { Token old = token_; token_ = scan(); return old; }
    bool keyword(const std::string &word) const {
        return token_.kind == Kind::Identifier && lower(token_.text) == lower(word);
    }
    bool takeKeyword(const std::string &word) {
        if (!keyword(word)) return false;
        pop(); return true;
    }
    void needKeyword(const std::string &word) {
        if (!takeKeyword(word)) error("expected '" + word + "'");
    }
    bool take(char symbol) {
        if (token_.kind != Kind::Symbol || token_.text != std::string(1, symbol)) return false;
        pop(); return true;
    }
    void need(char symbol) {
        if (!take(symbol)) error(std::string("expected '") + symbol + "'");
    }
    Token identifier() {
        if (token_.kind != Kind::Identifier) error("expected identifier");
        return pop();
    }
    Token name() {
        if (token_.kind != Kind::Identifier) error("expected identifier");
        if (reserved(token_.text)) error("reserved identifier '" + token_.text + "'");
        return pop();
    }
    unsigned uint16() {
        if (token_.kind != Kind::Number) error("expected uint16 integer literal");
        unsigned value = 0;
        for (char c : token_.text) {
            if (!digit(c)) error("expected unsigned decimal integer literal");
            const unsigned d = static_cast<unsigned>(c - '0');
            if (value > (65535u - d) / 10u) error("integer literal does not fit in a uint16 (0-65535)");
            value = value * 10u + d;
        }
        pop(); return value;
    }
    double number() {
        const auto at = token_.begin;
        bool negative = take('-');
        if (!negative) take('+');
        if (token_.kind != Kind::Number) error("expected finite decimal number");
        const auto text = token_.text;
        errno = 0;
        char *end = nullptr;
        double value = std::strtod(text.c_str(), &end);
        // strtod may set ERANGE for a finite subnormal too. Preserve those
        // values; reject an underflow that silently loses a nonzero literal.
        if (end != text.c_str() + text.size() || !std::isfinite(value) ||
                (errno == ERANGE && value == 0))
            fail(at, "decimal literal is outside the finite representable range");
        pop(); return negative ? -value : value;
    }
};
} // namespace golem::syntax

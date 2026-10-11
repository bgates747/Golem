#pragma once
#include "scalar_backend.hpp"
#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <sstream>

namespace readable {
inline std::string lower(std::string word) {
    for (char &c : word) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return word;
}
class Parser {
    const std::string &source;
    size_t pos = 0;
    [[noreturn]] void fail(const std::string &message) const {
        unsigned line = 1 + static_cast<unsigned>(std::count(source.begin(), source.begin() + pos, '\n'));
        throw scalar::Error("line " + std::to_string(line) + ": " + message);
    }
    void whitespace() {
        for (;;) {
            while (pos < source.size() && std::isspace(static_cast<unsigned char>(source[pos]))) ++pos;
            if (source.compare(pos, 2, "//") != 0) return;
            while (pos < source.size() && source[pos] != '\n') ++pos;
        }
    }
    static bool start(char c) { return std::isalpha(static_cast<unsigned char>(c)) || c == '_'; }
    static bool cont(char c) { return start(c) || std::isdigit(static_cast<unsigned char>(c)); }
    bool take(char c) {
        whitespace();
        if (pos == source.size() || source[pos] != c) return false;
        ++pos; return true;
    }
    void need(char c) {
        if (!take(c)) fail(std::string("expected '") + c + "'");
    }
    std::string identifier() {
        whitespace();
        if (pos == source.size() || !start(source[pos])) fail("expected identifier");
        size_t begin = pos++;
        while (pos < source.size() && cont(source[pos])) ++pos;
        return source.substr(begin, pos - begin);
    }
    void keyword(const std::string &word) {
        if (lower(identifier()) != lower(word)) fail("expected '" + word + "'");
    }
    std::string name() {
        auto value = identifier();
        const std::set<std::string> reserved = {
            "float32", "sint16", "range", "sub", "endsub", "floor", "print",
            "uint16", "for", "to", "next", "str"
        };
        if (reserved.count(lower(value))) fail("reserved identifier '" + value + "'");
        return value;
    }
    bool digit() const {
        return pos < source.size() && std::isdigit(static_cast<unsigned char>(source[pos]));
    }
    double number() {
        whitespace();
        size_t begin = pos;
        if (pos < source.size() && (source[pos] == '-' || source[pos] == '+')) ++pos;
        size_t digits = 0;
        while (digit()) { ++pos; ++digits; }
        if (pos < source.size() && source[pos] == '.') {
            ++pos;
            while (digit()) { ++pos; ++digits; }
        }
        if (!digits) fail("expected finite decimal number");
        if (pos < source.size() && (source[pos] == 'e' || source[pos] == 'E')) {
            ++pos;
            if (pos < source.size() && (source[pos] == '-' || source[pos] == '+')) ++pos;
            if (!digit()) fail("expected exponent digits");
            while (digit()) ++pos;
        }
        std::string text = source.substr(begin, pos - begin);
        double value = std::strtod(text.c_str(), nullptr);
        if (!std::isfinite(value)) fail("expected finite decimal number");
        return value;
    }
  public:
    explicit Parser(const std::string &text) : source(text) {}
    scalar::Spec run() {
        scalar::Spec spec;
        keyword("float32"); spec.input = name(); keyword("Range"); need('(');
        spec.range.lo = number(); need(','); spec.range.hi = number(); need(')');
        if (take('=')) spec.initial = number();
        need(';');
        keyword("sint16"); spec.output = name(); need(';');
        keyword("Sub"); spec.entry = name(); need(';');
        if (spec.input == spec.output || spec.input == spec.entry || spec.output == spec.entry)
            fail("duplicate declaration name");
        if (identifier() != spec.output) fail("assignment must target declared sint16 output");
        need('='); keyword("sint16"); need('('); keyword("floor"); need('(');
        if (identifier() != spec.input) fail("floor operand must be declared float32 input");
        need(')'); need(')'); need(';'); keyword("EndSub"); need(';');
        whitespace();
        if (pos != source.size()) fail("unexpected trailing source; this slice supports one job");
        return spec;
    }
    double updateValue() {
        double value = number(); whitespace();
        if (pos != source.size()) fail("unexpected text after input value");
        return value;
    }
};
inline scalar::Result compile(const std::string &source) {
    return scalar::compile(Parser(source).run());
}
inline std::string manifest(const scalar::Result &result) {
    std::ostringstream out;
    out << std::setprecision(17)
        << "{\n  \"schema\": 1,\n  \"experimental_task\": \"GOLEM-003\",\n"
        << "  \"input\": {\"name\": \"" << result.spec.input << "\", \"id\": "
        << result.layout.input << ", \"offset\": 0, \"type\": \"float32\", \"bytes\": 4, \"min\": "
        << result.spec.range.lo << ", \"max\": " << result.spec.range.hi << "},\n"
        << "  \"output\": {\"name\": \"" << result.spec.output << "\", \"id\": "
        << result.layout.output << ", \"offset\": 0, \"type\": \"sint16\", \"bytes\": 2},\n"
        << "  \"entry\": {\"name\": \"" << result.spec.entry << "\", \"id\": "
        << result.layout.entry << ", \"blocks\": 1, \"bytes\": "
        << result.programs.at(result.layout.entry).size() << "},\n"
        << "  \"external_input_requires_host_validation\": true,\n"
        << "  \"byte_order\": \"little-endian\",\n"
        << "  \"bootstrap_bytes\": " << result.bootstrap.size() << ",\n"
        << "  \"input_update_bytes\": 15,\n  \"call_bytes\": " << result.invoke.size() << ",\n"
        << "  \"cleanup_bytes\": " << result.cleanup.size() << "\n}\n";
    return out.str();
}
} // namespace readable

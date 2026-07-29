// golemc.cpp
//
// Toy Golem compiler (proof of concept).
//
// Supports exactly one statement form: `print "string literal";` (the
// trailing semicolon is optional). Compiles a .golem source file down to
// the *compiled program* format demonstrated by
// examples/hello_golem/program.asm: a flat binary blob containing only the
// raw payload bytes that get written into a VDP buffer and called - see
// that file's header comment for the rationale (printable text needs no
// escape interpretation to become "print this" when replayed via the
// Buffered Commands API's buffer-call mechanism, so codegen for a
// string-literal-only print degenerates to "emit the literal bytes").
//
// This is NOT the real Golem compiler - it has no lexer/parser generality,
// no type system, no expressions, nothing beyond this single statement
// form. It exists to close the loop first opened by examples/hello_golem/:
// that example's program.bin was hand-authored to *stand in* for compiler
// output; this program is a genuine (if minimal) compiler that produces
// the same kind of output for real, from real .golem source.
//
// Usage: golemc <input.golem> <output.bin>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct CompileError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

int lineNumberAt(const std::string &src, size_t pos) {
    return 1 + static_cast<int>(std::count(src.begin(), src.begin() + pos, '\n'));
}

// Advance pos past any whitespace and `//`-to-end-of-line comments. This
// toy grammar has no statement terminators other than the `print` keyword
// itself (plus an optional `;`), so newlines are not otherwise significant.
void skipWhitespaceAndComments(const std::string &src, size_t &pos) {
    for (;;) {
        while (pos < src.size() && std::isspace(static_cast<unsigned char>(src[pos]))) {
            ++pos;
        }
        if (pos + 1 < src.size() && src[pos] == '/' && src[pos + 1] == '/') {
            while (pos < src.size() && src[pos] != '\n') {
                ++pos;
            }
            continue;
        }
        break;
    }
}

// Parse a double-quoted string literal starting at src[pos] (which must be
// '"'). Supports \\, \", and \n escapes only. Advances pos past the
// closing quote and returns the literal's decoded bytes.
std::string parseStringLiteral(const std::string &src, size_t &pos) {
    int startLine = lineNumberAt(src, pos);
    if (pos >= src.size() || src[pos] != '"') {
        throw CompileError("line " + std::to_string(startLine) +
                            ": expected '\"' to start a string literal");
    }
    ++pos;
    std::string out;
    for (;;) {
        if (pos >= src.size()) {
            throw CompileError("line " + std::to_string(startLine) +
                                ": unterminated string literal");
        }
        char c = src[pos];
        if (c == '"') {
            ++pos;
            break;
        }
        if (c == '\\' && pos + 1 < src.size()) {
            char next = src[pos + 1];
            if (next == '"' || next == '\\') {
                out.push_back(next);
                pos += 2;
                continue;
            }
            if (next == 'n') {
                out.push_back('\n');
                pos += 2;
                continue;
            }
        }
        out.push_back(c);
        ++pos;
    }
    return out;
}

// Compile Golem source into the raw payload-byte "compiled output" this toy
// model uses (see program.asm's header comment): each `print "..."`
// statement's literal bytes are emitted verbatim, followed by CR LF
// (13, 10) - matching program.asm's `db "...",13,10` convention.
std::vector<unsigned char> compile(const std::string &src) {
    std::vector<unsigned char> out;
    size_t pos = 0;
    skipWhitespaceAndComments(src, pos);
    while (pos < src.size()) {
        int lineNo = lineNumberAt(src, pos);
        static const std::string kPrint = "print";
        if (src.compare(pos, kPrint.size(), kPrint) != 0) {
            throw CompileError("line " + std::to_string(lineNo) +
                                ": expected 'print' statement "
                                "(the only statement form this toy compiler supports)");
        }
        pos += kPrint.size();
        skipWhitespaceAndComments(src, pos);
        std::string text = parseStringLiteral(src, pos);
        for (unsigned char c : text) {
            out.push_back(c);
        }
        out.push_back(13);
        out.push_back(10);
        skipWhitespaceAndComments(src, pos);
        if (pos < src.size() && src[pos] == ';') {
            ++pos;
            skipWhitespaceAndComments(src, pos);
        }
    }
    return out;
}

std::string readFile(const std::string &path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw CompileError("could not open input file: " + path);
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

void writeFile(const std::string &path, const std::vector<unsigned char> &data) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw CompileError("could not open output file for writing: " + path);
    }
    out.write(reinterpret_cast<const char *>(data.data()),
              static_cast<std::streamsize>(data.size()));
}

} // namespace

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "usage: golemc <input.golem> <output.bin>\n";
        return 2;
    }
    try {
        std::string src = readFile(argv[1]);
        std::vector<unsigned char> compiled = compile(src);
        writeFile(argv[2], compiled);
        std::cout << "wrote " << compiled.size() << " bytes to " << argv[2] << "\n";
    } catch (const CompileError &e) {
        std::cerr << "golemc: error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

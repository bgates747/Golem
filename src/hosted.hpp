// Hosted extension of golemc. Included after the legacy compiler helpers so the
// two frontends share byte encoders, diagnostics and string/comment semantics.
// No runtime evaluation takes place here: intervals are safety analysis only.
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <limits>
#include <set>

namespace hosted {
using Bytes = std::vector<unsigned char>;
struct Arg {
    std::string text;
    bool number = false;
    double value = 0;
    bool quoted = false;
};
struct Node {
    std::string name, text;
    std::vector<Arg> args;
    std::vector<Node> body;
    int line = 1;
};
inline std::string lower(std::string s) {
    for (char &c : s)
        c = char(std::tolower(static_cast<unsigned char>(c)));
    return s;
}
[[noreturn]] inline void fail(int line, const std::string &s) {
    throw CompileError("line " + std::to_string(line) + ": " + s);
}
class Parser {
    const std::string &src;
    size_t p = 0;
    void ws() { skipWhitespaceAndComments(src, p); }
    bool take(char c) {
        ws();
        if (p < src.size() && src[p] == c) {
            ++p;
            return true;
        }
        return false;
    }
    void need(char c) {
        if (!take(c))
            fail(lineNumberAt(src, p), std::string("expected '") + c + "'");
    }
    std::string identifier() {
        ws();
        size_t start = p;
        if (p == src.size() || !(std::isalpha(static_cast<unsigned char>(src[p])) || src[p] == '_'))
            fail(lineNumberAt(src, p), "expected identifier");
        while (p < src.size() &&
               (std::isalnum(static_cast<unsigned char>(src[p])) || src[p] == '_'))
            ++p;
        return src.substr(start, p - start);
    }
    Arg argument() {
        ws();
        Arg a;
        if (p < src.size() && src[p] == '"') {
            a.text = parseStringLiteral(src, p);
            a.quoted = true;
            return a;
        }
        if (p < src.size() && (std::isdigit(static_cast<unsigned char>(src[p])) || src[p] == '-' ||
                               src[p] == '+' || src[p] == '.')) {
            size_t start = p;
            char *end = nullptr;
            a.value = std::strtod(src.c_str() + p, &end);
            p = size_t(end - src.c_str());
            if (p == start || !std::isfinite(a.value))
                fail(lineNumberAt(src, start), "expected finite numeric literal");
            a.number = true;
            a.text = src.substr(start, p - start);
            return a;
        }
        a.text = identifier();
        return a;
    }

  public:
    explicit Parser(const std::string &s) : src(s) {}
    std::vector<Node> block(bool nested = false, unsigned depth = 0) {
        if (depth > 32)
            fail(lineNumberAt(src, p), "hosted syntax nesting exceeds 32");
        std::vector<Node> out;
        for (;;) {
            ws();
            if (p == src.size()) {
                if (nested)
                    fail(lineNumberAt(src, p), "missing '}'");
                break;
            }
            if (src[p] == '}') {
                if (!nested)
                    fail(lineNumberAt(src, p), "unexpected '}'");
                ++p;
                break;
            }
            Node n;
            n.line = lineNumberAt(src, p);
            n.name = lower(identifier());
            if (n.name == "print") {
                ws();
                n.text = parseStringLiteral(src, p);
                need(';');
                out.push_back(n);
                continue;
            }
            if (n.name == "buffer" || n.name == "field" || n.name == "matrix" ||
                n.name == "program" || n.name == "asset" || n.name == "table")
                n.text = identifier();
            need('(');
            if (!take(')')) {
                do {
                    n.args.push_back(argument());
                } while (take(','));
                need(')');
            }
            if (n.name == "program" || n.name == "repeat") {
                need('{');
                n.body = block(true, depth + 1);
            }
            need(';');
            out.push_back(n);
        }
        return out;
    }
};
struct Range {
    double lo = -std::numeric_limits<double>::infinity(),
           hi = std::numeric_limits<double>::infinity();
};
inline Range exact(double n) { return {n, n}; }
inline Range rounded(double lo, double hi) {
    float l = float(lo), h = float(hi);
    if (double(l) > lo)
        l = std::nextafter(l, -std::numeric_limits<float>::infinity());
    if (double(h) < hi)
        h = std::nextafter(h, std::numeric_limits<float>::infinity());
    return {l, h};
}
inline Range plus(Range a, Range b) { return rounded(a.lo + b.lo, a.hi + b.hi); }
inline Range times(Range a, Range b) {
    if ((a.lo == 0 && a.hi == 0) || (b.lo == 0 && b.hi == 0))
        return exact(0);
    const double x[] = {a.lo * b.lo, a.lo * b.hi, a.hi * b.lo, a.hi * b.hi};
    for (double v : x)
        if (std::isnan(v))
            return {};
    return rounded(*std::min_element(x, x + 4), *std::max_element(x, x + 4));
}
struct Type {
    unsigned bytes, format;
    Range range;
    bool integer;
};
inline Type type(const std::string &s, int line) {
    if (s == "u8")
        return {1, 0, {0, 255}, true};
    if (s == "u16")
        return {2, 0, {0, 65535}, true};
    if (s == "s16")
        return {2, 0xc0, {-32768, 32767}, true};
    if (s == "s32")
        return {4, 0x40, {-2147483648.0, 2147483647.0}, true};
    if (s == "q8s16")
        return {2, 0xc8, {-128, 127.99609375}, false};
    if (s == "q8s32")
        return {4, 0x48, {-8388608, 8388607.99609375}, false};
    if (s == "f32")
        return {
            4,
            0,
            {-double(std::numeric_limits<float>::max()), double(std::numeric_limits<float>::max())},
            false};
    fail(line, "unknown field type '" + s + "'");
}
enum class Kind { Buffer, Matrix, Program, Field, Asset, Table };
struct Symbol {
    Kind kind = Kind::Buffer;
    unsigned id = 0, size = 0, offset = 0, rows = 0, cols = 0;
    unsigned records = 0;
    std::string typeName;
    Range range;
    Bytes data;
    std::vector<Range> values;
};
struct Result {
    Bytes code, clear;
    std::string map;
};
class Compiler {
    std::filesystem::path sourceDirectory;
    std::map<std::string, Symbol> syms;
    std::set<unsigned> ids;
    std::map<unsigned, std::set<unsigned>> edges;
    std::map<unsigned, Bytes> initial, programs;
    std::map<unsigned, std::vector<Range>> bounds, bootstrapBounds;
    std::set<unsigned> mutableMatrices;
    std::set<unsigned> finiteRepeat;
    std::map<unsigned, unsigned> comparisonConstants;
    std::map<std::pair<unsigned, unsigned>, unsigned> rejectionPrograms;
    Bytes setup;
    std::ostringstream listing;
    unsigned nextInternal = 2, currentProgram = 0;
    bool sharedWordScratch = false;
    unsigned wordTransform = 0, wordSource = 0, wordDestination = 0;
    int line = 1;
    void count(const Node &n, size_t size) {
        if (n.args.size() != size)
            fail(line, n.name + ": wrong argument count");
    }
    unsigned integer(const Arg &a, unsigned max) {
        if (!a.number || a.value < 0 || a.value > max || std::floor(a.value) != a.value)
            fail(line, "expected integer in 0.." + std::to_string(max));
        return unsigned(a.value);
    }
    Symbol &symbol(const Arg &a, Kind k) {
        auto i = syms.find(a.text);
        if (a.number || a.quoted || i == syms.end())
            fail(line, "unknown symbol '" + a.text + "'");
        if (i->second.kind != k)
            fail(line, "wrong symbol kind: " + a.text);
        return i->second;
    }
    Symbol &buffer(const Arg &a) {
        auto i = syms.find(a.text);
        if (a.number || a.quoted || i == syms.end() || i->second.kind != Kind::Buffer)
            fail(line, "expected data buffer: " + a.text);
        return i->second;
    }
    void claim(const std::string &name, Symbol s) {
        if (syms.count(name))
            fail(line, "duplicate symbol: " + name);
        if (s.kind != Kind::Field) {
            if (s.id < 1000 || s.id == 65535 || (s.id >= 30000 && s.id <= 30009) ||
                (s.id >= 63800 && s.id <= 64100) || !ids.insert(s.id).second)
                fail(line, "reserved or duplicate buffer ID");
        }
        syms.emplace(name, std::move(s));
    }
    unsigned scratch(unsigned size) {
        while (ids.count(nextInternal))
            ++nextInternal;
        if (nextInternal >= 1000)
            fail(line, "private buffer ID budget exceeded");
        unsigned id = nextInternal++;
        ids.insert(id);
        initial[id] = Bytes(size, 0);
        listing << "scratch " << id << " " << size << "\n";
        return id;
    }
    void span(const Symbol &f, unsigned n) {
        if (f.offset > f.size || n > f.size - f.offset)
            fail(line, "field span exceeds buffer");
    }
    const Symbol *matrixOwner(const Symbol &f) {
        for (const auto &kv : syms)
            if (kv.second.kind == Kind::Matrix && kv.second.id == f.id)
                return &kv.second;
        return nullptr;
    }
    void writable(const Symbol &f, unsigned bytes) {
        for (const auto &kv : syms)
            if (kv.second.kind == Kind::Asset && kv.second.id == f.id)
                fail(line, "immutable asset cannot be modified");
        if (matrixOwner(f))
            fail(line, "raw matrix mutation is not qualified; use matrix intrinsics");
        span(f, bytes);
        for (const auto &kv : syms) {
            const auto &other = kv.second;
            if (other.kind != Kind::Field || other.id != f.id)
                continue;
            auto width = type(other.typeName, line).bytes;
            if (other.offset < f.offset + bytes && f.offset < other.offset + width &&
                (other.offset != f.offset || other.typeName != f.typeName ||
                 other.range.lo != f.range.lo || other.range.hi != f.range.hi))
                fail(line, "write overlaps another typed field; use individually checked stores");
        }
    }
    static void command(Bytes &out, unsigned id, unsigned op) {
        emitBufferHeader(out, int(id));
        out.push_back(static_cast<unsigned char>(op));
    }
    void copy(Bytes &out, unsigned dst, unsigned off, unsigned src, unsigned from, unsigned size) {
        emitAdjustMultiBufferFetched(out, int(dst), 2, off, size, int(src), from);
        listing << "copy " << currentProgram << " " << dst << ":" << off << " " << src << ":"
                << from << " " << size << "\n";
    }
    void call(Bytes &out, unsigned id) {
        command(out, id, 1);
        edges[currentProgram].insert(id);
    }
    static void floatBytes(Bytes &out, double value) {
        float f = float(value);
        if (!std::isfinite(f))
            throw CompileError("float literal overflow");
        uint32_t bits;
        static_assert(sizeof(f) == 4, "requires binary32 float");
        std::memcpy(&bits, &f, 4);
        for (unsigned i = 0; i < 4; ++i)
            out.push_back(static_cast<unsigned char>(bits >> (8 * i)));
    }
    Bytes literal(const Symbol &s, const Arg &a) {
        if (!a.number || a.value < s.range.lo || a.value > s.range.hi)
            fail(line, "literal outside field bounds");
        auto t = type(s.typeName, line);
        Bytes out;
        if (s.typeName == "f32") {
            if (double(float(a.value)) < s.range.lo || double(float(a.value)) > s.range.hi)
                fail(line, "rounded float literal lies outside destination bounds");
            floatBytes(out, a.value);
            return out;
        }
        double value = a.value * ((s.typeName == "q8s16" || s.typeName == "q8s32") ? 256 : 1);
        if (std::floor(value) != value)
            fail(line, "literal not exactly representable in integer/fixed field");
        uint32_t bits = uint32_t(int64_t(value));
        for (unsigned i = 0; i < t.bytes; ++i)
            out.push_back(static_cast<unsigned char>(bits >> (8 * i)));
        return out;
    }
    // All affine arguments are fetched fields, including interned literals.
    Symbol operand(const Arg &a) {
        if (!a.number) {
            auto s = symbol(a, Kind::Field);
            if (s.typeName == "u8" || s.typeName == "u16")
                fail(line, "unsigned field requires explicit widening before float conversion");
            if (const auto *owner = matrixOwner(s))
                s.range = value(*owner).at(s.offset / 4);
            return s;
        }
        Symbol s;
        s.kind = Kind::Field;
        s.id = scratch(4);
        s.size = 4;
        s.typeName = "f32";
        s.range = exact(double(float(a.value)));
        initial[s.id].clear();
        floatBytes(initial[s.id], a.value);
        return s;
    }
    void fetched(Bytes &out, const Symbol &f) {
        out.push_back(static_cast<unsigned char>(type(f.typeName, line).format));
        emitLE16(out, f.id);
        emitLE16(out, f.offset);
    }
    void matrixHeader(Bytes &out, const Symbol &m, unsigned operation) {
        command(out, m.id, 34);
        out.push_back(static_cast<unsigned char>(operation));
        out.push_back(static_cast<unsigned char>(m.rows));
        out.push_back(static_cast<unsigned char>(m.cols));
    }
    std::vector<Range> value(const Symbol &s) {
        auto i = bounds.find(s.id);
        return i == bounds.end() ? std::vector<Range>(s.rows * s.cols) : i->second;
    }
    void shapes(const Symbol &a, const Symbol &b) {
        if (a.rows != b.rows || a.cols != b.cols)
            fail(line, "matrix shape mismatch");
    }
    void separate(const Symbol &dst, const Symbol &src) {
        if (dst.id == src.id)
            fail(line, "unqualified source/destination alias");
    }
    void identity(Bytes &out, const Symbol &m) {
        if (m.rows != m.cols || (m.rows != 3 && m.rows != 4))
            fail(line, "affine identity requires 3x3 or 4x4");
        command(out, m.id, m.rows == 3 ? 32 : 33);
        out.push_back(0);
        std::vector<Range> v(m.rows * m.cols, exact(0));
        for (unsigned i = 0; i < m.rows; ++i)
            v[i * m.cols + i] = exact(1);
        bounds[m.id] = v;
    }
    void affine(Bytes &out, const Symbol &m, unsigned op, const std::vector<Symbol> &args) {
        if (m.rows != 3 || m.cols != 3)
            fail(line, "this affine operation requires 3x3");
        command(out, m.id, 32);
        out.push_back(static_cast<unsigned char>(op | 0x60));
        for (const auto &f : args)
            fetched(out, f);
    }
    std::vector<Range> product3(const std::vector<Range> &a, const std::vector<Range> &b) {
        std::vector<Range> v(9, exact(0));
        for (unsigned i = 0; i < 3; ++i)
            for (unsigned j = 0; j < 3; ++j)
                for (unsigned k = 0; k < 3; ++k)
                    v[i * 3 + j] = plus(v[i * 3 + j], times(a[i * 3 + k], b[k * 3 + j]));
        return v;
    }
    void declare(const Node &n) {
        line = n.line;
        Symbol s;
        if (n.name == "asset" || n.name == "table") {
            count(n, n.name == "asset" ? 3 : 4);
            s.id = integer(n.args[0], 65534);
            const Arg &path = n.args[n.name == "asset" ? 1 : 3];
            if (!path.quoted || path.text.empty() || path.text.find('\0') != std::string::npos)
                fail(line, "asset needs a quoted relative path");
            std::filesystem::path relative(path.text);
            if (relative.is_absolute())
                fail(line, "asset path must be relative to source");
            for (const auto &part : relative)
                if (part == "..")
                    fail(line, "asset parent traversal is not allowed");
            s.size = integer(n.args[n.name == "asset" ? 2 : 1], 65535);
            s.records = n.name == "asset" ? 1 : integer(n.args[2], 1024);
            if (!s.size || !s.records || uint64_t(s.size) * s.records > 512 * 1024)
                fail(line, "invalid asset size/count or table budget");
            if (n.name == "table" && (s.id < 4000 || s.id + s.records > 5000))
                fail(line, "table record IDs must fit reserved 4000..4999 range");
            std::ifstream file(sourceDirectory / relative, std::ios::binary | std::ios::ate);
            if (!file || file.tellg() != std::streamoff(s.size * s.records))
                fail(line, "missing asset or exact size mismatch: " + path.text);
            file.seekg(0);
            s.data.resize(s.size * s.records);
            if (!file.read(reinterpret_cast<char *>(s.data.data()), s.data.size()))
                fail(line, "asset read failed");
            s.kind = n.name == "asset" ? Kind::Asset : Kind::Table;
            claim(n.text, s);
            for (unsigned i = 0; i < s.records; ++i) {
                if (i && !ids.insert(s.id + i).second)
                    fail(line, "table record ID collision");
                initial[s.id + i] =
                    Bytes(s.data.begin() + i * s.size, s.data.begin() + (i + 1) * s.size);
            }
            uint64_t hash = UINT64_C(14695981039346656037);
            for (auto byte : s.data)
                hash = (hash ^ byte) * UINT64_C(1099511628211);
            listing << "import " << n.text << " " << path.text << " " << s.data.size()
                    << " fnv1a64 " << std::hex << std::setw(16) << std::setfill('0') << hash
                    << std::dec << " records " << s.records << "\n";
            return;
        }
        if (n.name == "buffer" || n.name == "matrix" || n.name == "program") {
            count(n, n.name == "matrix" ? 3 : n.name == "buffer" ? 2 : 1);
            s.id = integer(n.args[0], 65534);
            if (n.name == "matrix") {
                s.kind = Kind::Matrix;
                s.rows = integer(n.args[1], 8);
                s.cols = integer(n.args[2], 8);
                if (!s.rows || !s.cols)
                    fail(line, "zero matrix dimension");
                s.size = s.rows * s.cols * 4;
            } else if (n.name == "buffer") {
                s.size = integer(n.args[1], 65535);
                if (!s.size)
                    fail(line, "zero buffer size");
            } else
                s.kind = Kind::Program;
            claim(n.text, s);
            if (s.kind != Kind::Program)
                initial[s.id] = Bytes(s.size, 0);
        }
    }
    void field(const Node &n) {
        line = n.line;
        if (n.args.size() != 3 && n.args.size() != 5)
            fail(line, "Field requires 3 or 5 arguments");
        auto i = syms.find(n.args[0].text);
        if (n.args[0].number || n.args[0].quoted || i == syms.end() ||
            (i->second.kind != Kind::Buffer && i->second.kind != Kind::Matrix &&
             i->second.kind != Kind::Asset))
            fail(line, "Field owner must be buffer or matrix");
        Symbol s;
        s.id = i->second.id;
        s.size = i->second.size;
        s.rows = i->second.rows;
        s.cols = i->second.cols;
        s.kind = Kind::Field;
        s.offset = integer(n.args[1], 65535);
        if (n.args[2].number || n.args[2].quoted)
            fail(line, "field type must be an identifier");
        s.typeName = lower(n.args[2].text);
        auto t = type(s.typeName, line);
        s.range = t.range;
        if (i->second.kind == Kind::Matrix && (s.typeName != "f32" || s.offset % 4))
            fail(line, "matrix field must be aligned f32");
        if (n.args.size() == 5) {
            if (!n.args[3].number || !n.args[4].number)
                fail(line, "field bounds must be numeric");
            s.range = {n.args[3].value, n.args[4].value};
            if (s.range.lo < t.range.lo || s.range.hi > t.range.hi || s.range.lo > s.range.hi)
                fail(line, "invalid field bounds");
        }
        span(s, t.bytes);
        claim(n.text, s);
        listing << "field " << n.text << " " << s.id << ":" << s.offset << " " << s.typeName << " "
                << s.range.lo << " " << s.range.hi << "\n";
    }
    double decoded(const Bytes &data, unsigned offset, const std::string &name) {
        auto t = type(name, line);
        uint32_t raw = 0;
        for (unsigned i = 0; i < t.bytes; ++i)
            raw |= uint32_t(data.at(offset + i)) << (8 * i);
        if (name == "f32") {
            float value;
            std::memcpy(&value, &raw, 4);
            return value;
        }
        if (name == "u8" || name == "u16")
            return raw;
        int64_t value = raw;
        if (raw & (UINT32_C(1) << (t.bytes * 8 - 1)))
            value -= INT64_C(1) << (t.bytes * 8);
        return double(value) / ((name == "q8s16" || name == "q8s32") ? 256 : 1);
    }
    void validateFields(const Symbol &dst, const Bytes &data, unsigned offset) {
        for (const auto &kv : syms) {
            const auto &f = kv.second;
            if (f.kind != Kind::Field || f.id != dst.id)
                continue;
            double v = decoded(data, offset + f.offset, f.typeName);
            if (!std::isfinite(v) || v < f.range.lo || v > f.range.hi)
                fail(line, "imported data violates typed field: " + kv.first);
        }
    }
    void markMutable(const std::vector<Node> &nodes) {
        for (const auto &n : nodes) {
            if (!n.args.empty() &&
                (n.name.rfind("mat", 0) == 0 || n.name == "identity" || n.name == "translate" ||
                 n.name == "scale" || n.name == "rotatedegrees" || n.name == "invertaffine")) {
                auto it = syms.find(n.args[0].text);
                if (it != syms.end() && it->second.kind == Kind::Matrix)
                    mutableMatrices.insert(it->second.id);
            }
            markMutable(n.body);
        }
    }
    unsigned comparisonConstant(unsigned value) {
        auto found = comparisonConstants.find(value);
        if (found != comparisonConstants.end())
            return found->second;
        unsigned id = scratch(2);
        initial[id] = le16Bytes(value);
        comparisonConstants[value] = id;
        return id;
    }
    unsigned rejectProgram(const Symbol &status) {
        if (status.typeName != "u16" || status.range.lo != 0 || status.range.hi != 1)
            fail(line, "validation status must be 0..1 u16");
        writable(status, 2);
        auto key = std::make_pair(status.id, status.offset);
        auto found = rejectionPrograms.find(key);
        if (found != rejectionPrograms.end())
            return found->second;
        unsigned id = scratch(0);
        initial.erase(id);
        emitAdjustMultiLiteral(programs[id], int(status.id), 2, status.offset, {0, 0});
        rejectionPrograms[key] = id;
        return id;
    }
    void condition(Bytes &out, unsigned caller, unsigned target, unsigned op, unsigned left,
                   unsigned leftOffset, unsigned right, unsigned rightOffset) {
        emitCondCallBufferValue(out, int(target), static_cast<unsigned char>(op), int(left),
                                leftOffset, int(right), rightOffset);
        edges[caller].insert(target);
    }
    void requireRange(Bytes &out, const Symbol &field, Range range, const Symbol &status) {
        auto t = type(field.typeName, line);
        if (field.typeName == "f32" || field.id == status.id)
            fail(line, "range validation requires fixed storage separate from status");
        if (!std::isfinite(range.lo) || !std::isfinite(range.hi) || range.lo < t.range.lo ||
            range.hi > t.range.hi || range.lo > range.hi)
            fail(line, "invalid runtime range");
        unsigned reject = rejectProgram(status);
        const bool fixed = field.typeName == "q8s16" || field.typeName == "q8s32";
        const bool signedType = field.typeName != "u8" && field.typeName != "u16";
        int64_t lo = int64_t(std::ceil(range.lo * (fixed ? 256 : 1)));
        int64_t hi = int64_t(std::floor(range.hi * (fixed ? 256 : 1)));
        if (lo > hi)
            fail(line, "runtime range contains no representable values");
        // Compare ordered raw bytes. Each byte is zero-extended to a u16 word,
        // so even 0xff bytes cannot collide with firmware's 0xffff sentinel.
        unsigned bytes = scratch(t.bytes * 2);
        for (unsigned i = 0; i < t.bytes; ++i)
            copy(out, bytes, i * 2, field.id, field.offset + i, 1);
        if (signedType)
            emitAdjustMultiLiteral(out, int(bytes), 7, (t.bytes - 1) * 2, {0x80});
        uint32_t sign = signedType ? UINT32_C(1) << (t.bytes * 8 - 1) : 0;
        const uint32_t limits[] = {uint32_t(lo) ^ sign, uint32_t(hi) ^ sign};
        for (unsigned side = 0; side < 2; ++side) {
            unsigned next = 0;
            for (unsigned i = 0; i < t.bytes; ++i) {
                unsigned stage = scratch(0);
                initial.erase(stage);
                unsigned constant = comparisonConstant((limits[side] >> (i * 8)) & 255);
                condition(programs[stage], stage, reject, side ? 5 : 4, bytes, i * 2, constant, 0);
                if (next)
                    condition(programs[stage], stage, next, 2, bytes, i * 2, constant, 0);
                next = stage;
            }
            call(out, next);
        }
        listing << "require_range " << currentProgram << " " << field.id << ":" << field.offset
                << " " << field.typeName << " " << range.lo << " " << range.hi << " sticky_status "
                << status.id << ":" << status.offset << "\n";
    }
    std::vector<Symbol> recordLayout(const Symbol &owner) {
        std::vector<Symbol> fields;
        for (const auto &kv : syms)
            if (kv.second.kind == Kind::Field && kv.second.id == owner.id)
                fields.push_back(kv.second);
        std::sort(fields.begin(), fields.end(),
                  [](const Symbol &a, const Symbol &b) { return a.offset < b.offset; });
        unsigned end = 0;
        for (const auto &field : fields) {
            if (field.offset != end)
                fail(line, "record fields must cover storage without gaps/aliases");
            end += type(field.typeName, line).bytes;
        }
        if (end != owner.size)
            fail(line, "record fields must cover complete storage");
        return fields;
    }
    void storeWordFloor(Bytes &out, const Symbol &f, const Symbol &m, bool unsignedStore) {
        auto range = value(m)[0];
        if (range.lo < (unsignedStore ? 0 : -32768) || range.hi > (unsignedStore ? 65534 : 32767) ||
            !std::isfinite(range.lo) || !std::isfinite(range.hi))
            fail(line, "signed store range not proved within -32768..32767");
        writable(f, 2);
        if (std::floor(range.lo) < f.range.lo ||
            (unsignedStore ? std::floor(range.hi) : std::ceil(range.hi)) > f.range.hi)
            fail(line, "signed result exceeds narrowed destination bounds");
        Symbol transform;
        transform.kind = Kind::Matrix;
        if (sharedWordScratch && !wordTransform) {
            wordTransform = scratch(36);
            wordSource = scratch(4);
            wordDestination = scratch(4);
        }
        transform.id = sharedWordScratch ? wordTransform : scratch(36);
        transform.rows = transform.cols = 3;
        transform.size = 36;
        Symbol scalar;
        scalar.kind = Kind::Field;
        scalar.id = m.id;
        scalar.size = 4;
        scalar.typeName = "f32";
        scalar.range = range;
        Arg zero;
        zero.number = true;
        Arg bias = zero;
        bias.value = 32768;
        identity(out, transform);
        affine(out, transform, 6, {scalar, operand(zero)});
        if (!unsignedStore)
            affine(out, transform, 6, {operand(bias), operand(zero)});
        unsigned source = sharedWordScratch ? wordSource : scratch(4),
                 dest = sharedWordScratch ? wordDestination : scratch(4);
        command(out, dest, 41);
        out.push_back(8);
        out.push_back(0xc0);
        emitLE16(out, transform.id);
        emitLE16(out, source);
        emitLE16(out, 1);
        // XOR the sign-bias bit in the temporary result; preserve neighbours.
        if (!unsignedStore) {
            command(out, dest, 5);
            out.push_back(7);
            emitLE16(out, 1);
            out.push_back(128);
        }
        copy(out, f.id, f.offset, dest, 0, 2);
    }
    void storePositive32Floor(Bytes &out, const Symbol &f, const Symbol &m) {
        auto range = value(m)[0];
        if (range.lo < 0 || range.hi > 16777215 || !std::isfinite(range.lo) ||
            !std::isfinite(range.hi))
            fail(line, "positive32 store requires finite 0..16777215 input");
        writable(f, 4);
        if (std::floor(range.lo) < f.range.lo || std::floor(range.hi) > f.range.hi)
            fail(line, "positive32 result exceeds destination bounds");
        Symbol transform;
        transform.kind = Kind::Matrix;
        transform.id = scratch(36);
        transform.rows = transform.cols = 3;
        transform.size = 36;
        Symbol scalar;
        scalar.kind = Kind::Field;
        scalar.id = m.id;
        scalar.size = 4;
        scalar.typeName = "f32";
        scalar.range = range;
        Arg zero;
        zero.number = true;
        identity(out, transform);
        affine(out, transform, 6, {scalar, operand(zero)});
        unsigned source = scratch(8), dest = scratch(8);
        command(out, dest, 41);
        out.push_back(8);
        out.push_back(0x40);
        emitLE16(out, transform.id);
        emitLE16(out, source);
        emitLE16(out, 1);
        // Only nonnegative magnitudes reach stock's unsigned conversion. The
        // result is below 2^24, hence also a representable positive signed32.
        copy(out, f.id, f.offset, dest, 0, 4);
    }
    Symbol privateScalar() {
        Symbol m;
        m.kind = Kind::Matrix;
        m.id = scratch(4);
        m.size = 4;
        m.rows = m.cols = 1;
        return m;
    }
    void loadScalar(Bytes &out, const Symbol &m, const Symbol &f) {
        matrixHeader(out, m, 0x20);
        out.push_back(static_cast<unsigned char>(type(f.typeName, line).format));
        emitLE16(out, f.id);
        emitLE16(out, f.offset);
        bounds[m.id] = {f.range};
    }
    void scaleScalar(Bytes &out, const Symbol &dst, const Symbol &src, double factor) {
        matrixHeader(out, dst, 0x27);
        emitLE16(out, src.id);
        Arg a;
        a.number = true;
        a.value = factor;
        fetched(out, operand(a));
        bounds[dst.id] = {times(value(src)[0], exact(double(float(factor))))};
    }
    void statements(const std::vector<Node> &nodes, Bytes &out) {
        for (const auto &n : nodes)
            statement(n, out);
    }
    void statement(const Node &n, Bytes &out) {
        line = n.line;
        const auto &a = n.args;
        const auto &op = n.name;
        if (op == "lessthan") {
            count(n, 3);
            auto &dst = symbol(a[0], Kind::Field), &left = symbol(a[1], Kind::Field),
                 &right = symbol(a[2], Kind::Field);
            if (dst.typeName != "u16" || dst.range.lo != 0 || dst.range.hi != 1 ||
                left.typeName != right.typeName || left.typeName == "f32")
                fail(line, "LessThan needs bool u16 destination and matching fixed input types");
            writable(dst, 2);
            unsigned width = type(left.typeName, line).bytes;
            auto overlaps = [&](const Symbol &src) {
                return dst.id == src.id && dst.offset < src.offset + width &&
                       src.offset < dst.offset + 2;
            };
            if (overlaps(left) || overlaps(right))
                fail(line, "LessThan destination aliases an operand");
            unsigned ordered = scratch(width * 4), accept = scratch(0);
            initial.erase(accept);
            emitAdjustMultiLiteral(out, int(dst.id), 2, dst.offset, {0, 0});
            emitAdjustMultiLiteral(programs[accept], int(dst.id), 2, dst.offset, {1, 0});
            for (unsigned i = 0; i < width; ++i) {
                copy(out, ordered, i * 2, left.id, left.offset + i, 1);
                copy(out, ordered, (width + i) * 2, right.id, right.offset + i, 1);
            }
            if (left.typeName != "u8" && left.typeName != "u16") {
                emitAdjustMultiLiteral(out, int(ordered), 7, (width - 1) * 2, {128});
                emitAdjustMultiLiteral(out, int(ordered), 7, (width * 2 - 1) * 2, {128});
            }
            unsigned next = 0;
            for (unsigned i = 0; i < width; ++i) {
                unsigned stage = scratch(0);
                initial.erase(stage);
                condition(programs[stage], stage, accept, 4, ordered, i * 2,
                          ordered, (width + i) * 2);
                if (next)
                    condition(programs[stage], stage, next, 2, ordered, i * 2,
                              ordered, (width + i) * 2);
                next = stage;
            }
            call(out, next);
            listing << "less_than " << currentProgram << " " << left.typeName << " "
                    << left.id << ":" << left.offset << " " << right.id << ":" << right.offset
                    << " result " << dst.id << ":" << dst.offset << " ordered_bytes " << width
                    << "\n";
            return;
        }
        if (op == "requirerange") {
            if (a.size() != 2 && a.size() != 4)
                fail(line, "RequireRange needs field, [low, high,] sticky status");
            auto &f = symbol(a[0], Kind::Field);
            Range range = f.range;
            if (a.size() == 4) {
                if (!a[1].number || !a[2].number)
                    fail(line, "runtime range bounds must be numeric");
                range = {a[1].value, a[2].value};
            }
            requireRange(out, f, range, symbol(a.back(), Kind::Field));
            return;
        }
        if (op == "requireequal") {
            count(n, 3);
            auto &l = symbol(a[0], Kind::Field);
            auto &r = symbol(a[1], Kind::Field);
            auto &status = symbol(a[2], Kind::Field);
            if (l.typeName != r.typeName || l.typeName == "f32" || l.id == status.id ||
                r.id == status.id)
                fail(line, "RequireEqual needs matching fixed types separate from status");
            unsigned reject = rejectProgram(status), left = scratch(2), right = scratch(2);
            for (unsigned i = 0; i < type(l.typeName, line).bytes; ++i) {
                copy(out, left, 0, l.id, l.offset + i, 1);
                copy(out, right, 0, r.id, r.offset + i, 1);
                condition(out, currentProgram, reject, 3, left, 0, right, 0);
            }
            listing << "require_equal " << currentProgram << " " << l.id << ":" << l.offset << " "
                    << r.id << ":" << r.offset << " sticky_status " << status.id << ":"
                    << status.offset << "\n";
            return;
        }
        if (op == "copychecked") {
            count(n, 3);
            auto &dst = symbol(a[0], Kind::Field), &src = symbol(a[1], Kind::Field),
                 &status = symbol(a[2], Kind::Field);
            if (dst.typeName != src.typeName || dst.typeName == "f32" ||
                status.id == dst.id || status.id == src.id)
                fail(line, "CopyChecked needs matching fixed types and separate status storage");
            unsigned width = type(dst.typeName, line).bytes;
            writable(dst, width);
            if (dst.id == src.id && dst.offset < src.offset + width && src.offset < dst.offset + width)
                fail(line, "CopyChecked storage overlaps");
            // Validate the fresh status before emitting its reset. A failed
            // range check preserves the complete old destination value.
            (void)rejectProgram(status);
            emitAdjustMultiLiteral(out, int(status.id), 2, status.offset, {1, 0});
            requireRange(out, src, dst.range, status);
            unsigned accepted = scratch(0);
            initial.erase(accepted);
            copy(programs[accepted], dst.id, dst.offset, src.id, src.offset, width);
            condition(out, currentProgram, accepted, 2, status.id, status.offset,
                      comparisonConstant(1), 0);
            listing << "copy_checked " << currentProgram << " " << src.id << ":" << src.offset
                    << " to " << dst.id << ":" << dst.offset << " " << dst.typeName << " "
                    << dst.range.lo << " " << dst.range.hi << " status " << status.id << ":"
                    << status.offset << "\n";
            return;
        }
        if (op == "copyrecord") {
            count(n, 2);
            auto &dst = buffer(a[0]);
            auto &src = buffer(a[1]);
            if (dst.id == src.id || dst.size != src.size)
                fail(line, "record size/alias mismatch");
            auto df = recordLayout(dst), sf = recordLayout(src);
            if (df.size() != sf.size())
                fail(line, "record layouts differ");
            for (unsigned i = 0; i < df.size(); ++i) {
                writable(df[i], type(df[i].typeName, line).bytes);
                if (df[i].offset != sf[i].offset || df[i].typeName != sf[i].typeName ||
                    sf[i].range.lo < df[i].range.lo || sf[i].range.hi > df[i].range.hi)
                    fail(line, "record copy does not preserve field layout/ranges");
            }
            copy(out, dst.id, 0, src.id, 0, dst.size);
            return;
        }
        if (op == "widenunsigned") {
            count(n, 2);
            auto &dst = symbol(a[0], Kind::Field);
            auto &src = symbol(a[1], Kind::Field);
            if ((src.typeName != "u8" && src.typeName != "u16") ||
                (dst.typeName != "s16" && dst.typeName != "s32" &&
                 !(dst.typeName == "u16" && src.typeName == "u8")) ||
                src.range.lo < dst.range.lo || src.range.hi > dst.range.hi)
                fail(line, "unsigned widening requires a containing integer range");
            unsigned dw = type(dst.typeName, line).bytes, sw = type(src.typeName, line).bytes;
            writable(dst, dw);
            if (dst.id == src.id && dst.offset < src.offset + sw && src.offset < dst.offset + dw)
                fail(line, "widening storage overlaps");
            emitAdjustMultiLiteral(out, int(dst.id), 2, dst.offset, Bytes(dw, 0));
            copy(out, dst.id, dst.offset, src.id, src.offset, sw);
            return;
        }
        if (op == "loadelement") {
            count(n, 4);
            auto &dst = buffer(a[0]);
            auto it = syms.find(a[1].text);
            auto &src = it != syms.end() && it->second.kind == Kind::Asset
                            ? symbol(a[1], Kind::Asset)
                            : buffer(a[1]);
            auto &index = symbol(a[2], Kind::Field);
            auto &status = symbol(a[3], Kind::Field);
            if (dst.id == src.id || !dst.size || src.size % dst.size || index.typeName != "u16" ||
                index.id == dst.id || status.id == dst.id || status.id == src.id ||
                status.id == index.id)
                fail(line,
                     "element lookup requires separate compatible record/index/status storage");
            unsigned records = src.size / dst.size;
            if (!records)
                fail(line, "empty element array");
            if (status.typeName != "u16" || status.range.lo != 0 || status.range.hi != 1)
                fail(line, "element lookup status must be 0..1 u16");
            writable(status, 2);
            auto df = recordLayout(dst), sf = recordLayout(src);
            if (sf.size() != df.size() * records)
                fail(line, "element source layout is not repeated records");
            for (unsigned r = 0; r < records; ++r)
                for (unsigned i = 0; i < df.size(); ++i) {
                    const auto &f = sf[r * df.size() + i];
                    if (f.offset != r * dst.size + df[i].offset || f.typeName != df[i].typeName ||
                        f.range.lo < df[i].range.lo || f.range.hi > df[i].range.hi)
                        fail(line, "element copy does not preserve destination layout/range");
                }
            for (const auto &f : df)
                writable(f, type(f.typeName, line).bytes);
            emitAdjustMultiLiteral(out, int(status.id), 2, status.offset, {0, 0});
            unsigned address = scratch(3), limit = comparisonConstant(records - 1),
                     instruction = scratch(15), body = scratch(0);
            initial.erase(body);
            Bytes code;
            emitAdjustMultiLiteral(code, int(address), 2, 0, {0, 0, 0});
            unsigned parent = currentProgram;
            currentProgram = body;
            copy(code, address, 0, index.id, index.offset, 2);
            unsigned high = 1;
            while (high <= dst.size / 2)
                high *= 2;
            // Exact bounded integer multiplication by the record stride. The
            // guard guarantees every intermediate offset fits the copied word.
            for (unsigned bit = high / 2; bit; bit /= 2) {
                emitAdjustMultiBufferFetched(code, int(address), 4, 0, 2, int(address), 0);
                if (dst.size & bit)
                    emitAdjustMultiBufferFetched(code, int(address), 4, 0, 2, int(index.id),
                                                 index.offset);
            }
            initial[instruction].clear();
            emitAdjustMultiBufferFetched(initial[instruction], int(dst.id), 2, 0, dst.size,
                                         int(src.id), 0);
            copy(code, instruction, 13, address, 0, 2);
            call(code, instruction);
            emitAdjustMultiLiteral(code, int(status.id), 2, status.offset, {1, 0});
            programs[body] = code;
            currentProgram = parent;
            condition(out, parent, body, 6, index.id, index.offset, limit, 0);
            listing << "element_lookup " << parent << " source " << src.id << " count " << records
                    << " stride " << dst.size << " instruction " << instruction
                    << " source_offset_field 13 guarded_body " << body << "\n";
            return;
        }
        if (op == "loadindexed") {
            if (a.size() != 3 && a.size() != 4)
                fail(line, "LoadIndexed needs destination, table, index [, status]");
            auto &dst = buffer(a[0]);
            auto &table = symbol(a[1], Kind::Table);
            auto &index = symbol(a[2], Kind::Field);
            if (dst.size != table.size || index.typeName != "u16")
                fail(line, "indexed record size/type mismatch");
            if (index.id == dst.id)
                fail(line, "index storage cannot alias selected record");
            Symbol status;
            if (a.size() == 4) {
                status = symbol(a[3], Kind::Field);
                if (status.typeName != "u16" || status.range.lo != 0 || status.range.hi != 1 ||
                    status.id == dst.id ||
                    (status.id == index.id && status.offset < index.offset + 2 &&
                     index.offset < status.offset + 2))
                    fail(line, "lookup status needs disjoint 0..1 u16 field");
                writable(status, 2);
                emitAdjustMultiLiteral(out, int(status.id), 2, status.offset, {0, 0});
            } else if (index.range.lo < 0 || index.range.hi >= table.records)
                fail(line, "unbounded lookup requires explicit status field");
            for (unsigned r = 0; r < table.records; ++r)
                validateFields(dst, table.data, r * table.size);
            unsigned address = scratch(3), limit = scratch(2), instruction = scratch(15),
                     body = scratch(0);
            initial[limit] = le16Bytes(table.records);
            initial.erase(body);
            Bytes copyCommand;
            emitAdjustMultiBufferFetched(copyCommand, int(dst.id), 2, 0, table.size, int(table.id),
                                         0);
            initial[instruction] = copyCommand;
            unsigned parent = currentProgram;
            currentProgram = body;
            Bytes code;
            emitAdjustMultiLiteral(code, int(address), 2, 0,
                                   {static_cast<unsigned char>(table.id),
                                    static_cast<unsigned char>(table.id >> 8), 0});
            emitAdjustMultiBufferFetched(code, int(address), 4, 0, 2, int(index.id), index.offset);
            copy(code, instruction, 11, address, 0, 2);
            call(code, instruction);
            if (a.size() == 4)
                emitAdjustMultiLiteral(code, int(status.id), 2, status.offset, {1, 0});
            programs[body] = code;
            currentProgram = parent;
            emitCondCallBufferValue(out, int(body), 4, int(index.id), index.offset, int(limit), 0);
            edges[parent].insert(body);
            listing << "lookup " << parent << " table " << table.id << " count " << table.records
                    << " stride " << table.size << " instruction " << instruction
                    << " source_id_offset 11 guarded_body " << body << "\n";
            return;
        }
        if (op == "print") {
            out.insert(out.end(), n.text.begin(), n.text.end());
            return;
        }
        if (op == "bytes") {
            if (currentProgram || a.empty())
                fail(line, "Bytes is bootstrap-only");
            auto &b = buffer(a[0]);
            if (a.size() != b.size + 1)
                fail(line, "Bytes must fill the entire declared buffer");
            Bytes data;
            for (size_t i = 1; i < a.size(); ++i)
                data.push_back(static_cast<unsigned char>(integer(a[i], 255)));
            initial[b.id] = data;
            return;
        }
        if (op == "set") {
            count(n, 2);
            auto &f = symbol(a[0], Kind::Field);
            writable(f, type(f.typeName, line).bytes);
            auto data = literal(f, a[1]);
            emitAdjustMultiLiteral(out, int(f.id), 2, f.offset, data);
            return;
        }
        if (op == "copy") {
            count(n, 3);
            auto &d = symbol(a[0], Kind::Field);
            auto s = symbol(a[1], Kind::Field);
            if (const auto *owner = matrixOwner(s))
                s.range = value(*owner).at(s.offset / 4);
            unsigned size = integer(a[2], 65535);
            if (!size)
                fail(line, "zero copy size");
            span(d, size);
            span(s, size);
            writable(d, size);
            if (size != type(d.typeName, line).bytes || size != type(s.typeName, line).bytes ||
                d.typeName != s.typeName || s.range.lo < d.range.lo || s.range.hi > d.range.hi)
                fail(line, "copy does not preserve destination type/range; bulk copies need "
                           "qualification");
            if (d.id == s.id && d.offset < s.offset + size && s.offset < d.offset + size)
                fail(line, "overlapping copy");
            copy(out, d.id, d.offset, s.id, s.offset, size);
            return;
        }
        if (op == "call" || op == "callif") {
            count(n, op == "call" ? 1 : 4);
            auto &p = symbol(a[0], Kind::Program);
            if (op == "call")
                call(out, p.id);
            else {
                auto &l = symbol(a[1], Kind::Field);
                auto &r = symbol(a[3], Kind::Field);
                if (l.typeName != "u16" || r.typeName != "u16" || l.range.hi > 65534 ||
                    r.range.hi > 65534)
                    fail(line, "condition requires bounded u16 fields excluding 65535");
                static const std::map<std::string, unsigned> ops = {
                    {"eq", 2}, {"ne", 3}, {"lt", 4}, {"gt", 5}, {"le", 6}, {"ge", 7}};
                auto i = ops.find(lower(a[2].text));
                if (i == ops.end())
                    fail(line, "unknown comparison");
                emitCondCallBufferValue(out, int(p.id), static_cast<unsigned char>(i->second),
                                        int(l.id), l.offset, int(r.id), r.offset);
                edges[currentProgram].insert(p.id);
            }
            for (unsigned id : mutableMatrices)
                bounds.erase(id);
            return;
        }
        if (op == "matvalues") {
            if (a.empty())
                fail(line, "MatValues missing matrix");
            auto &m = symbol(a[0], Kind::Matrix);
            if (a.size() != 1 + m.rows * m.cols)
                fail(line, "matrix value count mismatch");
            matrixHeader(out, m, 0);
            out.push_back(0);
            std::vector<Range> v;
            for (size_t i = 1; i < a.size(); ++i) {
                if (!a[i].number)
                    fail(line, "MatValues requires literals");
                floatBytes(out, a[i].value);
                v.push_back(exact(double(float(a[i].value))));
            }
            bounds[m.id] = v;
            return;
        }
        if (op == "matload") {
            count(n, 2);
            auto &m = symbol(a[0], Kind::Matrix);
            auto f = operand(a[1]);
            auto t = type(f.typeName, line);
            span(f, m.rows * m.cols * t.bytes);
            matrixHeader(out, m, 0x20);
            fetched(out, f);
            std::vector<Range> v(m.rows * m.cols, t.range);
            for (unsigned i = 0; i < v.size(); ++i)
                for (const auto &kv : syms) {
                    const auto &sf = kv.second;
                    if (sf.kind == Kind::Field && sf.id == f.id &&
                        sf.offset == f.offset + i * t.bytes && sf.typeName == f.typeName)
                        v[i] = sf.range;
                }
            if (const auto *owner = matrixOwner(f)) {
                auto source = value(*owner);
                std::copy_n(source.begin() + f.offset / 4, v.size(), v.begin());
            }
            bounds[m.id] = v;
            return;
        }
        if (op == "matadd" || op == "matsub" || op == "matmul") {
            count(n, 3);
            auto &d = symbol(a[0], Kind::Matrix);
            auto &x = symbol(a[1], Kind::Matrix);
            auto &y = symbol(a[2], Kind::Matrix);
            separate(d, x);
            separate(d, y);
            if (op == "matmul") {
                if (x.cols != y.rows || d.rows != x.rows || d.cols != y.cols)
                    fail(line, "matrix product shape mismatch");
            } else {
                shapes(d, x);
                shapes(d, y);
            }
            matrixHeader(out, d, op == "matadd" ? 4 : op == "matsub" ? 5 : 6);
            emitLE16(out, x.id);
            emitLE16(out, y.id);
            auto xv = value(x), yv = value(y);
            std::vector<Range> v(d.rows * d.cols, exact(0));
            for (unsigned i = 0; i < d.rows; ++i)
                for (unsigned j = 0; j < d.cols; ++j) {
                    auto &r = v[i * d.cols + j];
                    if (op == "matmul") {
                        for (unsigned k = 0; k < x.cols; ++k)
                            r = plus(r, times(xv[i * x.cols + k], yv[k * y.cols + j]));
                    } else
                        r = plus(xv[i * d.cols + j], op == "matadd"
                                                         ? yv[i * d.cols + j]
                                                         : times(exact(-1), yv[i * d.cols + j]));
                }
            bounds[d.id] = v;
            return;
        }
        if (op == "matscale") {
            count(n, 3);
            auto &d = symbol(a[0], Kind::Matrix);
            auto &s = symbol(a[1], Kind::Matrix);
            separate(d, s);
            shapes(d, s);
            auto f = operand(a[2]);
            matrixHeader(out, d, 0x27);
            emitLE16(out, s.id);
            fetched(out, f);
            auto v = value(s);
            for (auto &r : v)
                r = times(r, f.range);
            bounds[d.id] = v;
            return;
        }
        if (op == "matextract") {
            count(n, 4);
            auto &d = symbol(a[0], Kind::Matrix);
            auto &s = symbol(a[1], Kind::Matrix);
            separate(d, s);
            unsigned row = integer(a[2], 7), col = integer(a[3], 7);
            if (row + d.rows > s.rows || col + d.cols > s.cols)
                fail(line, "submatrix outside source");
            matrixHeader(out, d, 8);
            emitLE16(out, s.id);
            out.push_back(static_cast<unsigned char>(row));
            out.push_back(static_cast<unsigned char>(col));
            auto sv = value(s);
            std::vector<Range> v;
            for (unsigned i = 0; i < d.rows; ++i)
                for (unsigned j = 0; j < d.cols; ++j)
                    v.push_back(sv[(row + i) * s.cols + col + j]);
            bounds[d.id] = v;
            return;
        }
        if (op == "matput") {
            count(n, 5);
            auto &d = symbol(a[0], Kind::Matrix);
            auto &s = symbol(a[1], Kind::Matrix);
            separate(d, s);
            shapes(d, s);
            unsigned row = integer(a[2], 7), col = integer(a[3], 7);
            if (row >= d.rows || col >= d.cols)
                fail(line, "matrix cell outside destination");
            auto f = operand(a[4]);
            matrixHeader(out, d, 0x21);
            emitLE16(out, s.id);
            out.push_back(static_cast<unsigned char>(row));
            out.push_back(static_cast<unsigned char>(col));
            fetched(out, f);
            auto v = value(s);
            v[row * d.cols + col] = f.range;
            bounds[d.id] = v;
            return;
        }
        if (op == "identity") {
            count(n, 1);
            identity(out, symbol(a[0], Kind::Matrix));
            return;
        }
        if (op == "translate" || op == "scale" || op == "rotatedegrees") {
            count(n, op == "rotatedegrees" ? 2 : 3);
            auto &m = symbol(a[0], Kind::Matrix);
            std::vector<Symbol> args;
            for (size_t i = 1; i < a.size(); ++i)
                args.push_back(operand(a[i]));
            affine(out, m, op == "translate" ? 6 : op == "scale" ? 5 : 2, args);
            std::vector<Range> transform(9, exact(0));
            transform[0] = transform[4] = transform[8] = exact(1);
            if (op == "translate") {
                transform[2] = args[0].range;
                transform[5] = args[1].range;
            } else if (op == "scale") {
                transform[0] = args[0].range;
                transform[4] = args[1].range;
            } else {
                transform[0] = transform[1] = transform[3] = transform[4] = {-1, 1};
            }
            // Stock left-multiplies: supplied transform * existing.
            bounds[m.id] = product3(transform, value(m));
            return;
        }
        if (op == "invertaffine") {
            count(n, 1);
            auto &m = symbol(a[0], Kind::Matrix);
            if (m.rows != 3 || m.cols != 3)
                fail(line, "qualified inverse currently requires 3x3 diagonal affine");
            auto v = value(m);
            for (unsigned i = 0; i < 9; ++i) {
                if (i % 4) {
                    if (v[i].lo != 0 || v[i].hi != 0)
                        fail(line, "inverse nonsingularity not proved: expected diagonal");
                } else {
                    if (!std::isfinite(v[i].lo) || !std::isfinite(v[i].hi) ||
                        (v[i].lo <= 0 && v[i].hi >= 0))
                        fail(line, "inverse denominator may be zero/nonfinite");
                    v[i] = rounded(1 / v[i].hi, 1 / v[i].lo);
                }
            }
            command(out, m.id, 32);
            out.push_back(1);
            bounds[m.id] = v;
            return;
        }
        if (op == "storei16floor" || op == "storeu16floor") {
            count(n, 2);
            auto &f = symbol(a[0], Kind::Field);
            auto &m = symbol(a[1], Kind::Matrix);
            bool u = op == "storeu16floor";
            if (f.typeName != (u ? "u16" : "s16") || m.rows != 1 || m.cols != 1)
                fail(line, "integer floor store needs matching word field and scalar matrix");
            storeWordFloor(out, f, m, u);
            return;
        }
        if (op == "matabs") {
            count(n, 2);
            auto &dst = symbol(a[0], Kind::Matrix), &src = symbol(a[1], Kind::Matrix);
            if (dst.rows != 1 || dst.cols != 1 || src.rows != 1 || src.cols != 1 ||
                dst.id == src.id)
                fail(line, "MatAbs requires distinct scalar matrices");
            auto range = value(src)[0];
            if (!std::isfinite(range.lo) || !std::isfinite(range.hi))
                fail(line, "MatAbs requires finite source bounds");
            Symbol field;
            field.kind = Kind::Field;
            field.id = src.id;
            field.size = 4;
            field.typeName = "f32";
            field.range = range;
            loadScalar(out, dst, field);
            emitAdjustMultiLiteral(out, int(dst.id), 5, 3, {127});
            bounds[dst.id] = {{range.lo <= 0 && range.hi >= 0
                                  ? 0 : std::min(std::abs(range.lo), std::abs(range.hi)),
                              std::max(std::abs(range.lo), std::abs(range.hi))}};
            return;
        }
        if (op == "storepositive32floor") {
            count(n, 2);
            auto &dst = symbol(a[0], Kind::Field), &src = symbol(a[1], Kind::Matrix);
            if (dst.typeName != "s32" || src.rows != 1 || src.cols != 1)
                fail(line, "StorePositive32Floor requires s32 destination and scalar source");
            storePositive32Floor(out, dst, src);
            return;
        }
        if (op == "mattrunc" || op == "matroundaway") {
            count(n, 2);
            auto &dst = symbol(a[0], Kind::Matrix), &src = symbol(a[1], Kind::Matrix);
            if (dst.rows != 1 || dst.cols != 1 || src.rows != 1 || src.cols != 1 ||
                dst.id == src.id)
                fail(line, "MatTrunc requires distinct scalar matrices");
            auto range = value(src)[0];
            double maximum = std::max(std::abs(range.lo), std::abs(range.hi));
            bool nearest = op == "matroundaway";
            if (!std::isfinite(maximum) || maximum > (nearest ? 8388607 : 16777215))
                fail(line, "MatTrunc requires finite magnitude <=16777215");
            // Preserve the IEEE binary32 sign separately. Only a nonnegative
            // magnitude reaches stock's unsigned float-to-fixed conversion.
            unsigned absolute = scratch(4), sign = scratch(1);
            copy(out, absolute, 0, src.id, 0, 4);
            copy(out, sign, 0, src.id, 3, 1);
            emitAdjustMultiLiteral(out, int(absolute), 5, 3, {127});
            emitAdjustMultiLiteral(out, int(sign), 5, 0, {128});
            Symbol field;
            field.kind = Kind::Field;
            field.id = absolute;
            field.size = 4;
            field.typeName = "f32";
            field.range = {0, maximum};
            Symbol transform;
            transform.kind = Kind::Matrix;
            transform.id = scratch(36);
            transform.rows = transform.cols = 3;
            transform.size = 36;
            Arg zero;
            zero.number = true;
            identity(out, transform);
            affine(out, transform, 6, {field, operand(zero)});
            // The predecessor of 0.5 avoids rounding nextafter(0.5,0)+0.5
            // up to 1 before truncation. Exact half ties still round upwards.
            if (nearest) {
                Arg half = zero;
                half.value = std::nextafter(0.5f, 0.0f);
                affine(out, transform, 6, {operand(half), operand(zero)});
            }
            unsigned source = scratch(8), integerResult = scratch(8);
            command(out, integerResult, 41);
            out.push_back(8);
            out.push_back(0x40);
            emitLE16(out, transform.id);
            emitLE16(out, source);
            emitLE16(out, 1);
            field.id = integerResult;
            field.size = 8;
            field.typeName = "s32";
            field.range = {0, nearest ? std::round(maximum) : std::floor(maximum)};
            loadScalar(out, dst, field);
            emitAdjustMultiBufferFetched(out, int(dst.id), 6, 3, 1, int(sign), 0);
            bounds[dst.id] = {{nearest ? std::round(range.lo) : std::trunc(range.lo),
                               nearest ? std::round(range.hi) : std::trunc(range.hi)}};
            return;
        }
        if (op == "divmodpositive") {
            count(n, 4);
            auto &q = symbol(a[0], Kind::Field), &r = symbol(a[1], Kind::Field),
                 &input = symbol(a[2], Kind::Field);
            unsigned divisor = integer(a[3], 16777215);
            if (!divisor || q.typeName != "u16" ||
                (r.typeName != "u16" && r.typeName != "s32") || input.typeName != "s32" ||
                input.range.lo < 0 || input.range.hi > 16777215 ||
                std::ceil(input.range.lo) > std::floor(input.range.hi))
                fail(line, "DivModPositive needs bounded nonnegative s32 input, u16 quotient and u16/s32 remainder");
            unsigned low = unsigned(std::ceil(input.range.lo)),
                     high = unsigned(std::floor(input.range.hi));
            if (high / divisor > 32767 || q.range.lo > low / divisor ||
                q.range.hi < high / divisor || r.range.lo > 0 || r.range.hi < divisor - 1)
                fail(line, "DivModPositive output ranges cannot contain the result");
            writable(q, 2);
            unsigned remainderBytes = type(r.typeName, line).bytes;
            writable(r, remainderBytes);
            auto overlap = [](const Symbol &a, unsigned aw, const Symbol &b, unsigned bw) {
                return a.id == b.id && a.offset < b.offset + bw && b.offset < a.offset + aw;
            };
            if (overlap(q, 2, r, remainderBytes) || overlap(q, 2, input, 4) ||
                overlap(r, remainderBytes, input, 4))
                fail(line, "DivModPositive storage overlaps");
            float reciprocal = 1.0f / float(divisor);
            // Exhaustive compile-time proof of this finite integer domain under
            // binary32 multiplication. Emit operations, never a table of answers.
            for (unsigned x = low; x <= high; ++x) {
                float divided = float(x) * reciprocal;
                if (unsigned(divided) != x / divisor)
                    fail(line, "binary32 constant division is not exact over input domain at " +
                                   std::to_string(x));
            }
            auto x = privateScalar(), divided = privateScalar(), integerQ = privateScalar(),
                 multiple = privateScalar(), remainder = privateScalar();
            loadScalar(out, x, input);
            scaleScalar(out, divided, x, double(reciprocal));
            storeWordFloor(out, q, divided, true);
            Symbol signedQ = q;
            signedQ.typeName = "s16";
            signedQ.range = {double(low / divisor), double(high / divisor)};
            loadScalar(out, integerQ, signedQ);
            scaleScalar(out, multiple, integerQ, divisor);
            matrixHeader(out, remainder, 5);
            emitLE16(out, x.id);
            emitLE16(out, multiple.id);
            // x and q*divisor are exact integers below 2^24, so subtraction is
            // exact and the proved division identity gives this tight interval.
            bounds[remainder.id] = {{0, double(divisor - 1)}};
            if (r.typeName == "s32")
                storePositive32Floor(out, r, remainder);
            else
                storeWordFloor(out, r, remainder, true);
            listing << "divmod_positive " << currentProgram << " source " << input.id << ":"
                    << input.offset << " divisor " << divisor << " exhaustive_domain " << low << " "
                    << high << "\n";
            return;
        }
        if (op == "transformdata") {
            count(n, 8);
            auto &dst = buffer(a[0]);
            auto &src = buffer(a[1]);
            auto &m = symbol(a[2], Kind::Matrix);
            if (dst.id == src.id || dst.size != src.size || m.rows != m.cols || m.rows < 2)
                fail(line, "TransformData requires distinct equal-sized buffers and square matrix");
            const auto formatName = lower(a[3].text);
            auto t = type(formatName, line);
            // The public fixed-point form needs interval qualification of every
            // input tuple; never silently invoke stock negative-to-unsigned casts.
            // StoreI16Floor provides the checked fixed-point path for R19-04.
            if (formatName != "f32")
                fail(line, "fixed-point TransformData awaits tuple range qualification; use "
                           "checked signed store");
            unsigned offset = integer(a[4], 65535), stride = integer(a[5], 65535),
                     limit = integer(a[6], 65535), components = integer(a[7], 8);
            if (!limit || !components || components > m.rows || stride < components * t.bytes ||
                offset > src.size ||
                uint64_t(offset) + uint64_t(limit - 1) * stride + components * t.bytes > src.size)
                fail(line, "invalid transform span/count/stride");
            auto mv = value(m);
            for (unsigned tuple = 0; tuple < limit; ++tuple) {
                std::vector<Range> input(m.rows, exact(1));
                for (unsigned k = 0; k < components; ++k) {
                    input[k] = {};
                    for (const auto &kv : syms) {
                        const auto &f = kv.second;
                        if (f.kind == Kind::Field && f.id == src.id &&
                            f.offset == offset + tuple * stride + k * t.bytes &&
                            f.typeName == formatName)
                            input[k] = f.range;
                    }
                }
                for (unsigned r = 0; r < components; ++r) {
                    Range result = exact(0);
                    for (unsigned k = 0; k < m.cols; ++k)
                        result = plus(result, times(mv[r * m.cols + k], input[k]));
                    if (!std::isfinite(result.lo) || !std::isfinite(result.hi))
                        fail(line,
                             "transform finite result bounds not proved; declare tuple fields");
                }
            }
            command(out, dst.id, 41);
            out.push_back(15);
            out.push_back(static_cast<unsigned char>(t.format));
            emitLE16(out, m.id);
            emitLE16(out, src.id);
            out.push_back(static_cast<unsigned char>(components));
            emitLE16(out, offset);
            emitLE16(out, stride);
            emitLE16(out, limit);
            return;
        }
        if (op == "plotquad") {
            count(n, 9);
            unsigned colour = integer(a[0], 63), instruction = scratch(27);
            Bytes bytes = {18, 0, static_cast<unsigned char>(colour)};
            for (unsigned i = 0; i < 4; ++i) {
                bytes.push_back(25);
                bytes.push_back(i < 2 ? 4 : 85);
                bytes.insert(bytes.end(), 4, 0);
            }
            initial[instruction] = bytes;
            for (unsigned i = 0; i < 8; ++i) {
                auto &f = symbol(a[1 + i], Kind::Field);
                if (f.typeName != "s16")
                    fail(line, "PlotQuad coordinates require signed words");
                copy(out, instruction, 5 + (i / 2) * 6 + (i % 2) * 2, f.id, f.offset, 2);
            }
            call(out, instruction);
            return;
        }
        if (op == "plotpoint") {
            count(n, 3);
            unsigned mode = integer(a[0], 255);
            auto &x = symbol(a[1], Kind::Field);
            auto &y = symbol(a[2], Kind::Field);
            if (x.typeName != "s16" || y.typeName != "s16")
                fail(line, "PLOT coordinates require s16 fields");
            unsigned id = scratch(6);
            initial[id] = {25, static_cast<unsigned char>(mode), 0, 0, 0, 0};
            copy(out, id, 2, x.id, x.offset, 2);
            copy(out, id, 4, y.id, y.offset, 2);
            call(out, id);
            return;
        }
        if (op == "addu16") {
            count(n, 2);
            auto &f = symbol(a[0], Kind::Field);
            writable(f, 2);
            if (f.typeName != "u16" || f.range.lo != 0 || f.range.hi != 65535)
                fail(line, "modular addition requires full-range u16 destination");
            unsigned id = scratch(3);
            copy(out, id, 0, f.id, f.offset, 2);
            if (a[1].number)
                emitAdjustMultiLiteral(out, int(id), 4, 0, le16Bytes(integer(a[1], 65535)));
            else {
                auto &s = symbol(a[1], Kind::Field);
                if (s.typeName != "u16")
                    fail(line, "AddU16 operand must be u16");
                emitAdjustMultiBufferFetched(out, int(id), 4, 0, 2, int(s.id), s.offset);
            }
            copy(out, f.id, f.offset, id, 0, 2);
            return;
        }
        if (op == "repeat") {
            count(n, 1);
            unsigned trips = integer(a[0], 512), counter = scratch(3), limit = scratch(2),
                     body = scratch(0);
            initial[limit] = le16Bytes(trips);
            initial.erase(body);
            auto saved = bounds;
            for (unsigned id : mutableMatrices)
                bounds.erase(id);
            unsigned parent = currentProgram;
            currentProgram = body;
            Bytes bytes;
            statements(n.body, bytes);
            currentProgram = parent;
            emitAdjustMultiLiteral(bytes, int(counter), 4, 0, {1, 0});
            emitCondCallBufferValue(bytes, int(body), 4, int(counter), 0, int(limit), 0);
            finiteRepeat.insert(body);
            programs[body] = bytes;
            if (trips) {
                emitAdjustMultiLiteral(out, int(counter), 2, 0, {0, 0, 0});
                call(out, body);
            }
            bounds = saved;
            for (unsigned id : mutableMatrices)
                bounds.erase(id);
            return;
        }
        fail(line, "hosted intrinsic not implemented: " + op);
    }

  public:
    explicit Compiler(std::filesystem::path directory = std::filesystem::current_path())
        : sourceDirectory(std::move(directory)) {}
    Result run(const std::string &source) {
        listing << std::setprecision(17);
        auto nodes = Parser(source).block();
        // Opt-in, first top-level declaration only. The floor-store expansion
        // resets its transform and consumes/copies the result before it returns;
        // no user Call or yield occurs inside it. These private temporaries can
        // therefore be shared across successive stores and nested user calls.
        if (!nodes.empty() && nodes.front().name == "sharedwordscratch") {
            count(nodes.front(), 0);
            sharedWordScratch = true;
            nodes.erase(nodes.begin());
            listing << "scalar_word_scratch shared_nonreentrant_expansion\n";
        }
        for (const auto &n : nodes)
            declare(n);
        for (const auto &n : nodes)
            if (n.name == "field")
                field(n);
        for (const auto &kv : syms)
            if (kv.second.kind == Kind::Asset)
                validateFields(kv.second, kv.second.data, 0);
        for (const auto &n : nodes)
            if (n.name == "program")
                markMutable(n.body);
        for (const auto &n : nodes)
            if (n.name != "buffer" && n.name != "field" && n.name != "matrix" &&
                n.name != "program" && n.name != "asset" && n.name != "table")
                statement(n, setup);
        bootstrapBounds = bounds;
        for (const auto &n : nodes)
            if (n.name == "program") {
                currentProgram = syms.at(n.text).id;
                bounds = bootstrapBounds;
                for (unsigned id : mutableMatrices)
                    bounds.erase(id);
                statements(n.body, programs[currentProgram]);
                if (programs[currentProgram].size() > 65535)
                    fail(n.line, "program exceeds one block");
            }
        std::set<unsigned> visiting;
        std::function<void(unsigned, unsigned)> visit = [&](unsigned id, unsigned depth) {
            if (depth > 16)
                fail(1, "call depth exceeds 16");
            if (!visiting.insert(id).second)
                fail(1, "recursive hosted call");
            for (unsigned to : edges[id])
                visit(to, depth + 1);
            visiting.erase(id);
        };
        for (const auto &p : programs)
            visit(p.first, 1);
        visit(0, 1);
        Result r;
        r.code = {23, 0, 248, 1, 0, 1, 0}; // Stock affine feature flag, no firmware modification.
        unsigned resident = 0, assetBytes = 0, programBytes = 0, scratchBytes = 0;
        std::set<unsigned> assetIds, executableIds;
        for (const auto &kv : syms)
            if (kv.second.kind == Kind::Asset || kv.second.kind == Kind::Table)
                for (unsigned i = 0; i < kv.second.records; ++i)
                    assetIds.insert(kv.second.id + i);
        for (const auto &edge : edges)
            executableIds.insert(edge.second.begin(), edge.second.end());
        for (const auto &kv : initial) {
            command(r.code, kv.first, 2);
            emitWriteBlock(r.code, int(kv.first), kv.second);
            resident += unsigned(kv.second.size());
            if (assetIds.count(kv.first))
                assetBytes += unsigned(kv.second.size());
            else if (executableIds.count(kv.first))
                programBytes += unsigned(kv.second.size());
            else
                scratchBytes += unsigned(kv.second.size());
        }
        for (const auto &kv : programs) {
            if (kv.second.size() > 65535)
                fail(1, "generated program exceeds one write block");
            command(r.code, kv.first, 2);
            emitWriteBlock(r.code, int(kv.first), kv.second);
            resident += unsigned(kv.second.size());
            programBytes += unsigned(kv.second.size());
        }
        r.code.insert(r.code.end(), setup.begin(), setup.end());
        if (assetBytes > 512 * 1024 || programBytes > 128 * 1024 || scratchBytes > 64 * 1024)
            fail(1, "hosted asset/program/scratch budget exceeded");
        if (ids.size() > 1024 || resident + r.code.size() > 1024 * 1024)
            fail(1, "hosted resident resource budget exceeded");
        for (const auto &kv : syms)
            if (kv.second.kind != Kind::Field)
                listing << "symbol " << kv.first << " " << kv.second.id << " "
                        << (kv.second.kind == Kind::Program ? programs.at(kv.second.id).size()
                                                            : kv.second.size)
                        << " " << int(kv.second.kind) << "\n";
        listing << "bootstrap_bytes " << r.code.size() << "\nresident_payload_bytes " << resident
                << "\nowned_ids " << ids.size() << "\nasset_payload_bytes " << assetBytes
                << "\nprogram_payload_bytes " << programBytes << "\nscratch_payload_bytes "
                << scratchBytes << "\nresident_plus_bootstrap_payload_bytes "
                << resident + r.code.size() << "\n";
        for (unsigned id : ids) {
            command(r.clear, id, 2);
            listing << "owned " << id << "\n";
        }
        listing << "cleanup_bytes " << r.clear.size() << "\n";
        r.map = listing.str();
        return r;
    }
};
inline Result compile(const std::string &source,
                      const std::filesystem::path &directory = std::filesystem::current_path()) {
    return Compiler(directory).run(source);
}
} // namespace hosted

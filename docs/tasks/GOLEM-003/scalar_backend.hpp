#pragma once

// Task-local extraction from Rally hosted.hpp at 4db7e96f70c3d63f4ab53766f3b32063e9357c8b.
// Preserves loadScalar, MatFloor and signed storeWordFloor command expansions.
// This backend has no source parser, Rally Node/Arg dependency or source generation.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace scalar {
using Bytes = std::vector<unsigned char>;
struct Error : std::runtime_error { using std::runtime_error::runtime_error; };
struct Range { double lo, hi; };
struct Spec {
    std::string input, output, entry;
    Range range;
    double initial = 0;
};
// This is a provisional compatibility policy, not the language's ID allocation ABI.
struct Layout {
    unsigned input = 1000, output = 1001, source = 1100, integral = 1101, entry = 2000;
};
enum class Format : unsigned char { F32 = 0, S32 = 0x40, S16 = 0xc0 };
struct Field { unsigned id, offset; Format format; };
struct Scalar { unsigned id; Range range; };
struct Result {
    Bytes bootstrap, cleanup, invoke;
    std::map<unsigned, Bytes> initial, programs;
    Spec spec;
    Layout layout;
};

inline void le16(Bytes &out, unsigned value) {
    if (value > 65535) throw Error("16-bit operand overflow");
    out.push_back(static_cast<unsigned char>(value));
    out.push_back(static_cast<unsigned char>(value >> 8));
}
inline void command(Bytes &out, unsigned id, unsigned op) {
    out.insert(out.end(), {23, 0, 160});
    le16(out, id);
    out.push_back(static_cast<unsigned char>(op));
}
inline Bytes binary32(double value) {
    static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559,
                  "requires IEEE-754 binary32");
    if (!std::isfinite(value) || std::abs(value) > std::numeric_limits<float>::max())
        throw Error("expected finite representable float32");
    float rounded = static_cast<float>(value);
    uint32_t bits;
    std::memcpy(&bits, &rounded, sizeof(bits));
    Bytes out;
    for (unsigned i = 0; i < 4; ++i)
        out.push_back(static_cast<unsigned char>(bits >> (8 * i)));
    return out;
}
inline void literal(Bytes &out, unsigned id, unsigned op, unsigned offset, const Bytes &bytes) {
    command(out, id, 5);
    out.push_back(static_cast<unsigned char>(op | 0xc0));
    le16(out, offset);
    le16(out, static_cast<unsigned>(bytes.size()));
    out.insert(out.end(), bytes.begin(), bytes.end());
}
inline void copy(Bytes &out, unsigned dst, unsigned offset, unsigned src,
                 unsigned from, unsigned count, unsigned op = 2) {
    command(out, dst, 5);
    out.push_back(static_cast<unsigned char>(op | 0xe0));
    le16(out, offset); le16(out, count); le16(out, src); le16(out, from);
}
inline void condition(Bytes &out, unsigned target, unsigned op, unsigned left, unsigned right) {
    // Compare buffer-fetched u16 words on both sides; preserve firmware sentinel workaround.
    command(out, target, 6);
    out.push_back(static_cast<unsigned char>(op | 0xa0));
    le16(out, left); le16(out, 0); le16(out, right); le16(out, 0);
}
inline void writeBlock(Bytes &out, unsigned id, const Bytes &bytes) {
    if (bytes.size() > 65535) throw Error("write block exceeds 65535 bytes");
    command(out, id, 0);
    le16(out, static_cast<unsigned>(bytes.size()));
    out.insert(out.end(), bytes.begin(), bytes.end());
}
inline void validateInput(const Spec &spec, double value) {
    const auto bytes = binary32(value);
    (void)bytes;
    double rounded = static_cast<float>(value);
    if (value < spec.range.lo || value > spec.range.hi ||
        rounded < spec.range.lo || rounded > spec.range.hi)
        throw Error("input and its float32 rounding must satisfy declared Range");
}
inline Bytes inputUpdate(const Result &result, double value) {
    validateInput(result.spec, value);
    Bytes out;
    literal(out, result.layout.input, 2, 0, binary32(value));
    return out;
}

class Lowering {
    std::map<unsigned, Bytes> initial, programs;
    std::map<unsigned, unsigned> constants;
    unsigned next = 2;
    unsigned scratch(unsigned size) {
        if (next >= 1000) throw Error("private scratch ID budget exceeded");
        unsigned id = next++;
        initial[id] = Bytes(size, 0);
        return id;
    }
    unsigned helper() {
        unsigned id = scratch(0);
        initial.erase(id);
        programs[id] = {};
        return id;
    }
    unsigned constant(unsigned value) {
        auto it = constants.find(value);
        if (it != constants.end()) return it->second;
        unsigned id = scratch(2);
        initial[id].clear();
        le16(initial[id], value);
        constants[value] = id;
        return id;
    }
    Field floatLiteral(double value) {
        unsigned id = scratch(4);
        initial[id] = binary32(value);
        return {id, 0, Format::F32};
    }
    static void fetched(Bytes &out, Field field) {
        out.push_back(static_cast<unsigned char>(field.format));
        le16(out, field.id); le16(out, field.offset);
    }
    static void loadScalar(Bytes &out, unsigned dst, Field field) {
        command(out, dst, 34);
        out.insert(out.end(), {0x20, 1, 1});
        fetched(out, field);
    }
    static void identity(Bytes &out, unsigned transform) {
        command(out, transform, 32); out.push_back(0);
    }
    static void translate(Bytes &out, unsigned transform, Field x, Field y) {
        command(out, transform, 32); out.push_back(0x66);
        fetched(out, x); fetched(out, y);
    }
    static void transformData(Bytes &out, unsigned dst, unsigned transform,
                              unsigned source, Format format) {
        command(out, dst, 41);
        out.push_back(8); // One explicit set; source width is one affine coordinate pair.
        out.push_back(static_cast<unsigned char>(format));
        le16(out, transform); le16(out, source); le16(out, 1);
    }
    Scalar floorScalar(Bytes &out, unsigned dst, Scalar src) {
        if (dst == src.id) throw Error("floor requires distinct scalar matrices");
        double maximum = std::max(std::abs(src.range.lo), std::abs(src.range.hi));
        if (!std::isfinite(maximum) || maximum > 16777215)
            throw Error("floor requires finite magnitude <=16777215");
        unsigned absolute = scratch(4), sign = scratch(1);
        copy(out, absolute, 0, src.id, 0, 4);
        copy(out, sign, 0, src.id, 3, 1);
        literal(out, absolute, 5, 3, {127});
        literal(out, sign, 5, 0, {128});
        unsigned transform = scratch(36);
        identity(out, transform);
        translate(out, transform, {absolute, 0, Format::F32}, floatLiteral(0));
        unsigned source = scratch(8), integer = scratch(8);
        transformData(out, integer, transform, source, Format::S32);
        Field integerField{integer, 0, Format::S32};
        loadScalar(out, dst, integerField);

        // Preserve the original sign and detect ANY fractional magnitude by comparing
        // binary32 bytes. A 32768 bias alone loses tiny negative fractions.
        unsigned flag = scratch(2), left = scratch(2), right = scratch(2);
        unsigned different = helper(), negative = helper(), increment = helper();
        literal(programs[different], flag, 2, 0, {1, 0});
        literal(programs[increment], integer, 4, 0, {1, 0, 0, 0});
        auto &body = programs[negative];
        literal(body, flag, 2, 0, {0, 0});
        for (unsigned i = 0; i < 4; ++i) {
            copy(body, left, 0, absolute, i, 1);
            copy(body, right, 0, dst, i, 1);
            condition(body, different, 3, left, right);
        }
        condition(body, increment, 2, flag, constant(1));
        copy(out, left, 0, sign, 0, 1);
        condition(out, negative, 2, left, constant(128));
        loadScalar(out, dst, integerField);
        copy(out, dst, 3, sign, 0, 1, 6); // OR the preserved sign into binary32.
        return {dst, {std::floor(src.range.lo), std::floor(src.range.hi)}};
    }
    void storeSignedWord(Bytes &out, Field dst, Scalar src) {
        if (dst.format != Format::S16 || src.range.lo < -32768 || src.range.hi > 32767 ||
            !std::isfinite(src.range.lo) || !std::isfinite(src.range.hi))
            throw Error("signed word store requires proven -32768..32767 range");
        unsigned transform = scratch(36);
        identity(out, transform);
        translate(out, transform, {src.id, 0, Format::F32}, floatLiteral(0));
        Field bias = floatLiteral(32768), zero = floatLiteral(0);
        translate(out, transform, bias, zero);
        unsigned source = scratch(4), dest = scratch(4);
        transformData(out, dest, transform, source, Format::S16);
        command(out, dest, 5); out.push_back(7);
        le16(out, 1); out.push_back(128); // Undo sign bias in the temporary only.
        copy(out, dst.id, dst.offset, dest, 0, 2);
    }

  public:
    Result run(Spec spec, Layout layout = {}) && {
        if (!std::isfinite(spec.range.lo) || !std::isfinite(spec.range.hi) ||
            spec.range.lo > spec.range.hi || spec.range.lo < -32768 || spec.range.hi > 32767)
            throw Error("this slice requires finite input Range within -32768..32767");
        validateInput(spec, spec.initial);
        std::set<unsigned> ids;
        for (unsigned id : {layout.input, layout.output, layout.source, layout.integral, layout.entry}) {
            if (id < 1000 || id >= 65535 || !ids.insert(id).second)
                throw Error("compatibility layout needs five distinct IDs in 1000..65534");
        }
        initial[layout.input] = Bytes(4, 0);
        initial[layout.output] = Bytes(2, 0);
        initial[layout.source] = Bytes(4, 0);
        initial[layout.integral] = Bytes(4, 0);
        Bytes &job = programs[layout.entry];
        loadScalar(job, layout.source, {layout.input, 0, Format::F32});
        Scalar integral = floorScalar(job, layout.integral, {layout.source, spec.range});
        storeSignedWord(job, {layout.output, 0, Format::S16}, integral);

        Result result;
        result.spec = std::move(spec); result.layout = layout;
        result.initial = initial; result.programs = programs;
        result.bootstrap = {23, 0, 248, 1, 0, 1, 0}; // Stock affine feature enable.
        for (const auto &buffer : initial) {
            command(result.bootstrap, buffer.first, 2);
            writeBlock(result.bootstrap, buffer.first, buffer.second);
            ids.insert(buffer.first);
        }
        for (const auto &program : programs) {
            command(result.bootstrap, program.first, 2);
            writeBlock(result.bootstrap, program.first, program.second);
            ids.insert(program.first);
        }
        auto update = inputUpdate(result, result.spec.initial);
        result.bootstrap.insert(result.bootstrap.end(), update.begin(), update.end());
        command(result.invoke, layout.entry, 1);
        for (unsigned id : ids) command(result.cleanup, id, 2);
        return result;
    }
};
inline Result compile(const Spec &spec, Layout layout = {}) {
    return Lowering().run(spec, layout);
}
} // namespace scalar

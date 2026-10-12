#pragma once
#include "resident.hpp"

namespace golem::resident_drawing {
using namespace resident_scalar;
enum class Type { Uint8, Sint16, Uint16 };
struct Global {
    std::string name;
    Type type;
    int lo, hi, value;
    unsigned offset;
    unsigned width() const { return type == Type::Uint8 ? 1 : 2; }
    const char *typeName() const {
        return type == Type::Uint8 ? "uint8" : type == Type::Uint16 ? "uint16" : "sint16";
    }
    unsigned typeTag() const { return type == Type::Uint8 ? 5 : type == Type::Uint16 ? 1 : 2; }
};
struct Operand { int field = -1, value = 0; };
struct Operation { std::string name; std::vector<Operand> args; };
struct Spec {
    std::vector<Global> globals;
    std::vector<Operation> operations;
    std::string entry;
    Bytes values;
};
struct Result {
    Spec spec;
    unsigned input = 0, entry = 0;
    Bytes bootstrap, cleanup, invoke;
    std::map<unsigned, Bytes> initial, programs;
    std::map<unsigned, unsigned> owned;
    std::map<unsigned, std::string> roles;
    std::set<unsigned> initialBitmapIDs;
};
inline bool profile(const std::string &source) {
    syntax::Reader r(source);
    if (r.keyword("uint8") || r.keyword("sint16") || r.keyword("Sub")) return true;
    // uint16 also starts the scalar profile. Inspect the first Sub statement,
    // using tokens so comments and string contents cannot select a backend.
    if (!r.keyword("uint16")) return false;
    while (!r.end() && !r.keyword("Sub")) r.pop();
    if (r.end()) return false;
    r.pop(); r.name();
    return r.keyword("draw_bitmap") || r.keyword("draw_point") ||
           r.keyword("draw_line") || r.keyword("fill_quad");
}
inline Spec parse(const std::string &source) {
    syntax::Reader r(source);
    Spec s;
    std::map<std::string, unsigned> fields;
    std::set<std::string> names;
    auto name = [&]() {
        auto t = r.name();
        const auto folded = syntax::lower(t.text);
        if (folded == "uint8" || folded == "bounds" || folded == "draw_point" ||
                folded == "draw_line" || folded == "fill_quad" || folded == "draw_bitmap")
            syntax::fail(t.begin, "reserved drawing identifier '" + t.text + "'");
        if (!names.insert(t.text).second) syntax::fail(t.begin, "duplicate declaration '" + t.text + "'");
        return t.text;
    };
    auto integer = [&](int lo, int hi) {
        const auto at = r.peek().begin;
        const double value = r.number();
        if (value != std::floor(value) || value < lo || value > hi)
            syntax::fail(at, "integer literal outside expected bounds");
        return static_cast<int>(value);
    };
    while (r.keyword("uint8") || r.keyword("sint16") || r.keyword("uint16")) {
        const Type type = r.takeKeyword("uint8") ? Type::Uint8 :
            r.takeKeyword("uint16") ? Type::Uint16 : Type::Sint16;
        if (type == Type::Sint16) r.needKeyword("sint16");
        const int lo = type == Type::Sint16 ? -32768 : 0;
        const int hi = type == Type::Uint8 ? 255 : type == Type::Uint16 ? 65535 : 32767;
        Global g{name(), type, lo, hi, 0,
                 static_cast<unsigned>(s.values.size())};
        if (r.takeKeyword("Bounds")) {
            r.need('('); g.lo = integer(g.lo, g.hi); r.need(',');
            g.hi = integer(lo, hi); r.need(')');
            if (g.lo > g.hi) r.error("reversed Bounds");
        }
        r.need('='); g.value = integer(g.lo, g.hi);
        fields.emplace(g.name, static_cast<unsigned>(s.globals.size()));
        s.values.push_back(static_cast<unsigned char>(g.value));
        if (g.width() == 2) s.values.push_back(static_cast<unsigned char>(static_cast<unsigned>(g.value) >> 8));
        s.globals.push_back(g);
    }
    r.needKeyword("Sub"); s.entry = name();
    while (!r.keyword("EndSub")) {
        if (r.end()) r.error("expected 'EndSub'");
        const auto token = r.identifier();
        Operation op{syntax::lower(token.text), {}};
        const bool bitmap = op.name == "draw_bitmap";
        unsigned count = op.name == "draw_point" || bitmap ? 3 : op.name == "draw_line" ? 5 : op.name == "fill_quad" ? 9 : 0;
        if (!count) syntax::fail(token.begin, "unsupported resident drawing statement");
        r.need('(');
        for (unsigned i = 0; i < count; ++i) {
            if (i) r.need(',');
            Operand arg;
            if (r.peek().kind == syntax::Kind::Identifier) {
                const auto t = r.identifier();
                const auto found = fields.find(t.text);
                if (found == fields.end()) syntax::fail(t.begin, "undeclared drawing operand '" + t.text + "'");
                arg.field = static_cast<int>(found->second);
                const auto &g = s.globals.at(found->second);
                const Type expected = i ? Type::Sint16 : bitmap ? Type::Uint16 : Type::Uint8;
                if (g.type != expected) syntax::fail(t.begin, i ? "coordinate requires sint16" :
                    bitmap ? "bitmap buffer ID requires uint16" : "colour requires uint8");
                if (i == 0 && bitmap && (g.lo < 1 || g.hi > 65534))
                    syntax::fail(t.begin, "bitmap buffer Bounds must fit 1..65534");
                if (i == 0 && !bitmap && (g.lo < 0 || g.hi > 63)) syntax::fail(t.begin, "colour Bounds must fit 0..63");
            } else arg.value = integer(i ? -32768 : bitmap ? 1 : 0, i ? 32767 : bitmap ? 65534 : 63);
            op.args.push_back(arg);
        }
        r.need(')'); s.operations.push_back(std::move(op));
    }
    r.needKeyword("EndSub");
    if (!r.end()) r.error("unexpected trailing source; drawing profile supports one Sub");
    if (s.operations.empty()) r.error("drawing Sub requires an operation");
    if (s.values.size() > 65535) r.error("packed drawing inputs exceed one block");
    return s;
}
inline Result compile(const std::string &source, const std::vector<resident::Reservation> &reservations = {}) {
    Result r; r.spec = parse(source);
    BufferAllocator allocation;
    for (size_t i = 0; i < reservations.size(); ++i)
        allocation.reserve(reservations[i].first, reservations[i].last, "asset." + std::to_string(i));
    for (const auto &op : r.spec.operations) if (op.name == "draw_bitmap") {
        const auto &arg = op.args.front();
        const unsigned id = static_cast<unsigned>(arg.field < 0 ? arg.value : r.spec.globals.at(arg.field).value);
        if (!allocation.claims().count(id))
            throw AllocationError("bitmap buffer ID " + std::to_string(id) + " requires --reserve before compiler allocation");
        r.initialBitmapIDs.insert(id);
    }
    // The stock transform selector returns without consuming its operands when
    // this feature flag is off. Enable it before installing bitmap templates.
    if (!r.initialBitmapIDs.empty()) r.bootstrap = {23, 0, 248, 1, 0, 1, 0};
    if (!r.spec.values.empty()) {
        r.input = allocation.allocate("drawing.inputs"); r.initial[r.input] = r.spec.values;
    }
    r.entry = allocation.allocate("drawing.entry");
    auto &entry = r.programs[r.entry];
    for (size_t index = 0; index < r.spec.operations.size(); ++index) {
        const auto &op = r.spec.operations[index];
        const unsigned id = allocation.allocate("drawing.template." + std::to_string(index));
        const bool bitmap = op.name == "draw_bitmap";
        auto &bytes = r.programs[id];
        if (bitmap) bytes = {23,27,32,0,0, 23,0,150,1,255,255,
                            23,27,3,0,0,0,0, 23,0,150,1,255,255};
        else bytes = {18, 0, 0};
        auto operand = [&](unsigned argIndex, unsigned offset) {
            const auto &arg = op.args.at(argIndex);
            if (arg.field >= 0) {
                const auto &g = r.spec.globals.at(static_cast<size_t>(arg.field));
                copy(entry, id, offset, r.input, g.offset, g.width());
            } else {
                bytes.at(offset) = static_cast<unsigned char>(arg.value);
                if (argIndex || bitmap) bytes.at(offset + 1) = static_cast<unsigned char>(static_cast<unsigned>(arg.value) >> 8);
            }
        };
        if (bitmap) {
            operand(0, 3); operand(1, 14); operand(2, 16);
            command(entry, id, 1);
            continue;
        }
        operand(0, 2);
        const std::vector<unsigned> order = op.name == "draw_point" ? std::vector<unsigned>{0} :
            op.name == "draw_line" ? std::vector<unsigned>{0, 1} : std::vector<unsigned>{0, 1, 3, 2};
        for (size_t point = 0; point < order.size(); ++point) {
            const auto offset = static_cast<unsigned>(bytes.size());
            const unsigned code = op.name == "draw_point" ? 69 : op.name == "draw_line" ? (point ? 5 : 4) : (point < 2 ? 4 : 85);
            bytes.insert(bytes.end(), {25, static_cast<unsigned char>(code), 0, 0, 0, 0});
            operand(1 + 2 * order[point], offset + 2);
            operand(2 + 2 * order[point], offset + 4);
        }
        command(entry, id, 1);
    }
    for (const auto &claim : allocation.claims()) if (!claim.second.external) {
        const unsigned id = claim.first;
        const auto &bytes = r.programs.count(id) ? r.programs.at(id) : r.initial.at(id);
        r.owned[id] = static_cast<unsigned>(bytes.size()); r.roles[id] = claim.second.owner;
        command(r.bootstrap, id, 2); writeBlock(r.bootstrap, id, bytes);
        command(r.cleanup, id, 2);
    }
    command(r.invoke, r.entry, 1);
    return r;
}
inline Bytes inputUpdate(const Result &r) {
    Bytes out;
    if (r.input) literal(out, r.input, 2, 0, r.spec.values);
    return out;
}
inline std::string manifest(const Result &r, const std::vector<resident::Reservation> &reservations = {}) {
    std::ostringstream out;
    const auto q = resident::quote;
    out << "{\n  \"schema\": 1, \"profile\": \"resident-drawing\",\n  \"input\": ";
    if (r.input) out << "{\"id\":" << r.input << ",\"bytes\":" << r.spec.values.size() << '}';
    else out << "null";
    out << ",\n  \"entry\": {\"name\":" << q(r.spec.entry) << ",\"id\":" << r.entry
        << ",\"blocks\":1,\"bytes\":" << r.programs.at(r.entry).size() << "},\n  \"globals\": [";
    for (size_t i = 0; i < r.spec.globals.size(); ++i) {
        const auto &g = r.spec.globals[i]; if (i) out << ',';
        out << "{\"name\":" << q(g.name) << ",\"id\":" << r.input << ",\"offset\":" << g.offset
            << ",\"bytes\":" << g.width() << ",\"type\":" << q(g.typeName())
            << ",\"min\":" << g.lo << ",\"max\":" << g.hi << ",\"initial\":" << g.value << '}';
    }
    out << "],\n  \"reserved_ranges\": [";
    for (size_t i = 0; i < reservations.size(); ++i) {
        if (i) out << ',';
        out << "{\"first\":" << reservations[i].first << ",\"last\":" << reservations[i].last << '}';
    }
    out << "],\n  \"owned\": [";
    bool first = true;
    for (const auto &v : r.owned) {
        if (!first) out << ',';
        first = false;
        out << "{\"id\":" << v.first << ",\"bytes\":" << v.second << ",\"role\":" << q(r.roles.at(v.first))
            << ",\"kind\":" << q(r.programs.count(v.first) ? "program" : "data") << '}';
    }
    out << "],\n";
    if (!r.initialBitmapIDs.empty()) {
        out << "  \"initial_bitmap_ids\": [";
        bool firstID = true;
        for (unsigned id : r.initialBitmapIDs) { if (!firstID) out << ','; firstID = false; out << id; }
        out << "],\n  \"dynamic_bitmap_ids_require_reserved_loaded_asset\": true,\n";
    }
    out << "  \"serialized_nonreentrant\": true, \"external_input_requires_host_validation\": true,\n"
        << "  \"byte_order\": \"little-endian\", \"loader_root_reserved\": 0, \"special_id_reserved\": 65535,\n"
        << "  \"bootstrap_bytes\":" << r.bootstrap.size() << ",\"input_update_bytes\":" << inputUpdate(r).size()
        << ",\"call_bytes\":" << r.invoke.size() << ",\"cleanup_bytes\":" << r.cleanup.size() << "\n}\n";
    return out.str();
}
} // namespace golem::resident_drawing

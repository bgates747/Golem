#pragma once
// GOLEM-027 fork of the accepted GOLEM-026 shared frontend; bounded uint16 loops.
#include "../../../src/token_reader.hpp"
#include "../../../src/buffer_allocator.hpp"
#include <cstdint>
#include <functional>
#include <cstring>
#include <iomanip>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <vector>

namespace portable {
using golem::syntax::Kind;
using golem::syntax::Position;
using golem::syntax::Reader;
enum class Type { U16, F32 };
inline const char *name(Type t) { return t == Type::U16 ? "uint16" : "float32"; }
inline unsigned width(Type t) { return t == Type::U16 ? 2u : 4u; }
inline unsigned tag(Type t) { return t == Type::U16 ? 1u : 3u; }
struct Value {
    Type type;
    uint32_t bits;
    double number() const {
        if (type == Type::U16) return bits;
        float f;
        std::memcpy(&f, &bits, sizeof(f));
        return f;
    }
};
inline Value floating(float f) {
    static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559,
                  "host reference requires binary32");
    uint32_t bits;
    std::memcpy(&bits, &f, sizeof(bits));
    return {Type::F32, bits};
}
enum Status : unsigned { Ok = 0, AddOverflow = 1, SubUnderflow = 2,
    MultiplyOverflow = 3, BoundsFailure = 32, NonFinite = 33 };
struct Fault { unsigned status; };
enum class Op { Literal, Global, Add, Subtract, Multiply };
struct Expr {
    Type type;
    Op op;
    Position at;
    Value literal{Type::U16, 0};
    size_t global = 0;
    std::shared_ptr<Expr> left, right;
    std::optional<Value> constant;
};
using Expression = std::shared_ptr<Expr>;
struct Global {
    std::string name;
    Type type;
    double low, high;
    Value initial;
};
struct Assignment {
    size_t destination = 0; Expression value; Position at;
    std::string relation;
    Expression left, right;
    std::vector<Assignment> yes, no;
    bool loop = false;
    size_t counter = 0;
    int32_t start = 0, end = 0, step = 1, last = 0;
    uint32_t trips = 0;
};
struct Procedure { std::string name; std::vector<Assignment> statements; uint64_t workVisits = 0; };
struct Program { std::vector<Global> globals; std::vector<Procedure> procedures; };
struct Reservation { unsigned first, last; };
struct Layout {
    unsigned buffer, bytes, errorOffset;
    std::vector<unsigned> offsets;
    std::vector<uint8_t> initial;
    std::vector<Reservation> external;
};

inline Value binary(Op op, Value a, Value b) {
    if (a.type != b.type) throw std::logic_error("invalid typed expression");
    if (a.type == Type::U16) {
        uint32_t result = 0;
        if (op == Op::Add) {
            result = a.bits + b.bits;
            if (result > 65535) throw Fault{AddOverflow};
        } else if (op == Op::Subtract) {
            if (a.bits < b.bits) throw Fault{SubUnderflow};
            result = a.bits - b.bits;
        } else {
            result = a.bits * b.bits; // both operands already uint32_t
            if (result > 65535) throw Fault{MultiplyOverflow};
        }
        return {Type::U16, result};
    }
    const float left = static_cast<float>(a.number());
    const float right = static_cast<float>(b.number());
    const float result = op == Op::Add ? left + right :
                         op == Op::Subtract ? left - right : left * right;
    if (!std::isfinite(result)) throw Fault{NonFinite};
    return floating(result);
}
inline void admit(const Global &global, Value value) {
    if (!std::isfinite(value.number())) throw Fault{NonFinite};
    if (value.number() < global.low || value.number() > global.high)
        throw Fault{BoundsFailure};
}
inline bool reserved(const std::string &word) {
    const auto lower = golem::syntax::lower(word);
    return golem::syntax::reserved(word) || lower == "bounds" || lower == "bool" ||
           lower == "function" || lower == "endfunction" || lower == "return" ||
           lower == "native" || lower == "in" || lower == "runtime_error";
}
class Parser {
    Reader reader_;
    Program program_;
    std::map<std::string, size_t> globals_;
    std::set<std::string> names_;
    std::set<size_t> activeCounters_;
    std::optional<golem::syntax::Token> pendingAssignment_;
    std::string declarationName() {
        const auto token = reader_.identifier();
        if (reserved(token.text)) golem::syntax::fail(token.begin, "reserved declaration name");
        if (!names_.insert(token.text).second)
            golem::syntax::fail(token.begin, "duplicate declaration '" + token.text + "'");
        return token.text;
    }
    Value literal(Type type, double *supplied = nullptr) {
        const auto at = reader_.peek().begin;
        const double input = reader_.number();
        if (supplied) *supplied = input;
        if (type == Type::U16) {
            if (input < 0 || input > 65535 || std::floor(input) != input)
                golem::syntax::fail(at, "literal does not fit uint16");
            return {type, static_cast<uint32_t>(input)};
        }
        const float f = static_cast<float>(input);
        if (!std::isfinite(f) || (input != 0 && f == 0))
            golem::syntax::fail(at, "literal is outside finite nonzero float32 range");
        return floating(f);
    }
    Expression atom(Type type) {
        const auto at = reader_.peek().begin;
        if (reader_.take('(')) {
            auto result = expression(type);
            reader_.need(')');
            return result;
        }
        auto result = std::make_shared<Expr>();
        result->type = type; result->at = at;
        if (reader_.peek().kind == Kind::Number || reader_.peek().text == "-" || reader_.peek().text == "+") {
            result->op = Op::Literal;
            result->literal = literal(type);
            result->constant = result->literal;
        } else {
            const auto token = reader_.identifier();
            auto found = globals_.find(token.text);
            if (found == globals_.end()) golem::syntax::fail(at, "undeclared global '" + token.text + "'");
            if (program_.globals[found->second].type != type)
                golem::syntax::fail(at, "mixed variable types require an explicit conversion (not supported in this slice)");
            result->op = Op::Global; result->global = found->second;
        }
        return result;
    }
    Expression combine(Op op, Expression left, Expression right, Position at) {
        auto result = std::make_shared<Expr>();
        result->type = left->type; result->op = op; result->at = at;
        result->left = std::move(left); result->right = std::move(right);
        if (result->left->constant && result->right->constant) {
            try { result->constant = binary(op, *result->left->constant, *result->right->constant); }
            catch (Fault f) { golem::syntax::fail(at, "invalid constant intermediate, language status " + std::to_string(f.status)); }
        }
        return result;
    }
    Expression product(Type type) {
        auto result = atom(type);
        while (reader_.peek().text == "*") {
            const auto at = reader_.pop().begin;
            result = combine(Op::Multiply, result, atom(type), at);
        }
        return result;
    }
    Expression expression(Type type) {
        auto result = product(type);
        while (reader_.peek().text == "+" || reader_.peek().text == "-") {
            const auto token = reader_.pop();
            result = combine(token.text == "+" ? Op::Add : Op::Subtract, result, product(type), token.begin);
        }
        return result;
    }
    void global() {
        const auto type = reader_.takeKeyword("uint16") ? Type::U16 : Type::F32;
        if (type == Type::F32) reader_.needKeyword("float32");
        const auto name = declarationName();
        const double maximum = type == Type::U16 ? 65535.0 : std::numeric_limits<float>::max();
        const double minimum = type == Type::U16 ? 0.0 : -maximum;
        double low = minimum, high = maximum;
        if (reader_.takeKeyword("Bounds")) {
            reader_.need('('); low = reader_.number(); reader_.need(',');
            high = reader_.number(); reader_.need(')');
            if (low > high || low < minimum || high > maximum ||
                (type == Type::U16 && (std::floor(low) != low || std::floor(high) != high)))
                reader_.error("invalid inclusive Bounds for declared type");
        }
        reader_.need('=');
        // Check original decimal as well as its rounded binary32 value.
        const auto initialAt = reader_.peek().begin;
        double supplied = 0;
        const auto initial = literal(type, &supplied);
        if (supplied < low || supplied > high || initial.number() < low || initial.number() > high)
            golem::syntax::fail(initialAt, "initializer is outside declared Bounds before or after rounding");
        globals_.emplace(name, program_.globals.size());
        program_.globals.push_back({name, type, low, high, initial});
    }
    std::vector<Assignment> statements(unsigned depth) {
        std::vector<Assignment> result;
        while (pendingAssignment_ || (!reader_.end() && !reader_.keyword("EndSub") && !reader_.keyword("Else") &&
               !reader_.keyword("ElseIf") && !reader_.keyword("EndIf") && !reader_.keyword("Next"))) {
            const auto at = pendingAssignment_ ? pendingAssignment_->begin : reader_.peek().begin;
            if (reader_.takeKeyword("For")) {
                result.push_back(loop(depth, at)); continue;
            }
            if (reader_.takeKeyword("If")) {
                if (depth >= 8) golem::syntax::fail(at, "If nesting exceeds eight levels");
                result.push_back(branch(depth + 1, 0, at));
                reader_.needKeyword("EndIf");
                continue;
            }
            if (!pendingAssignment_ && (reader_.peek().kind != Kind::Identifier || reserved(reader_.peek().text)))
                reader_.error("expected global assignment, If or For");
            const auto token = pendingAssignment_ ? *pendingAssignment_ : reader_.identifier();
            pendingAssignment_.reset();
            auto found = globals_.find(token.text);
            if (found == globals_.end()) golem::syntax::fail(at, "undeclared global '" + token.text + "'");
            if (activeCounters_.count(found->second))
                golem::syntax::fail(at, "cannot assign an active loop counter");
            reader_.need('=');
            auto value = expression(program_.globals[found->second].type);
            if (value->constant) {
                try { admit(program_.globals[found->second], *value->constant); }
                catch (Fault) { golem::syntax::fail(at, "constant assignment is outside destination Bounds"); }
            }
            Assignment assignment;
            assignment.destination = found->second; assignment.value = value; assignment.at = at;
            result.push_back(std::move(assignment));
        }
        return result;
    }
    Assignment branch(unsigned depth, unsigned alternatives, Position at) {
        if (alternatives > 64) golem::syntax::fail(at, "ElseIf chain exceeds 64 alternatives");
        Assignment result; result.at = at;
        result.left = expression(Type::U16);
        auto token = reader_.pop();
        result.relation = token.text;
        if (reader_.peek().text == "=" && reader_.peek().begin.offset == token.end.offset)
            result.relation += reader_.pop().text;
        if (result.relation != "==" && result.relation != "!=" && result.relation != "<" &&
            result.relation != ">" && result.relation != "<=" && result.relation != ">=")
            golem::syntax::fail(token.begin, "expected uint16 comparison == != < > <= >=");
        result.right = expression(Type::U16);
        reader_.needKeyword("Then");
        result.yes = statements(depth);
        if (reader_.keyword("ElseIf")) {
            const auto next = reader_.pop().begin;
            result.no.push_back(branch(depth, alternatives + 1, next));
        } else if (reader_.takeKeyword("Else")) result.no = statements(depth);
        return result;
    }
    int32_t rangeInteger() {
        const auto at = reader_.peek().begin;
        const double number = reader_.number();
        if (number < -65535 || number > 65535 || std::floor(number) != number)
            golem::syntax::fail(at, "Range requires integer literals with magnitude at most 65535");
        return static_cast<int32_t>(number);
    }
    Assignment loop(unsigned depth, Position at) {
        Assignment result; result.at = at; result.loop = true;
        const auto token = reader_.identifier();
        const auto found = globals_.find(token.text);
        if (found == globals_.end()) golem::syntax::fail(token.begin, "declare the loop counter before For");
        result.counter = found->second;
        const auto &counter = program_.globals[result.counter];
        if (counter.type != Type::U16) golem::syntax::fail(token.begin, "loop counter must be uint16");
        if (activeCounters_.count(result.counter)) golem::syntax::fail(token.begin, "nested loops require distinct counters");
        if (activeCounters_.size() >= 8) golem::syntax::fail(at, "loop nesting exceeds eight levels");
        reader_.needKeyword("In"); reader_.needKeyword("Range"); reader_.need('(');
        result.start = rangeInteger(); reader_.need(':'); result.end = rangeInteger();
        if (reader_.take(':')) result.step = rangeInteger();
        reader_.need(')');
        if (result.start < 0 || result.end < 0)
            golem::syntax::fail(at, "Range endpoints must fit uint16");
        if (!result.step || (result.start < result.end && result.step < 0) ||
                (result.start > result.end && result.step > 0))
            golem::syntax::fail(at, "Range step is zero or points away from the endpoint");
        const int32_t distance = std::abs(result.end - result.start);
        result.trips = static_cast<uint32_t>(distance / std::abs(result.step)) + 1;
        result.last = result.start + static_cast<int32_t>(result.trips - 1) * result.step;
        if (std::min(result.start, result.last) < counter.low || std::max(result.start, result.last) > counter.high)
            golem::syntax::fail(at, "visited Range values exceed counter Bounds");
        activeCounters_.insert(result.counter);
        result.yes = statements(depth);
        reader_.needKeyword("Next");
        if (reader_.peek().text == token.text) {
            const auto next = reader_.pop();
            // With no terminators, "Next i = ..." is unnamed Next followed by
            // an assignment. Preserve its already-read identifier for the parser.
            if (reader_.peek().text == "=") pendingAssignment_ = next;
        }
        activeCounters_.erase(result.counter);
        return result;
    }
    uint64_t work(const std::vector<Assignment> &body) {
        uint64_t total = 0;
        for (const auto &s : body) {
            total += s.loop ? 1 + uint64_t(s.trips) * (1 + work(s.yes)) :
                     !s.relation.empty() ? 1 + std::max(work(s.yes), work(s.no)) : 1;
            if (total > 262144) golem::syntax::fail(s.at, "entry exceeds 262144 bounded work visits");
        }
        return total;
    }
    void procedure() {
        reader_.needKeyword("Sub");
        Procedure procedure{declarationName(), {}};
        procedure.statements = statements(0);
        reader_.needKeyword("EndSub");
        if (procedure.statements.empty()) reader_.error("empty Sub is not supported");
        procedure.workVisits = work(procedure.statements);
        program_.procedures.push_back(std::move(procedure));
    }

public:
    explicit Parser(const std::string &source) : reader_(source) {}
    Program parse() {
        while (reader_.keyword("uint16") || reader_.keyword("float32")) global();
        if (program_.globals.empty()) reader_.error("expected initialized uint16/float32 globals");
        while (reader_.keyword("Sub")) procedure();
        if (program_.procedures.empty()) reader_.error("expected a finite Sub");
        if (!reader_.end()) reader_.error("unsupported declaration or statement after Subs");
        return program_;
    }
};

inline void put(std::vector<uint8_t> &bytes, unsigned offset, Value value) {
    for (unsigned i = 0; i < width(value.type); ++i) bytes.at(offset + i) = static_cast<uint8_t>(value.bits >> (8 * i));
}
inline Value get(const std::vector<uint8_t> &bytes, unsigned offset, Type type) {
    uint32_t bits = 0;
    for (unsigned i = 0; i < width(type); ++i) bits |= uint32_t(bytes.at(offset + i)) << (8 * i);
    return {type, bits};
}
inline Layout layout(const Program &program, const std::vector<Reservation> &reservations = {}) {
    golem::BufferAllocator allocator;
    for (size_t i = 0; i < reservations.size(); ++i)
        allocator.reserve(reservations[i].first, reservations[i].last, "external." + std::to_string(i));
    Layout result{allocator.allocate("portable.shared_state"), 0, 0, {}, {}, reservations};
    for (const auto &global : program.globals) {
        result.offsets.push_back(result.bytes);
        result.bytes += width(global.type);
    }
    result.errorOffset = result.bytes;
    result.bytes += 2;
    if (result.bytes > 65535) throw std::runtime_error("shared state exceeds one stock WRITE block");
    result.initial.resize(result.bytes);
    for (size_t i = 0; i < program.globals.size(); ++i)
        put(result.initial, result.offsets[i], program.globals[i].initial);
    return result;
}
inline Value evaluate(const Expression &e, const Layout &layout, const std::vector<uint8_t> &state) {
    if (e->op == Op::Literal) return e->literal;
    if (e->op == Op::Global) return get(state, layout.offsets.at(e->global), e->type);
    const auto left = evaluate(e->left, layout, state);
    const auto right = evaluate(e->right, layout, state);
    return binary(e->op, left, right);
}
inline unsigned invokeReference(const Program &program, const Layout &layout,
                                std::vector<uint8_t> &state, size_t entry) {
    if (state.size() != layout.bytes || entry >= program.procedures.size())
        throw std::runtime_error("invalid host reference state or logical entry");
    put(state, layout.errorOffset, {Type::U16, Ok});
    try {
        for (size_t i = 0; i < program.globals.size(); ++i)
            admit(program.globals[i], get(state, layout.offsets[i], program.globals[i].type));
        std::function<void(const std::vector<Assignment>&)> execute = [&](const auto &statements) {
            for (const auto &statement : statements) {
                if (statement.loop) {
                    for (uint32_t n = 0; n < statement.trips; ++n) {
                        const auto value = statement.start + static_cast<int32_t>(n) * statement.step;
                        put(state, layout.offsets[statement.counter], {Type::U16, static_cast<uint32_t>(value)});
                        execute(statement.yes);
                    }
                } else if (!statement.relation.empty()) {
                    auto a = evaluate(statement.left, layout, state).bits;
                    auto b = evaluate(statement.right, layout, state).bits;
                    const auto &op = statement.relation;
                    const bool yes = op == "==" ? a == b : op == "!=" ? a != b : op == "<" ? a < b :
                                     op == ">" ? a > b : op == "<=" ? a <= b : a >= b;
                    execute(yes ? statement.yes : statement.no);
                } else {
                    const auto candidate = evaluate(statement.value, layout, state);
                    admit(program.globals[statement.destination], candidate);
                    put(state, layout.offsets[statement.destination], candidate);
                }
            }
        };
        execute(program.procedures[entry].statements);
        return Ok;
    } catch (Fault fault) {
        put(state, layout.errorOffset, {Type::U16, fault.status});
        return fault.status;
    }
}

inline std::string assembly(const Program &program, const Layout &layout) {
    std::ostringstream out;
    std::set<std::string> symbols;
    auto equ = [&](const std::string &symbol, unsigned value) {
        if (symbol.size() > 64) throw std::runtime_error("assembly symbol exceeds 64-character limit");
        if (!symbols.insert(golem::syntax::lower(symbol)).second)
            throw std::runtime_error("assembly symbol collision: " + symbol);
        out << symbol << ": equ " << value << '\n';
    };
    out << "; Shared layout only. No executable code or callable wrappers are emitted.\n";
    equ("golem_state_buffer", layout.buffer);
    equ("golem_state_bytes", layout.bytes);
    equ("golem_runtime_error_buffer", layout.buffer);
    equ("golem_runtime_error_offset", layout.errorOffset);
    equ("golem_runtime_error_bytes", 2);
    equ("golem_runtime_error_type", 1);
    for (size_t i = 0; i < program.globals.size(); ++i) {
        const auto &g = program.globals[i];
        const auto stem = "golem_" + g.name;
        equ(stem + "_buffer", layout.buffer); equ(stem + "_offset", layout.offsets[i]);
        equ(stem + "_bytes", width(g.type)); equ(stem + "_type", tag(g.type));
    }
    for (size_t i = 0; i < program.procedures.size(); ++i)
        equ("golem_" + program.procedures[i].name + "_entry_id", static_cast<unsigned>(i));
    return out.str();
}
inline std::string interfaceJson(const Program &program, const Layout &layout) {
    std::ostringstream out;
    out << std::setprecision(17)
        << "{\n  \"schema\": 1, \"profile\": \"portable-scalar-foundation\",\n"
        << "  \"experimental\": true, \"execution_contract\": \"host_reference_only\",\n"
        << "  \"state_buffer\": " << layout.buffer << ", \"state_bytes\": " << layout.bytes << ",\n"
        << "  \"state_owner\": \"application_shared\",\n"
        << "  \"initialization\": \"explicit_install_or_reset\",\n"
        << "  \"error\": {\"offset\": " << layout.errorOffset << ", \"bytes\": 2,"
           " \"codes\": {\"ok\": 0, \"uint16_add_overflow\": 1, \"uint16_sub_underflow\": 2,"
           " \"uint16_mul_overflow\": 3, \"bounds_failure\": 32, \"nonfinite_float\": 33}},\n"
        << "  \"globals\": [\n";
    for (size_t i = 0; i < program.globals.size(); ++i) {
        const auto &g = program.globals[i];
        out << "    {\"name\": \"" << g.name << "\", \"type\": \"" << name(g.type)
            << "\", \"offset\": " << layout.offsets[i] << ", \"bytes\": " << width(g.type)
            << ", \"bounds\": [" << g.low << ", " << g.high << "]}"
            << (i + 1 == program.globals.size() ? "\n" : ",\n");
    }
    out << "  ],\n  \"entries\": [\n";
    for (size_t i = 0; i < program.procedures.size(); ++i)
        out << "    {\"name\": \"" << program.procedures[i].name << "\", \"logical_id\": " << i
            << ", \"parameter_bytes\": 0, \"result_bytes\": 0, \"worst_case_visits\": "
            << program.procedures[i].workVisits << "}"
            << (i + 1 == program.procedures.size() ? "\n" : ",\n");
    out << "  ],\n  \"external_reservations\": [";
    for (size_t i = 0; i < layout.external.size(); ++i) {
        if (i) out << ',';
        out << "{\"first\":" << layout.external[i].first << ",\"last\":" << layout.external[i].last << '}';
    }
    out << "]\n}\n";
    return out.str();
}
inline void expressionJson(std::ostream &out, const Expression &e) {
    out << "{\"type\":\"" << name(e->type) << "\",\"line\":" << e->at.line
        << ",\"column\":" << e->at.column;
    if (e->op == Op::Literal) out << ",\"literal_bits\":" << e->literal.bits;
    else if (e->op == Op::Global) out << ",\"global\":" << e->global;
    else {
        out << ",\"operator\":\"" << (e->op == Op::Add ? "+" : e->op == Op::Subtract ? "-" : "*")
            << "\",\"left\":";
        expressionJson(out, e->left); out << ",\"right\":"; expressionJson(out, e->right);
    }
    out << '}';
}
inline void statementsJson(std::ostream &out, const std::vector<Assignment> &statements) {
    out << '[';
    for (size_t j = 0; j < statements.size(); ++j) {
        if (j) out << ',';
        const auto &s = statements[j];
        if (s.loop) {
            out << "{\"for\":{\"counter\":" << s.counter << ",\"start\":" << s.start
                << ",\"end\":" << s.end << ",\"step\":" << s.step << ",\"last\":" << s.last
                << ",\"trips\":" << s.trips << "},\"body\":";
            statementsJson(out, s.yes); out << '}';
        } else if (!s.relation.empty()) {
            out << "{\"if\":{\"operator\":\"" << s.relation << "\",\"left\":";
            expressionJson(out, s.left); out << ",\"right\":"; expressionJson(out, s.right);
            out << "},\"then\":"; statementsJson(out, s.yes);
            out << ",\"else\":"; statementsJson(out, s.no); out << '}';
        } else {
            out << "{\"destination\":" << s.destination << ",\"line\":" << s.at.line
                << ",\"column\":" << s.at.column << ",\"value\":";
            expressionJson(out, s.value); out << '}';
        }
    }
    out << ']';
}
inline std::string programJson(const Program &program) {
    std::ostringstream out;
    out << "{\"schema\":3,\"procedures\":[";
    for (size_t i = 0; i < program.procedures.size(); ++i) {
        if (i) out << ',';
        out << "{\"name\":\"" << program.procedures[i].name << "\",\"assignments\":";
        statementsJson(out, program.procedures[i].statements); out << '}';
    }
    out << "]}\n";
    return out.str();
}
} // namespace portable

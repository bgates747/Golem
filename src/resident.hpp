#pragma once
// Qualified finite resident grammar and ownership/output contract.
#include "ordinary.hpp"
#include "scalar_backend.hpp"
#include <iomanip>
#include <set>

namespace golem::resident {
using ordinary::Compiler;
using ordinary::Reader;
using ordinary::Kind;
using ordinary::compileStatement;
using Result = resident_scalar::Result;
struct Reservation { unsigned first, last; };
inline Result compileResident(const std::string &source, const std::vector<Reservation> &reservations = {}) {
    golem::BufferAllocator allocation;
    for (size_t i = 0; i < reservations.size(); ++i)
        allocation.reserve(reservations[i].first, reservations[i].last, "asset." + std::to_string(i));
    Compiler ordinary(allocation);
    ordinary.addition = Compiler::Addition::Initializer;
    Reader reader(source);
    // Ordinary globals initialize only at installation, not on each invocation.
    while (reader.keyword("uint16")) compileStatement(ordinary, reader, ordinary.out);
    std::set<std::string> names;
    for (const auto &variable : ordinary.variables()) names.insert(variable.first);
    auto name = [&]() {
        const auto token = reader.name();
        if (!names.insert(token.text).second) golem::syntax::fail(token.begin, "duplicate declaration name '" + token.text + "'");
        return token.text;
    };
    resident_scalar::Spec spec;
    reader.needKeyword("float32"); spec.input = name();
    reader.needKeyword("Range"); reader.need('(');
    spec.range.lo = reader.number(); reader.need(','); spec.range.hi = reader.number(); reader.need(')');
    if (reader.take('=')) spec.initial = reader.number();
    reader.needKeyword("sint16"); spec.output = name();
    reader.needKeyword("Sub"); spec.entry = name();
    ordinary.addition = Compiler::Addition::CheckedResident;
    resident_scalar::Extras extras;
    bool floorSeen = false;
    while (!reader.keyword("EndSub")) {
        if (reader.end()) reader.error("expected 'EndSub' to close resident job");
        if (reader.peek().kind == Kind::Identifier && reader.peek().text == spec.output) {
            if (floorSeen) reader.error("resident slice supports exactly one scalar floor assignment");
            reader.pop(); reader.need('='); reader.needKeyword("sint16"); reader.need('(');
            reader.needKeyword("floor"); reader.need('(');
            const auto operand = reader.identifier();
            if (operand.text != spec.input)
                golem::syntax::fail(operand.begin, "floor operand must be declared float32 input '" + spec.input + "'");
            reader.need(')'); reader.need(')'); floorSeen = true;
        } else {
            if (reader.keyword("For") || reader.keyword("Next"))
                reader.error("For/Next is not qualified for this resident profile");
            if (reader.keyword("uint16")) reader.error("declare resident uint16 globals before the float32 input");
            compileStatement(ordinary, reader, floorSeen ? extras.after : extras.before);
        }
    }
    if (!floorSeen) reader.error("resident slice requires one sint16(floor(input)) assignment");
    reader.needKeyword("EndSub");
    if (!reader.end()) reader.error("unexpected trailing source; resident slice supports one job");
    extras.initialize = ordinary.out;
    extras.data = ordinary.dataBuffers();
    extras.globals = ordinary.variables();
    extras.programs = ordinary.programs();
    extras.errorStatus = ordinary.errorStatus();
    return resident_scalar::compile(spec, allocation, std::move(extras));
}
inline double updateValue(const std::string &text) {
    Reader reader(text);
    double value = reader.number();
    if (!reader.end()) reader.error("unexpected text after input value");
    return value;
}
inline std::string quote(const std::string &text) {
    std::string result = "\"";
    const char *hex = "0123456789abcdef";
    for (unsigned char c : text) {
        if (c == '"' || c == '\\') { result += '\\'; result += static_cast<char>(c); }
        else if (c < 32) { result += "\\u00"; result += hex[c >> 4]; result += hex[c & 15]; }
        else result += static_cast<char>(c);
    }
    return result + '"';
}
inline std::string manifest(const Result &result, const std::vector<Reservation> &reservations = {}) {
    std::ostringstream out;
    out << std::setprecision(17)
        << "{\n  \"schema\": 1,\n  \"profile\": \"resident-scalar\",\n"
        << "  \"input\": {\"name\": " << quote(result.spec.input) << ", \"id\": " << result.layout.input
        << ", \"offset\": 0, \"type\": \"float32\", \"bytes\": 4, \"min\": " << result.spec.range.lo
        << ", \"max\": " << result.spec.range.hi << "},\n"
        << "  \"output\": {\"name\": " << quote(result.spec.output) << ", \"id\": " << result.layout.output
        << ", \"offset\": 0, \"type\": \"sint16\", \"bytes\": 2},\n"
        << "  \"entry\": {\"name\": " << quote(result.spec.entry) << ", \"id\": " << result.layout.entry
        << ", \"blocks\": 1, \"bytes\": " << result.programs.at(result.layout.entry).size() << "},\n"
        << "  \"globals\": [";
    bool first = true;
    for (const auto &global : result.extras.globals) {
        if (!first) out << ',';
        first = false;
        out << "{\"name\": " << quote(global.first) << ", \"id\": " << global.second
            << ", \"offset\": 0, \"type\": \"uint16\", \"bytes\": 2}";
    }
    out << "],\n";
    if (result.extras.errorStatus) {
        out << "  \"runtime_error\": {\"id\": " << result.extras.errorStatus
            << ", \"offset\": 0, \"type\": \"uint16\", \"bytes\": 2,"
            << " \"none\": 0, \"uint16_add_overflow\": 1, \"uint16_sub_underflow\": 2,"
            << " \"uint16_mul_overflow\": 3, \"uint16_divide_by_zero\": 4,"
            << " \"reset\": \"each_invocation\","
            << " \"on_error\": \"abort_job_preserve_destination\"},\n";
    }
    out << "  \"reserved_ranges\": [";
    first = true;
    for (const auto &range : reservations) {
        if (!first) out << ',';
        first = false;
        out << "{\"first\": " << range.first << ", \"last\": " << range.last << '}';
    }
    out << "],\n  \"owned\": [";
    first = true;
    for (const auto &owned : result.owned) {
        if (!first) out << ',';
        first = false;
        out << "\n    {\"id\": " << owned.first << ", \"bytes\": " << owned.second
            << ", \"kind\": " << quote(result.programs.count(owned.first) ? "program" : "data")
            << ", \"role\": " << quote(result.roles.at(owned.first)) << '}';
    }
    out << "\n  ],\n  \"byte_order\": \"little-endian\",\n  \"serialized_nonreentrant\": true,\n"
        << "  \"external_input_requires_host_validation\": true,\n"
        << "  \"loader_root_reserved\": 0,\n  \"special_id_reserved\": 65535,\n"
        << "  \"bootstrap_bytes\": " << result.bootstrap.size() << ",\n  \"input_update_bytes\": 15,\n"
        << "  \"call_bytes\": " << result.invoke.size() << ",\n  \"cleanup_bytes\": " << result.cleanup.size() << "\n}\n";
    return out.str();
}
} // namespace golem::resident

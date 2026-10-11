#pragma once
#include "../../../src/token_reader.hpp"
#include "../GOLEM-003/scalar_backend.hpp"
#include <iomanip>
#include <sstream>

namespace termless {
class Parser {
    golem::syntax::Reader reader;
public:
    explicit Parser(const std::string &source) : reader(source) {}
    scalar::Spec run() {
        scalar::Spec spec;
        reader.needKeyword("float32"); spec.input = reader.name().text;
        reader.needKeyword("Range"); reader.need('(');
        spec.range.lo = reader.number(); reader.need(',');
        spec.range.hi = reader.number(); reader.need(')');
        if (reader.take('=')) spec.initial = reader.number();
        reader.needKeyword("sint16"); spec.output = reader.name().text;
        reader.needKeyword("Sub"); spec.entry = reader.name().text;
        if (spec.input == spec.output || spec.input == spec.entry || spec.output == spec.entry)
            reader.error("duplicate declaration name");
        if (reader.identifier().text != spec.output)
            reader.error("assignment must target declared sint16 output");
        reader.need('='); reader.needKeyword("sint16"); reader.need('(');
        reader.needKeyword("floor"); reader.need('(');
        if (reader.identifier().text != spec.input)
            reader.error("floor operand must be declared float32 input");
        reader.need(')'); reader.need(')'); reader.needKeyword("EndSub");
        if (!reader.end()) reader.error("unexpected trailing source; this slice supports one job");
        return spec;
    }
    double updateValue() {
        double value = reader.number();
        if (!reader.end()) reader.error("unexpected text after input value");
        return value;
    }
};
inline scalar::Result compile(const std::string &source) {
    return scalar::compile(Parser(source).run());
}
inline std::string manifest(const scalar::Result &result) {
    std::ostringstream out;
    out << std::setprecision(17)
        << "{\n  \"schema\": 1,\n  \"experimental_task\": \"GOLEM-005\",\n"
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
} // namespace termless

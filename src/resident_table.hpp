#pragma once
// Small finite typed-table profile. Build inputs are bytes, never source filenames.
#include "resident.hpp"

namespace golem::resident_table {
using namespace resident_scalar;
struct Member { std::string name; bool signedValue; unsigned offset; };
struct Spec {
    std::string record, table, output, index, entry;
    std::vector<Member> members;
    unsigned rows = 0, initialIndex = 0;
    Bytes initialOutput, before, after;
    unsigned rowBytes() const { return static_cast<unsigned>(members.size()) * 2; }
};
struct Result {
    Spec spec;
    unsigned base = 0, index = 0, output = 0, status = 0, entry = 0, copier = 0;
    Bytes bootstrap, cleanup, invoke;
    std::map<unsigned, Bytes> initial, programs;
    std::map<unsigned, unsigned> owned;
    std::map<unsigned, std::string> roles;
};
inline Spec parse(const std::string &source) {
    syntax::Reader r(source);
    Spec s;
    std::set<std::string> names;
    auto name = [&]() {
        const auto t = r.name();
        if (!names.insert(t.text).second) syntax::fail(t.begin, "duplicate declaration name '" + t.text + "'");
        return t.text;
    };
    r.needKeyword("Record"); s.record = name();
    std::set<std::string> fields;
    while (!r.keyword("EndRecord")) {
        if (s.members.size() == 32) r.error("table profile supports at most 32 record fields");
        bool signedValue = r.takeKeyword("sint16");
        if (!signedValue) r.needKeyword("uint16");
        auto field = r.name();
        if (!fields.insert(field.text).second) syntax::fail(field.begin, "duplicate record field");
        s.members.push_back({field.text, signedValue, s.rowBytes()});
    }
    if (s.members.empty()) r.error("record must contain at least one field");
    r.needKeyword("EndRecord");
    r.needKeyword("Table");
    if (r.identifier().text != s.record) r.error("table must use the declared record type");
    s.table = name(); r.need('(');
    if (r.uint16() != 0) r.error("table profile requires zero-based inclusive bounds");
    r.need(':'); const auto upper = r.uint16();
    if (upper > 1023) r.error("table profile supports 1..1024 rows");
    s.rows = upper + 1; r.need(')');
    if (r.identifier().text != s.record) r.error("destination must use the declared record type");
    s.output = name(); r.need('=');
    if (r.identifier().text != s.record) r.error("initializer must use the declared record constructor");
    r.need('(');
    for (size_t i = 0; i < s.members.size(); ++i) {
        if (i) r.need(',');
        bool negative = r.take('-');
        unsigned magnitude = r.uint16();
        if (s.members[i].signedValue) {
            if (magnitude > (negative ? 32768u : 32767u)) r.error("record initializer exceeds sint16 bounds");
        } else if (negative) r.error("uint16 record initializer must not be negative");
        le16(s.initialOutput, negative ? (65536u - magnitude) & 65535u : magnitude);
    }
    r.need(')'); r.needKeyword("uint16"); s.index = name(); r.need('='); s.initialIndex = r.uint16();
    r.needKeyword("Sub"); s.entry = name();
    bool selected = false;
    while (!r.keyword("EndSub")) {
        if (r.takeKeyword("print")) {
            if (r.peek().kind != syntax::Kind::String) r.error("table job print requires a literal string");
            const auto text = r.pop();
            // Keep raw text from becoming an accidental VDU command stream.
            for (unsigned char c : text.text)
                if ((c < 32 && c != 10 && c != 13) || c > 126)
                    syntax::fail(text.begin, "table job print supports ASCII text and newlines only");
            auto &out = selected ? s.after : s.before;
            out.insert(out.end(), text.text.begin(), text.text.end());
        } else {
            if (selected) r.error("table profile supports exactly one whole-record selection");
            if (r.identifier().text != s.output) r.error("selection must assign the declared record destination");
            r.need('=');
            if (r.identifier().text != s.table) r.error("selection must use the declared table");
            r.need('(');
            if (r.identifier().text != s.index) r.error("selection must use the declared uint16 index");
            r.need(')'); selected = true;
        }
    }
    if (!selected) r.error("table job requires one whole-record selection");
    r.needKeyword("EndSub");
    if (!r.end()) r.error("unexpected trailing source; table profile supports one job");
    return s;
}
inline Result compile(const std::string &source, const std::string &tableName, const Bytes &data,
                      const std::vector<resident::Reservation> &reservations = {}) {
    Result result; result.spec = parse(source);
    const auto &s = result.spec;
    if (tableName != s.table) throw Error("--table-data name must match declared table '" + s.table + "'");
    if (data.size() != size_t(s.rows) * s.rowBytes())
        throw Error("table data size must equal row count times packed record bytes");
    BufferAllocator allocation;
    for (size_t i = 0; i < reservations.size(); ++i)
        allocation.reserve(reservations[i].first, reservations[i].last, "asset." + std::to_string(i));
    result.base = allocation.allocateRun(s.rows, "table." + s.table + ".row");
    for (unsigned row = 0; row < s.rows; ++row)
        result.initial[result.base + row] = Bytes(data.begin() + row * s.rowBytes(), data.begin() + (row + 1) * s.rowBytes());
    auto storage = [&](const std::string &role, Bytes bytes) {
        unsigned id = allocation.allocate(role); result.initial[id] = std::move(bytes); return id;
    };
    auto word = [&](unsigned value) { Bytes b; le16(b, value); return b; };
    auto program = [&](const std::string &role) {
        unsigned id = allocation.allocate(role); result.programs[id] = {}; return id;
    };
    result.index = storage("input." + s.index, word(s.initialIndex));
    result.output = storage("output." + s.output, s.initialOutput);
    result.status = storage("runtime.error", word(0));
    const auto high = storage("table.index.high", word(0));
    const auto low = storage("table.index.low", word(0));
    const auto limitHigh = storage("table.limit.high", word((s.rows - 1) >> 8));
    const auto limitLow = storage("table.limit.low", word((s.rows - 1) & 255));
    const auto zero = storage("table.zero", word(0));
    const auto valid = storage("table.index.valid", word(0));
    Bytes address = word(result.base); address.push_back(0); // Add-with-carry has a third-byte landing slot.
    const auto sourceID = storage("table.source.id", address);
    result.entry = program("entry." + s.entry);
    const auto accept = program("table.accept");
    const auto compareLow = program("table.compare.low");
    const auto failure = program("table.index.error");
    result.copier = program("table.copy.template");
    literal(result.programs[accept], valid, 2, 0, word(1));
    condition(result.programs[compareLow], accept, 6, low, limitLow);
    literal(result.programs[failure], result.status, 2, 0, word(5));
    copy(result.programs[result.copier], result.output, 0, result.base, 0, s.rowBytes());
    auto &job = result.programs[result.entry];
    literal(job, result.status, 2, 0, word(0));
    job.insert(job.end(), s.before.begin(), s.before.end());
    literal(job, valid, 2, 0, word(0));
    // Compare zero-extended bytes, never a 0xFFFF word (firmware sentinel).
    copy(job, high, 0, result.index, 1, 1);
    copy(job, low, 0, result.index, 0, 1);
    condition(job, accept, 4, high, limitHigh);
    condition(job, compareLow, 2, high, limitHigh);
    ordinary::emitCondJumpBufferValue(job, failure, 2, valid, 0, zero, 0);
    // Admission succeeded. Only now calculate/patch the source buffer ID.
    literal(job, sourceID, 2, 0, address);
    copy(job, sourceID, 0, result.index, 0, 2, 4);
    copy(job, result.copier, 11, sourceID, 0, 2);
    command(job, result.copier, 1);
    job.insert(job.end(), s.after.begin(), s.after.end());
    result.bootstrap = {23,0,248,1,0,1,0};
    for (const auto &claim : allocation.claims()) if (!claim.second.external) {
        unsigned id = claim.first;
        const auto &bytes = result.programs.count(id) ? result.programs.at(id) : result.initial.at(id);
        result.owned[id] = static_cast<unsigned>(bytes.size()); result.roles[id] = claim.second.owner;
        command(result.bootstrap, id, 2); writeBlock(result.bootstrap, id, bytes);
        command(result.cleanup, id, 2);
    }
    command(result.invoke, result.entry, 1);
    return result;
}
inline Bytes inputUpdate(const Result &result, unsigned index) {
    Bytes out, bytes; le16(bytes, index);
    literal(out, result.index, 2, 0, bytes); return out;
}
inline std::string manifest(const Result &r, const std::vector<resident::Reservation> &reservations = {}) {
    std::ostringstream out;
    auto q = resident::quote;
    out << "{\n  \"schema\": 1,\n  \"profile\": \"resident-table\",\n"
        << "  \"input\": {\"name\": " << q(r.spec.index) << ", \"id\": " << r.index
        << ", \"offset\": 0, \"type\": \"uint16\", \"bytes\": 2},\n"
        << "  \"output\": {\"name\": " << q(r.spec.output) << ", \"id\": " << r.output
        << ", \"offset\": 0, \"type\": " << q(r.spec.record) << ", \"bytes\": " << r.spec.rowBytes() << "},\n"
        << "  \"table\": {\"name\": " << q(r.spec.table) << ", \"record_type\": " << q(r.spec.record)
        << ", \"base_id\": " << r.base << ", \"lower\": 0, \"upper\": " << r.spec.rows - 1
        << ", \"rows\": " << r.spec.rows << ", \"row_bytes\": " << r.spec.rowBytes()
        << ", \"immutable\": true, \"fields\": [";
    bool first = true;
    for (const auto &f : r.spec.members) {
        if (!first) out << ',';
        first = false;
        out << "{\"name\": " << q(f.name) << ", \"offset\": " << f.offset
            << ", \"type\": " << q(f.signedValue ? "sint16" : "uint16") << ", \"bytes\": 2}";
    }
    out << "]},\n  \"entry\": {\"name\": " << q(r.spec.entry) << ", \"id\": " << r.entry
        << ", \"blocks\": 1, \"bytes\": " << r.programs.at(r.entry).size() << "},\n"
        << "  \"runtime_error\": {\"id\": " << r.status
        << ", \"offset\": 0, \"type\": \"uint16\", \"bytes\": 2, \"none\": 0, \"table_index_out_of_bounds\": 5,"
        << " \"reset\": \"each_invocation\", \"on_error\": \"abort_job_preserve_destination\"},\n"
        << "  \"reserved_ranges\": [";
    first = true;
    for (const auto &range : reservations) {
        if (!first) out << ',';
        first = false; out << "{\"first\": " << range.first << ", \"last\": " << range.last << '}';
    }
    out << "],\n  \"owned\": ["; first = true;
    for (const auto &v : r.owned) {
        if (!first) out << ',';
        first = false;
        out << "\n    {\"id\": " << v.first << ", \"bytes\": " << v.second << ", \"kind\": "
            << q(r.programs.count(v.first) ? "program" : "data") << ", \"role\": " << q(r.roles.at(v.first)) << '}';
    }
    out << "\n  ],\n  \"byte_order\": \"little-endian\",\n  \"serialized_nonreentrant\": true,\n"
        << "  \"external_input_requires_host_validation\": false,\n"
        << "  \"loader_root_reserved\": 0,\n  \"special_id_reserved\": 65535,\n"
        << "  \"bootstrap_bytes\": " << r.bootstrap.size() << ",\n  \"input_update_bytes\": 13,\n"
        << "  \"call_bytes\": 6,\n  \"cleanup_bytes\": " << r.cleanup.size() << "\n}\n";
    return out.str();
}
} // namespace golem::resident_table

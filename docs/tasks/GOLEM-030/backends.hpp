#pragma once
#include "portable.hpp"
#include "../../../src/ordinary.hpp"
#include <functional>

namespace dual {
using Bytes = std::vector<unsigned char>;
using namespace portable;
using namespace golem::ordinary;
struct Location { unsigned buffer, offset; };
struct Buffered {
    Bytes install, cleanup;
    std::map<unsigned, Bytes> programs, data;
    std::vector<unsigned> entries, owned;
};
inline Bytes call(unsigned id) { Bytes b; emitBufferHeader(b, id); b.push_back(1); return b; }
inline Bytes clear(unsigned id) { Bytes b; emitBufferHeader(b, id); b.push_back(2); return b; }
inline void append(Bytes &a, const Bytes &b) { a.insert(a.end(), b.begin(), b.end()); }
inline Bytes write(unsigned id, const Bytes &data) {
    if (data.empty() || data.size() > 65535) throw std::runtime_error("invalid block length");
    auto b = clear(id); emitWriteBlock(b, id, data); return b;
}
inline void copy(Bytes &code, Location to, Location from, unsigned bytes = 2) {
    emitAdjustMultiBufferFetched(code, to.buffer, kOpSet, to.offset, bytes, from.buffer, from.offset);
}
class BufferEmitter {
    const Program &program_;
    const Layout &layout_;
    golem::BufferAllocator &allocation_;
    Compiler compiler_;
    Buffered output_;
    unsigned compareA_, compareB_, flag_, zero_, setFlag_, boundsExit_;
    unsigned allocate(const std::string &role) {
        const auto id = allocation_.allocate("dual.buffered." + role);
        output_.owned.push_back(id);
        return id;
    }
    unsigned data(const Bytes &bytes, const std::string &role) {
        const auto id = allocate(role); output_.data[id] = bytes; return id;
    }
    unsigned helper(Bytes bytes, const std::string &role) {
        if (bytes.empty() || bytes.size() > 65535) throw std::runtime_error("buffered procedure exceeds WRITE block");
        const auto id = allocate(role); output_.programs[id] = std::move(bytes); return id;
    }
    void bound(Bytes &code, Location value, unsigned bound, bool below) {
        // All compared words contain a single zero-extended byte: no FFFF sentinel.
        emitAdjustMultiLiteral(code, flag_, kOpSet, 0, {0, 0});
        copy(code, {compareA_, 0}, {value.buffer, value.offset + 1}, 1);
        emitAdjustMultiLiteral(code, compareB_, kOpSet, 0, le16Bytes(bound >> 8));
        emitCondCallBufferValue(code, setFlag_, below ? 4 : 5, compareA_, 0, compareB_, 0);
        Bytes low;
        copy(low, {compareA_, 0}, value, 1);
        emitAdjustMultiLiteral(low, compareB_, kOpSet, 0, le16Bytes(bound & 255));
        emitCondJumpBufferValue(low, setFlag_, below ? 4 : 5, compareA_, 0, compareB_, 0);
        const auto lowID = helper(low, "bounds_low");
        emitCondCallBufferValue(code, lowID, 2, compareA_, 0, compareB_, 0);
        emitCondJumpBufferValue(code, boundsExit_, 3, flag_, 0, zero_, 0);
    }
    void admit(Bytes &code, Location value, const Global &global) {
        if (global.low > 0) bound(code, value, static_cast<unsigned>(global.low), true);
        if (global.high < 65535) bound(code, value, static_cast<unsigned>(global.high), false);
    }
    Location expression(Bytes &code, const Expression &expression) {
        if (expression->constant)
            return {data(le16Bytes(expression->constant->bits), "constant"), 0};
        if (expression->op == Op::Global)
            return {layout_.buffer, layout_.offsets[expression->global]};
        if (expression->op == Op::Not || expression->op == Op::And || expression->op == Op::Or) {
            const auto left = this->expression(code, expression->left);
            const auto result = data({0,0}, "logical");
            if (expression->op == Op::Not) {
                copy(code, {compareA_,0}, left);
                emitAdjustMultiLiteral(code, result, kOpSet, 0, {0,0});
                Bytes yes; emitAdjustMultiLiteral(yes, result, kOpSet, 0, {1,0});
                emitCondCallBufferValue(code, helper(yes,"logical_not"), 2, compareA_,0,zero_,0);
            } else {
                copy(code, {result,0}, left);
                Bytes rightCode; const auto right=this->expression(rightCode, expression->right);
                copy(rightCode, {result,0}, right);
                emitCondCallBufferValue(code, helper(rightCode,"logical_rhs"),
                    expression->op == Op::And ? 3 : 2, result,0,zero_,0);
                // Conditional CALL returns after an RHS fault: propagate through
                // every logical frame before destination writes can resume.
                emitCondJumpBufferValue(code, 65535, 3, compiler_.errorStatus(),0,zero_,0);
            }
            return {result,0};
        }
        if (expression->op == Op::Compare) {
            Assignment test; test.relation = expression->relation;
            test.left = expression->left; test.right = expression->right;
            condition(code, test);
            const auto result = data({0, 0}, "boolean");
            copy(code, {result, 0}, {flag_, 0}); return {result, 0};
        }
        const auto left = this->expression(code, expression->left);
        auto right = this->expression(code, expression->right);
        copy(code, {static_cast<unsigned>(compiler_.accumulatorId()), 0}, left);
        if (expression->op == Op::Add) {
            emitAdjustMultiBufferFetched(code, compiler_.accumulatorId(), kOpAddWithCarry, 0, 2,
                                        right.buffer, right.offset);
            compiler_.checkCarry(code);
        } else {
            if (right.offset) {
                const auto temp = data({0, 0}, "operand");
                copy(code, {temp, 0}, right); right = {temp, 0};
            }
            if (expression->op == Op::Subtract)
                compiler_.subtract(code, false, 0, right.buffer);
            else compiler_.multiply(code, compiler_.accumulatorId(), false, 0, right.buffer);
        }
        const auto result = data({0, 0}, "expression");
        copy(code, {result, 0}, {static_cast<unsigned>(compiler_.accumulatorId()), 0});
        return {result, 0};
    }
    static Bytes jump(unsigned id) { Bytes b; emitBufferHeader(b, id); b.push_back(7); return b; }
    void condition(Bytes &code, const Assignment &statement) {
        if (statement.relation == "bool") {
            const auto value = expression(code, statement.left);
            copy(code, {flag_, 0}, value); return;
        }
        const auto left = expression(code, statement.left);
        const auto right = expression(code, statement.right);
        emitAdjustMultiLiteral(code, flag_, kOpSet, 0, {0, 0});
        copy(code, {compareA_, 0}, {left.buffer, left.offset + 1}, 1);
        copy(code, {compareB_, 0}, {right.buffer, right.offset + 1}, 1);
        const auto &op = statement.relation;
        const unsigned lowOp = op == "==" ? 2 : op == "!=" ? 3 : op == "<" ? 4 :
                               op == ">" ? 5 : op == "<=" ? 6 : 7;
        if (op != "==") {
            const unsigned highOp = op == "!=" ? 3 : (op == "<" || op == "<=") ? 4 : 5;
            emitCondCallBufferValue(code, setFlag_, highOp, compareA_, 0, compareB_, 0);
        }
        Bytes low;
        copy(low, {compareA_, 0}, left, 1); copy(low, {compareB_, 0}, right, 1);
        emitCondJumpBufferValue(low, setFlag_, lowOp, compareA_, 0, compareB_, 0);
        const auto lowID = helper(low, "comparison_low");
        emitCondCallBufferValue(code, lowID, 2, compareA_, 0, compareB_, 0);
    }
    unsigned sequence(const std::vector<Assignment> &statements, unsigned continuation) {
        size_t end = statements.size();
        while (end) {
            const auto &last = statements[end - 1];
            Bytes code;
            if (last.loop) {
                const auto latch = allocate("loop_latch");
                const auto body = sequence(last.yes, latch);
                Assignment test; test.relation = "==";
                test.left = std::make_shared<Expr>();
                test.left->type = Type::U16; test.left->op = Op::Global; test.left->global = last.counter;
                test.right = std::make_shared<Expr>();
                test.right->type = Type::U16; test.right->op = Op::Literal;
                test.right->literal = {Type::U16, static_cast<uint32_t>(last.last)};
                test.right->constant = test.right->literal;
                Bytes advance; condition(advance, test);
                emitCondJumpBufferValue(advance, continuation, 3, flag_, 0, zero_, 0);
                // Counter values and increments are proven by constant Range and
                // the prohibition on body writes. Use scratch carry, never the
                // neighbouring packed global, and never advance after last.
                const Location counter{layout_.buffer, layout_.offsets[last.counter]};
                copy(advance, {static_cast<unsigned>(compiler_.accumulatorId()), 0}, counter);
                emitAdjustMultiLiteral(advance, compiler_.accumulatorId(), kOpAddWithCarry, 0,
                                       le16Bytes(static_cast<unsigned>(last.step) & 65535));
                copy(advance, counter, {static_cast<unsigned>(compiler_.accumulatorId()), 0});
                append(advance, jump(body)); output_.programs[latch] = advance;
                emitAdjustMultiLiteral(code, counter.buffer, kOpSet, counter.offset, le16Bytes(last.start));
                append(code, jump(body));
                continuation = helper(code, "loop_start"); --end;
            } else if (!last.relation.empty()) {
                const auto yes = sequence(last.yes, continuation);
                const auto no = sequence(last.no, continuation);
                condition(code, last);
                emitCondJumpBufferValue(code, yes, 3, flag_, 0, zero_, 0);
                append(code, jump(no));
                continuation = helper(code, "branch"); --end;
            } else {
                size_t start = end - 1;
                while (start && !statements[start - 1].loop && statements[start - 1].relation.empty()) --start;
                for (size_t i = start; i < end; ++i) {
                    const auto &statement = statements[i];
                    if(statement.paired) {
                        const auto n=expression(code,statement.left);
                        const auto d=expression(code,statement.right);
                        const auto operands=compiler_.divmodOperands();
                        copy(code,{operands.first,0},n); copy(code,{operands.second,0},d);
                        const auto q=data({0,0},"quotient"), r=data({0,0},"remainder");
                        compiler_.divmod(code,q,r);
                        admit(code,{q,0},program_.globals[statement.destination]);
                        admit(code,{r,0},program_.globals[statement.remainder]);
                        copy(code,{layout_.buffer,layout_.offsets[statement.destination]},{q,0});
                        copy(code,{layout_.buffer,layout_.offsets[statement.remainder]},{r,0});
                        continue;
                    }
                    const auto candidate = expression(code, statement.value);
                    admit(code, candidate, program_.globals[statement.destination]);
                    copy(code, {layout_.buffer, layout_.offsets[statement.destination]}, candidate);
                }
                append(code, jump(continuation));
                continuation = helper(code, "straight_line"); end = start;
            }
        }
        return continuation;
    }

public:
    BufferEmitter(const Program &p, const Layout &l, golem::BufferAllocator &a)
        : program_(p), layout_(l), allocation_(a), compiler_(a) {
        compiler_.ensureAcc(); compiler_.ensureErrorState();
        compareA_ = data({0, 0}, "compare_a"); compareB_ = data({0, 0}, "compare_b");
        flag_ = data({0, 0}, "flag"); zero_ = data({0, 0}, "zero");
        Bytes flag; emitAdjustMultiLiteral(flag, flag_, kOpSet, 0, {1, 0});
        setFlag_ = helper(flag, "set_flag");
        Bytes fail; emitAdjustMultiLiteral(fail, l.buffer, kOpSet, l.errorOffset, {32, 0});
        boundsExit_ = helper(fail, "bounds_exit");
    }
    Buffered emit() {
        const auto finished = helper(jump(65535), "return");
        for (const auto &procedure : program_.procedures) {
            const auto body = sequence(procedure.statements, finished);
            Bytes code;
            emitAdjustMultiLiteral(code, layout_.buffer, kOpSet, layout_.errorOffset, {0, 0});
            emitAdjustMultiLiteral(code, compiler_.errorStatus(), kOpSet, 0, {0, 0});
            for (size_t i = 0; i < program_.globals.size(); ++i)
                admit(code, {layout_.buffer, layout_.offsets[i]}, program_.globals[i]);
            append(code, jump(body));
            const auto entry = helper(code, "entry." + procedure.name);
            output_.entries.push_back(entry);
        }
        for (const auto &item : compiler_.programs()) {
            auto code = item.second;
            // Qualified finite arithmetic exits set private status then return.
            // Publish that status to shared storage before the tail exit returns.
            Bytes prefix; emitBufferHeader(prefix, compiler_.errorStatus()); prefix.push_back(5);
            if (code.size() >= prefix.size() && std::equal(prefix.begin(), prefix.end(), code.begin()))
                copy(code, {layout_.buffer, layout_.errorOffset}, {compiler_.errorStatus(), 0});
            output_.programs[item.first] = code;
        }
        std::set<unsigned> owned(output_.owned.begin(), output_.owned.end());
        for (const auto &claim : allocation_.claims())
            if (!claim.second.external && claim.second.owner.rfind("dual.native.", 0) != 0) owned.insert(claim.first);
        output_.owned.assign(owned.begin(), owned.end());
        for (auto id : output_.owned) append(output_.cleanup, clear(id));
        output_.install = output_.cleanup; append(output_.install, compiler_.out);
        for (const auto &item : output_.data) emitWriteBlock(output_.install, item.first, item.second);
        for (const auto &item : output_.programs) emitWriteBlock(output_.install, item.first, item.second);
        return output_;
    }
};
inline std::string nativeC(const Program &program, const Layout &layout) {
    std::ostringstream out;
    out << "/* Generated from the shared Golem model; module state stays in VDP buffers. */\n"
        << "#include \"vdplet.h\"\nstatic volatile uint32_t identity=0x474c3330u;\n"
        << "static uint16_t get(const uint8_t*p){return (uint16_t)p[0]|(uint16_t)p[1]<<8;}\n"
        << "static void put(uint8_t*p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}\n"
        << "#define FAIL(n) do {put(state+" << layout.errorOffset << ",n);return VDPL_E_ARGUMENT;}while(0)\n"
        << "__attribute__((visibility(\"default\")))\n"
        << "int32_t vdplet_entry(const vdplet_host_v1*h,const vdplet_request_v1*r){\n"
        << "if(!h||!r||h->struct_bytes<sizeof(*h)||r->struct_bytes<sizeof(*r)||h->abi_major!=1||"
           "!(h->capabilities&VDPL_CAP_BUFFERS)||!h->buffer_acquire||!h->buffer_release)return VDPL_E_ABI;\n"
        << "if(r->operation==VDPL_INIT)return identity==0x474c3330u?0:VDPL_E_INIT;\n"
        << "if(r->operation==VDPL_FINI)return 0;\n"
        << "if(r->operation!=VDPL_RUN||r->entry_id>=" << program.procedures.size() << ")return VDPL_E_UNSUPPORTED;\n"
        << "if(r->argument_bytes!=2)return VDPL_E_ARGUMENT;\n"
        << "vdplet_view_v1 args={sizeof(args),0,0,0,0},view={sizeof(view),0,0,0,0};\n"
        << "int32_t rc=h->buffer_acquire(h->context,r->argument_buffer,VDPL_READ,0,2,&args);if(rc)return rc;\n"
        << "uint16_t id=get(args.data);rc=h->buffer_acquire(h->context,id,VDPL_WRITE,0,VDPL_WHOLE_BUFFER,&view);if(rc)return rc;\n"
        << "if(view.bytes!=" << layout.bytes << ")return VDPL_E_BOUNDS;\nuint8_t*state=view.data;put(state+" << layout.errorOffset << ",0);\n";
    auto admit = [&](const Global &global, const std::string &value) {
        if (global.low > 0) out << "if(" << value << '<' << static_cast<unsigned>(global.low) << "u)FAIL(32);\n";
        if (global.high < 65535) out << "if(" << value << '>' << static_cast<unsigned>(global.high) << "u)FAIL(32);\n";
    };
    for (size_t i = 0; i < program.globals.size(); ++i)
        admit(program.globals[i], "get(state+" + std::to_string(layout.offsets[i]) + ")");
    out << "switch(r->entry_id){\n";
    unsigned temporary = 0;
    std::function<std::string(const Expression &)> expression = [&](const Expression &e) -> std::string {
        if (e->constant) return std::to_string(e->constant->bits) + "u";
        if (e->op == Op::Global) {
            const auto result = "v" + std::to_string(temporary++);
            out << "uint32_t " << result << "=get(state+" << layout.offsets[e->global] << ");\n";
            return result;
        }
        const auto left = expression(e->left);
        if (e->op == Op::Not || e->op == Op::And || e->op == Op::Or) {
            const auto result="v"+std::to_string(temporary++);
            if(e->op == Op::Not) out << "uint32_t " << result << "=!" << left << ";\n";
            else {
                out << "uint32_t " << result << "=" << left << ";\nif(" << (e->op==Op::Or ? "!" : "") << result << "){\n";
                const auto right=expression(e->right);
                out << result << "=" << right << ";\n}\n";
            }
            return result;
        }
        const auto right = expression(e->right);
        const auto result = "v" + std::to_string(temporary++);
        if (e->op == Op::Compare) {
            out << "uint32_t " << result << "=(" << left << e->relation << right << ")?1u:0u;\n";
            return result;
        }
        if (e->op == Op::Subtract) out << "if((uint32_t)(" << left << ")<(uint32_t)(" << right << "))FAIL(2);\n";
        out << "uint32_t " << result << "=(uint32_t)(" << left << ")"
            << (e->op == Op::Add ? '+' : e->op == Op::Subtract ? '-' : '*') << "(uint32_t)(" << right << ");\n";
        if (e->op != Op::Subtract) out << "if(" << result << ">65535u)FAIL(" << (e->op == Op::Add ? 1 : 3) << ");\n";
        return result;
    };
    std::function<void(const std::vector<Assignment>&)> statements = [&](const auto &body) {
        for (const auto &statement : body) {
            if (statement.paired) {
                const auto n=expression(statement.left), denominator=expression(statement.right);
                const auto d="v"+std::to_string(temporary++);
                out << "uint32_t " << d << "=" << denominator << ";\n";
                out << "if(!" << d << ")FAIL(4);\n";
                const auto q="v"+std::to_string(temporary++), r="v"+std::to_string(temporary++);
                out << "uint32_t " << q << "=" << n << "/" << d << "," << r << "=" << n << "%" << d << ";\n";
                admit(program.globals[statement.destination],q); admit(program.globals[statement.remainder],r);
                out << "put(state+" << layout.offsets[statement.destination] << "," << q << ");\n"
                    << "put(state+" << layout.offsets[statement.remainder] << "," << r << ");\n";
            } else if (statement.loop) {
                const auto index = "loop" + std::to_string(temporary++);
                out << "for(uint32_t " << index << "=0;" << index << "<" << statement.trips << "u;++" << index << "){\n"
                    << "put(state+" << layout.offsets[statement.counter] << ",(uint16_t)(" << statement.start
                    << "+(int32_t)" << index << "*(" << statement.step << ")));\n";
                statements(statement.yes); out << "}\n";
            } else if (!statement.relation.empty()) {
                const auto left = expression(statement.left);
                out << "if(" << left << "){\n";
                statements(statement.yes); out << "}else{\n";
                statements(statement.no); out << "}\n";
            } else {
                const auto candidate = expression(statement.value);
                admit(program.globals[statement.destination], candidate);
                out << "put(state+" << layout.offsets[statement.destination] << ",(uint16_t)(" << candidate << "));\n";
            }
        }
    };
    for (size_t i = 0; i < program.procedures.size(); ++i) {
        out << "case " << i << ":{\n";
        statements(program.procedures[i].statements);
        out << "break;}\n";
    }
    out << "}\nh->buffer_release(h->context,view.lease);h->buffer_release(h->context,args.lease);return 0;\n}\n";
    return out.str();
}
} // namespace dual

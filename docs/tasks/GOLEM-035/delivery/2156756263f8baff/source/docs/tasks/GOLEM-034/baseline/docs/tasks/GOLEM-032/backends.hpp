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
    static Bytes bitsBytes(uint32_t bits) {
        return {static_cast<unsigned char>(bits),static_cast<unsigned char>(bits>>8),
                static_cast<unsigned char>(bits>>16),static_cast<unsigned char>(bits>>24)};
    }
    void propagate(Bytes &code) {emitCondJumpBufferValue(code,65535,3,compiler_.errorStatus(),0,zero_,0);}
    void compareBytes(Bytes &code,Location left,Location right,const std::string &op,unsigned count=4) {
        emitAdjustMultiLiteral(code,flag_,kOpSet,0,{0,0});
        const unsigned lowOp=op=="=="?2:op=="!="?3:op=="<"?4:op==">"?5:op=="<="?6:7;
        std::function<Bytes(unsigned)> bytes=[&](unsigned byte) {
            Bytes part;copy(part,{compareA_,0},{left.buffer,left.offset+byte},1);
            copy(part,{compareB_,0},{right.buffer,right.offset+byte},1);
            if(!byte) emitCondJumpBufferValue(part,setFlag_,lowOp,compareA_,0,compareB_,0);
            else {
                if(op!="==") emitCondCallBufferValue(part,setFlag_,op=="!="?3:(op=="<"||op=="<=")?4:5,compareA_,0,compareB_,0);
                emitCondCallBufferValue(part,helper(bytes(byte-1),"compare_bytes"),2,compareA_,0,compareB_,0);
            }
            return part;
        };
        append(code,bytes(count-1));
    }
    unsigned floatCheck(Bytes &code,Location value,bool input=false) {
        const auto exponent=data({0,0},"float_exponent"),low=data({0,0},"float_exponent_low");
        copy(code,{exponent,0},{value.buffer,value.offset+3},1);
        emitAdjustMultiLiteral(code,exponent,5,0,{127});
        copy(code,{low,0},{value.buffer,value.offset+2},1);emitAdjustMultiLiteral(code,low,5,0,{128});
        emitAdjustMultiBufferFetched(code,exponent,6,0,2,low,0);
        const auto all=data({255,0},"exponent_all");
        emitCondJumpBufferValue(code,arithmeticExit(33),2,exponent,0,all,0);
        if(input) {
            const auto mantissa=data({0,0},"float_mantissa"),top=data({0,0},"float_mantissa_top");
            copy(code,{mantissa,0},value,1);
            emitAdjustMultiBufferFetched(code,mantissa,6,0,1,value.buffer,value.offset+1);
            copy(code,{top,0},{value.buffer,value.offset+2},1);emitAdjustMultiLiteral(code,top,5,0,{127});
            emitAdjustMultiBufferFetched(code,mantissa,6,0,1,top,0);
            Bytes check;emitCondJumpBufferValue(check,arithmeticExit(33),3,mantissa,0,zero_,0);
            emitCondCallBufferValue(code,helper(check,"subnormal_input"),2,exponent,0,zero_,0);propagate(code);
        }
        Bytes zero;emitAdjustMultiLiteral(zero,value.buffer,kOpSet,value.offset,{0,0,0,0});
        emitCondCallBufferValue(code,helper(zero,"canonical_zero"),2,exponent,0,zero_,0);
        return exponent;
    }
    Location floatOrderValue(Bytes &code,Location value) {
        const auto ordered=data({0,0,0,0},"float_order");copy(code,{ordered,0},value,4);
        const auto negative=sign(code,{value.buffer,value.offset+2});
        Bytes positive,minus;emitAdjustMultiLiteral(positive,ordered,7,3,{128});
        emitAdjustMultiLiteral(minus,ordered,7,0,{255,255,255,255});
        emitCondCallBufferValue(code,helper(positive,"float_order_positive"),2,negative,0,zero_,0);
        emitCondCallBufferValue(code,helper(minus,"float_order_negative"),3,negative,0,zero_,0);
        return {ordered,0};
    }
    void floatLimit(Bytes &code,Location value,float limit,const std::string &op) {
        const auto ordered=floatOrderValue(code,value);
        const auto constant=data(bitsBytes(floatOrder(floating(limit).bits)),"float_limit");
        compareBytes(code,ordered,{constant,0},op);
        emitCondJumpBufferValue(code,boundsExit_,3,flag_,0,zero_,0);
    }
    unsigned loadFloat(Bytes &code,Location value,unsigned rows=1,unsigned columns=1,unsigned format=0) {
        const auto matrix=data(Bytes(rows*columns*4,0),"float_matrix");
        emitBufferHeader(code,matrix);code.insert(code.end(),{34,32,static_cast<unsigned char>(rows),static_cast<unsigned char>(columns),static_cast<unsigned char>(format)});
        auto tail=le16Bytes(value.buffer);append(code,tail);append(code,le16Bytes(value.offset));
        return matrix;
    }
    Location floatConvert(Bytes &code,const Expression &e,Location value) {
        if(e->type==Type::F32 && e->left->type==Type::F32) return value;
        if(e->type==Type::F32) {
            const auto widened=data({0,0,0,0},"integer_widen");
            emitAdjustMultiLiteral(code,widened,kOpSet,0,{0,0,0,0});copy(code,{widened,0},value);
            if(e->left->type==Type::S16) {
                const auto negative=sign(code,value);Bytes extend;emitAdjustMultiLiteral(extend,widened,kOpSet,2,{255,255});
                emitCondCallBufferValue(code,helper(extend,"sign_extend"),3,negative,0,zero_,0);
            }
            return {loadFloat(code,{widened,0},1,1,64),0};
        }
        floatLimit(code,value,e->type==Type::S16?-32769.0f:-1.0f,"<=");
        floatLimit(code,value,e->type==Type::S16?32768.0f:65536.0f,">=");
        const auto negative=sign(code,{value.buffer,value.offset+2});
        const auto absolute=data({0,0,0,0},"float_absolute");copy(code,{absolute,0},value,4);
        emitAdjustMultiLiteral(code,absolute,5,3,{127});
        Bytes raw=bitsBytes(floating(1.0f).bits);append(raw,bitsBytes(0));append(raw,bitsBytes(0));append(raw,bitsBytes(floating(1.0f).bits));
        const auto transformValues=data(raw,"float_integer_transform");copy(code,{transformValues,4},{absolute,0},4);
        const auto transform=loadFloat(code,{transformValues,0},2,2);
        const auto source=data({0,0,0,0},"conversion_source"),converted=data({0,0,0,0},"conversion_result");
        emitBufferHeader(code,converted);code.insert(code.end(),{41,8,64});append(code,le16Bytes(transform));append(code,le16Bytes(source));append(code,le16Bytes(1));
        Bytes negativeResult;negate(negativeResult,converted);
        emitCondCallBufferValue(code,helper(negativeResult,"restore_integer_sign"),3,negative,0,zero_,0);
        return {converted,0};
    }
    void bound(Bytes &code, Location value, unsigned bound, bool below, unsigned failure = 0) {
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
        emitCondJumpBufferValue(code, failure ? failure : boundsExit_, 3, flag_, 0, zero_, 0);
    }
    void admit(Bytes &code, Location value, const Global &global, bool input=false) {
        if(global.type==Type::F32) {
            const auto normalized=data({0,0,0,0},"float_admission");copy(code,{normalized,0},value,4);floatCheck(code,{normalized,0},input);
            if(global.low > -std::numeric_limits<float>::max()) floatLimit(code,{normalized,0},floatBound(global.low,true),"<");
            if(global.high < std::numeric_limits<float>::max()) floatLimit(code,{normalized,0},floatBound(global.high,false),">");
            return;
        }
        if(global.type==Type::S16) {
            const auto ordered=data({0,0},"signed_bounds");copy(code,{ordered,0},value);
            emitAdjustMultiLiteral(code,ordered,7,1,{128});
            if(global.low > -32768) bound(code,{ordered,0},static_cast<unsigned>(global.low+32768),true);
            if(global.high < 32767) bound(code,{ordered,0},static_cast<unsigned>(global.high+32768),false);
            return;
        }
        if (global.low > 0) bound(code, value, static_cast<unsigned>(global.low), true);
        if (global.high < 65535) bound(code, value, static_cast<unsigned>(global.high), false);
    }
    unsigned arithmeticExit(unsigned status) {
        Bytes fail;emitAdjustMultiLiteral(fail,compiler_.errorStatus(),kOpSet,0,le16Bytes(status));
        emitAdjustMultiLiteral(fail,layout_.buffer,kOpSet,layout_.errorOffset,le16Bytes(status));
        return helper(fail,"signed_error");
    }
    unsigned sign(Bytes &code, Location value) {
        const auto result=data({0,0},"sign");copy(code,{result,0},{value.buffer,value.offset+1},1);
        emitAdjustMultiLiteral(code,result,5,0,{128});return result;
    }
    void negate(Bytes &code,unsigned value) {
        emitAdjustMultiLiteral(code,value,7,0,{255,255});
        emitAdjustMultiLiteral(code,value,kOpAddWithCarry,0,{1,0});
    }
    Location signedArithmetic(Bytes &code,const Expression &e,Location left,Location right) {
        const auto a=sign(code,left), b=sign(code,right), result=data({0,0,0},"signed_result");
        const auto status=e->op==Op::Add?1u:e->op==Op::Subtract?2u:3u;
        const auto failure=arithmeticExit(status);
        if(e->op==Op::Multiply) {
            const auto magnitudeA=data({0,0,0},"magnitude"),magnitudeB=data({0,0,0},"magnitude");
            copy(code,{magnitudeA,0},left);copy(code,{magnitudeB,0},right);
            Bytes na,nb;negate(na,magnitudeA);negate(nb,magnitudeB);
            emitCondCallBufferValue(code,helper(na,"absolute"),3,a,0,zero_,0);
            emitCondCallBufferValue(code,helper(nb,"absolute"),3,b,0,zero_,0);
            copy(code,{static_cast<unsigned>(compiler_.accumulatorId()),0},{magnitudeA,0});
            compiler_.multiply(code,compiler_.accumulatorId(),false,0,magnitudeB);
            copy(code,{result,0},{static_cast<unsigned>(compiler_.accumulatorId()),0});
            emitAdjustMultiBufferFetched(code,a,7,0,2,b,0);
            Bytes positive,negative;
            bound(positive,{result,0},32767,false,failure);
            bound(negative,{result,0},32768,false,failure);
            negate(negative,result);
            emitCondCallBufferValue(code,helper(positive,"positive_product"),2,a,0,zero_,0);
            emitCondJumpBufferValue(code,65535,3,compiler_.errorStatus(),0,zero_,0);
            emitCondCallBufferValue(code,helper(negative,"negative_product"),3,a,0,zero_,0);
            emitCondJumpBufferValue(code,65535,3,compiler_.errorStatus(),0,zero_,0);
        } else {
            copy(code,{result,0},left);
            if(e->op==Op::Subtract) {
                const auto complement=data({0,0,0},"signed_subtrahend");copy(code,{complement,0},right);negate(code,complement);
                emitAdjustMultiBufferFetched(code,result,kOpAddWithCarry,0,2,complement,0);
            } else emitAdjustMultiBufferFetched(code,result,kOpAddWithCarry,0,2,right.buffer,right.offset);
            const auto changed=sign(code,{result,0});
            emitAdjustMultiBufferFetched(code,changed,7,0,2,a,0);
            emitAdjustMultiBufferFetched(code,a,7,0,2,b,0);
            if(e->op==Op::Add) emitAdjustMultiLiteral(code,a,7,0,{128,0});
            emitAdjustMultiBufferFetched(code,changed,5,0,2,a,0);
            emitCondJumpBufferValue(code,failure,3,changed,0,zero_,0);
        }
        return {result,0};
    }
    Location expression(Bytes &code, const Expression &expression) {
        if (expression->constant)
            return {data(expression->type==Type::F32?bitsBytes(expression->constant->bits):le16Bytes(expression->constant->bits), "constant"), 0};
        if (expression->op == Op::Global) {
            Location value{layout_.buffer, layout_.offsets[expression->global]};
            if(expression->type!=Type::F32) return value;
            const auto normalized=data({0,0,0,0},"float_read");copy(code,{normalized,0},value,4);floatCheck(code,{normalized,0});return {normalized,0};
        }
        if(expression->op==Op::Convert) {
            const auto value=this->expression(code,expression->left);
            if(expression->type==Type::F32 || expression->left->type==Type::F32) return floatConvert(code,expression,value);
            if(expression->type!=expression->left->type) bound(code,value,32767,false);
            return value;
        }
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
        if(expression->type==Type::F32) {
            const auto a=loadFloat(code,left),b=expression->op==Op::Multiply?0:loadFloat(code,right);
            const auto result=data({0,0,0,0},"float_result");emitBufferHeader(code,result);
            code.insert(code.end(),{34,static_cast<unsigned char>(expression->op==Op::Add?4:expression->op==Op::Subtract?5:39),1,1});append(code,le16Bytes(a));
            if(expression->op==Op::Multiply) {code.push_back(0);append(code,le16Bytes(right.buffer));append(code,le16Bytes(right.offset));}
            else append(code,le16Bytes(b));
            floatCheck(code,{result,0});return {result,0};
        }
        if(expression->type==Type::S16) return signedArithmetic(code,expression,left,right);
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
        auto left = expression(code, statement.left);
        auto right = expression(code, statement.right);
        if(statement.left->type==Type::F32) {
            left=floatOrderValue(code,left);right=floatOrderValue(code,right);
            compareBytes(code,left,right,statement.relation);return;
        }
        if(statement.left->type==Type::S16) {
            const auto a=data({0,0},"signed_order"),b=data({0,0},"signed_order");
            copy(code,{a,0},left);copy(code,{b,0},right);
            emitAdjustMultiLiteral(code,a,7,1,{128});emitAdjustMultiLiteral(code,b,7,1,{128});
            left={a,0};right={b,0};
        }
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
                    copy(code, {layout_.buffer, layout_.offsets[statement.destination]}, candidate,width(program_.globals[statement.destination].type));
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
        Bytes fail; emitAdjustMultiLiteral(fail,compiler_.errorStatus(),kOpSet,0,{32,0});
        emitAdjustMultiLiteral(fail, l.buffer, kOpSet, l.errorOffset, {32, 0});
        boundsExit_ = helper(fail, "bounds_exit");
    }
    Buffered emit() {
        output_.install={23,0,248,1,0,1,0};
        const auto finished = helper(jump(65535), "return");
        Bytes admission;
        for(size_t i=0;i<program_.globals.size();++i) admit(admission,{layout_.buffer,layout_.offsets[i]},program_.globals[i],true);
        const auto admissionID=admission.empty()?0:helper(admission,"entry_admission");
        for (const auto &procedure : program_.procedures) {
            const auto body = sequence(procedure.statements, finished);
            Bytes code;
            emitAdjustMultiLiteral(code, layout_.buffer, kOpSet, layout_.errorOffset, {0, 0});
            emitAdjustMultiLiteral(code, compiler_.errorStatus(), kOpSet, 0, {0, 0});
            if(admissionID) {append(code,call(admissionID));propagate(code);}
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
        append(output_.install, output_.cleanup); append(output_.install, compiler_.out);
        for (const auto &item : output_.data) emitWriteBlock(output_.install, item.first, item.second);
        for (const auto &item : output_.programs) emitWriteBlock(output_.install, item.first, item.second);
        return output_;
    }
};
inline std::string nativeC(const Program &program, const Layout &layout) {
    std::ostringstream out;
    out << "/* Generated from the shared Golem model; module state stays in VDP buffers. */\n"
        << "#include \"vdplet.h\"\nstatic volatile uint32_t identity=0x474c3332u;\n"
        << "static uint16_t get(const uint8_t*p){return (uint16_t)p[0]|(uint16_t)p[1]<<8;}\n"
        << "static void put(uint8_t*p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}\n"
        << "static inline int32_t signed16(uint32_t v){return (int32_t)v-((v&32768u)?65536:0); }\n"
        << "#define FAIL(n) do {put(state+" << layout.errorOffset << ",n);return VDPL_E_ARGUMENT;}while(0)\n"
        << "static inline uint32_t get32(const uint8_t*p){return (uint32_t)get(p)|(uint32_t)get(p+2)<<16;}\n"
        << "static inline void put32(uint8_t*p,uint32_t v){put(p,(uint16_t)v);put(p+2,(uint16_t)(v>>16));}\n"
        << "static inline float fvalue(uint32_t u){union{uint32_t u;float f;}v={.u=u};return v.f;}\n"
        << "static inline uint32_t fbits(float f){union{uint32_t u;float f;}v={.f=f};return v.u;}\n"
        << "static inline uint32_t ordered(uint32_t v){return (v&0x80000000u)?~v:(v^0x80000000u);}\n"
        << "#define FP_RESULT(v) do{if(((v)&0x7f800000u)==0x7f800000u)FAIL(33);if(!((v)&0x7f800000u))(v)=0;}while(0)\n"
        << "#define FP_INPUT(v) do{if(!((v)&0x7f800000u)&&((v)&0x007fffffu))FAIL(33);FP_RESULT(v);}while(0)\n"
        << "__attribute__((visibility(\"default\")))\n"
        << "int32_t vdplet_entry(const vdplet_host_v1*h,const vdplet_request_v1*r){\n"
        << "if(!h||!r||h->struct_bytes<sizeof(*h)||r->struct_bytes<sizeof(*r)||h->abi_major!=1||"
           "!(h->capabilities&VDPL_CAP_BUFFERS)||!h->buffer_acquire||!h->buffer_release)return VDPL_E_ABI;\n"
        << "if(r->operation==VDPL_INIT)return identity==0x474c3332u?0:VDPL_E_INIT;\n"
        << "if(r->operation==VDPL_FINI)return 0;\n"
        << "if(r->operation!=VDPL_RUN||r->entry_id>=" << program.procedures.size() << ")return VDPL_E_UNSUPPORTED;\n"
        << "if(r->argument_bytes!=2)return VDPL_E_ARGUMENT;\n"
        << "vdplet_view_v1 args={sizeof(args),0,0,0,0},view={sizeof(view),0,0,0,0};\n"
        << "int32_t rc=h->buffer_acquire(h->context,r->argument_buffer,VDPL_READ,0,2,&args);if(rc)return rc;\n"
        << "uint16_t id=get(args.data);rc=h->buffer_acquire(h->context,id,VDPL_WRITE,0,VDPL_WHOLE_BUFFER,&view);if(rc)return rc;\n"
        << "if(view.bytes!=" << layout.bytes << ")return VDPL_E_BOUNDS;\nuint8_t*state=view.data;put(state+" << layout.errorOffset << ",0);\n";
    unsigned admissionTemporary=0;
    auto admit = [&](const Global &global, const std::string &value,bool input=false) {
        if(global.type==Type::F32) {
            const auto v="admission"+std::to_string(admissionTemporary++);
            out << "uint32_t " << v << "=" << value << ";\n" << (input?"FP_INPUT(":"FP_RESULT(") << v << ");\n";
            if(global.low > -std::numeric_limits<float>::max()) out << "if(ordered(" << v << ")<" << floatOrder(floating(floatBound(global.low,true)).bits) << "u)FAIL(32);\n";
            if(global.high < std::numeric_limits<float>::max()) out << "if(ordered(" << v << ")>" << floatOrder(floating(floatBound(global.high,false)).bits) << "u)FAIL(32);\n";
            return;
        }
        if(global.type==Type::S16) {
            if(global.low > -32768) out << "if(signed16(" << value << ")<" << static_cast<int>(global.low) << ")FAIL(32);\n";
            if(global.high < 32767) out << "if(signed16(" << value << ")>" << static_cast<int>(global.high) << ")FAIL(32);\n";
            return;
        }
        if (global.low > 0) out << "if(" << value << '<' << static_cast<unsigned>(global.low) << "u)FAIL(32);\n";
        if (global.high < 65535) out << "if(" << value << '>' << static_cast<unsigned>(global.high) << "u)FAIL(32);\n";
    };
    for (size_t i = 0; i < program.globals.size(); ++i)
        admit(program.globals[i], std::string(program.globals[i].type==Type::F32?"get32(state+":"get(state+") + std::to_string(layout.offsets[i]) + ")",true);
    out << "switch(r->entry_id){\n";
    unsigned temporary = 0;
    std::function<std::string(const Expression &)> expression = [&](const Expression &e) -> std::string {
        if (e->constant) return std::to_string(e->constant->bits) + "u";
        if (e->op == Op::Global) {
            const auto result = "v" + std::to_string(temporary++);
            out << "uint32_t " << result << "=" << (e->type==Type::F32?"get32":"get") << "(state+" << layout.offsets[e->global] << ");\n";
            if(e->type==Type::F32) out << "FP_RESULT(" << result << ");\n";
            return result;
        }
        const auto left = expression(e->left);
        if(e->op==Op::Convert) {
            if(e->type==Type::F32 && e->left->type!=Type::F32) {
                const auto result="v"+std::to_string(temporary++);
                out << "uint32_t " << result << "=fbits((float)" << (e->left->type==Type::S16?"signed16("+left+")":left) << ");\n";
                return result;
            }
            if(e->left->type==Type::F32 && e->type!=Type::F32) {
                const auto result="v"+std::to_string(temporary++);
                out << "float f" << result << "=fvalue(" << left << ");\n"
                    << "if(!(f" << result << ">" << (e->type==Type::S16?"-32769.0f":"-1.0f") << "&&f" << result << "<" << (e->type==Type::S16?"32768.0f":"65536.0f") << "))FAIL(32);\n"
                    << "uint32_t " << result << "=(uint32_t)(int32_t)f" << result << "&65535u;\n";
                return result;
            }
            if(e->type!=e->left->type) out << "if(" << left << ">32767u)FAIL(32);\n";
            return left;
        }
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
            if(e->left->type==Type::F32) {
                out << "uint32_t " << result << "=(ordered(" << left << ")" << e->relation << "ordered(" << right << "))?1u:0u;\n";
                return result;
            }
            out << "uint32_t " << result << "=(" << (e->left->type==Type::S16?"signed16("+left+")":left)
                << e->relation << (e->right->type==Type::S16?"signed16("+right+")":right) << ")?1u:0u;\n";
            return result;
        }
        if(e->type==Type::F32) {
            out << "uint32_t " << result << "=fbits(fvalue(" << left << ")" << (e->op==Op::Add?'+':e->op==Op::Subtract?'-':'*') << "fvalue(" << right << "));\nFP_RESULT(" << result << ");\n";
            return result;
        }
        if(e->type==Type::S16) {
            out << "int32_t s" << result << "=signed16(" << left << ")" << (e->op==Op::Add?'+':e->op==Op::Subtract?'-':'*') << "signed16(" << right << ");\n"
                << "if(s" << result << " < -32768 || s" << result << " >32767)FAIL(" << (e->op==Op::Add?1: e->op==Op::Subtract?2:3) << ");\n"
                << "uint32_t " << result << "=(uint32_t)s" << result << "&65535u;\n";
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
                out << (program.globals[statement.destination].type==Type::F32?"put32":"put") << "(state+" << layout.offsets[statement.destination] << "," << candidate << ");\n";
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

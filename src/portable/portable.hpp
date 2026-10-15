#pragma once
// Promoted from the accepted portable frontend at b32c596; preserve qualified semantics.
#include "../token_reader.hpp"
#include "../buffer_allocator.hpp"
#include <cstdint>
#include <cmath>
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
enum class Type { U16, S16, F32, Bool, U32, S32 };
inline bool wide(Type t){return t==Type::U32 || t==Type::S32;}
inline bool signedInteger(Type t){return t==Type::S16 || t==Type::S32;}
inline bool integer(Type t){return t!=Type::F32 && t!=Type::Bool;}
inline const char *name(Type t) {
    switch(t){case Type::U16:return "uint16";case Type::S16:return "sint16";case Type::F32:return "float32";
              case Type::Bool:return "bool";case Type::U32:return "uint32";case Type::S32:return "sint32";}
    throw std::logic_error("invalid type");
}
inline unsigned width(Type t){return t==Type::F32 || wide(t)?4u:2u;}
inline unsigned tag(Type t){return t==Type::U32?7u:t==Type::S32?8u:t==Type::S16?2u:t==Type::Bool?6u:t==Type::U16?1u:3u;}
inline int64_t lowInteger(Type t){return t==Type::S32?-2147483648ll:t==Type::S16?-32768:0;}
inline int64_t highInteger(Type t){return t==Type::S32?2147483647ll:t==Type::U32?4294967295ll:t==Type::S16?32767:65535;}
struct Value {
    Type type;
    uint32_t bits;
    double number() const {
        if (type == Type::S16) return bits < 32768 ? static_cast<int32_t>(bits) : static_cast<int32_t>(bits)-65536;
        if(type==Type::S32)return bits<0x80000000u?static_cast<int64_t>(bits):static_cast<int64_t>(bits)-4294967296ll;
        if (type != Type::F32) return bits;
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
inline Value normalize(Value v, bool input=false) {
    if(v.type!=Type::F32) return v;
    const auto magnitude=v.bits&0x7fffffffu;
    if(magnitude>=0x7f800000u || (input && magnitude && magnitude<0x00800000u)) throw Fault{33};
    if(magnitude<0x00800000u) v.bits=0;
    return v;
}
inline Value convert(Value v,Type target) {
    v=normalize(v);if(target==v.type)return v;
    if(integer(v.type) && integer(target)) {
        const auto number=static_cast<int64_t>(v.number());
        if(number<lowInteger(target)||number>highInteger(target))throw Fault{32};
        return {target,static_cast<uint32_t>(number)&(width(target)==2?65535u:UINT32_MAX)};
    }
    if(wide(v.type)||wide(target))throw std::logic_error("wide/float conversions outside this tranche");
    if(target==Type::F32)return floating(static_cast<float>(v.number()));
    const auto value=std::trunc(v.number());
    if(value<lowInteger(target)||value>highInteger(target))throw Fault{32};
    return {target,static_cast<uint32_t>(static_cast<int32_t>(value))&65535u};
}
inline uint32_t floatOrder(uint32_t bits) {return bits&0x80000000u?~bits:bits^0x80000000u;}
inline float floatBound(double bound,bool lower) {
    auto value=static_cast<float>(bound);
    if(lower && static_cast<double>(value)<bound) value=std::nextafter(value,std::numeric_limits<float>::infinity());
    if(!lower && static_cast<double>(value)>bound) value=std::nextafter(value,-std::numeric_limits<float>::infinity());
    return value;
}
enum class Op { Literal, Global, ArrayRead, Add, Subtract, Multiply, Convert, Compare, Not, And, Or, ShiftRight, Xor, Divide, Sin, Cos, Floor };
struct Expr {
    Type type;
    Op op;
    Position at;
    Value literal{Type::U16, 0};
    size_t global = 0;
    std::shared_ptr<Expr> left, right;
    std::optional<Value> constant;
    std::string relation;
};
using Expression = std::shared_ptr<Expr>;
struct Global {
    std::string name;
    Type type;
    double low, high;
    Value initial;
};
struct Array {
    std::string name;
    Type type;
    int32_t lower=0, upper=0;
    bool readOnly=false;
    std::vector<uint8_t> initial;
    unsigned count() const {return static_cast<unsigned>(upper-lower+1);}
    unsigned bytes() const {return count()*width(type);}
};
struct Assignment {
    size_t destination = 0; Expression value; Position at;
    Expression index;
    std::string relation;
    Expression left, right;
    std::vector<Assignment> yes, no;
    bool paired = false;
    size_t remainder = 0;
    bool loop = false;
    size_t counter = 0;
    int32_t start = 0, end = 0, step = 1, last = 0;
    uint32_t trips = 0;
};
struct Procedure { std::string name; std::vector<Assignment> statements; uint64_t workVisits = 0; };
struct Program { std::vector<Global> globals; std::vector<Procedure> procedures; std::vector<Array> arrays; };
inline bool mathOperation(Op op) { return op==Op::Divide || op==Op::Sin || op==Op::Cos || op==Op::Floor; }
inline bool requiresNativeMath(const Expression &e) {
    return e && (mathOperation(e->op) || requiresNativeMath(e->left) || requiresNativeMath(e->right));
}
inline bool requiresNativeMath(const std::vector<Assignment> &body) {
    for(const auto&s:body)
        if(requiresNativeMath(s.value)||requiresNativeMath(s.index)||requiresNativeMath(s.left)||requiresNativeMath(s.right)||
           requiresNativeMath(s.yes)||requiresNativeMath(s.no))return true;
    return false;
}
inline bool requiresNativeMath(const Program &p) {
    for(const auto&s:p.procedures)if(requiresNativeMath(s.statements))return true;
    return false;
}
inline Value mathUnary(Op op,Value value) {
    value=normalize(value);
    if(value.type!=Type::F32)throw std::logic_error("float math requires float32");
    const auto x=static_cast<float>(value.number());
    return normalize(floating(op==Op::Sin?std::sin(x):op==Op::Cos?std::cos(x):std::floor(x)));
}
struct Reservation { unsigned first, last; };
struct Layout {
    unsigned buffer, bytes, errorOffset;
    std::vector<unsigned> offsets;
    std::vector<uint8_t> initial;
    std::vector<Reservation> external;
    std::vector<unsigned> arrayBuffers;
};

inline Value binary(Op op, Value a, Value b) {
    if(op==Op::ShiftRight) {
        if(!wide(a.type)||b.type!=Type::U16)throw std::logic_error("invalid shift operand types");
        if(b.bits>31)throw Fault{32};
        uint32_t result=a.bits>>b.bits;
        if(a.type==Type::S32 && (a.bits&0x80000000u) && b.bits)result|=0xffffffffu<<(32-b.bits);
        return {a.type,result};
    }
    if (a.type != b.type) throw std::logic_error("invalid typed expression");
    if(op==Op::Divide) {
        if(a.type!=Type::F32)throw std::logic_error("division requires float32");
        if(b.number()==0)throw Fault{4};
        return normalize(floating(static_cast<float>(a.number())/static_cast<float>(b.number())));
    }
    if(op==Op::Xor) {
        if(!integer(a.type))throw std::logic_error("XOR requires integer operands");
        return {a.type,(a.bits^b.bits)&(width(a.type)==2?65535u:UINT32_MAX)};
    }
    if(wide(a.type)) {
        const auto left=static_cast<int64_t>(a.number()),right=static_cast<int64_t>(b.number());
        if(op==Op::Multiply && a.type==Type::U32) {
            const uint64_t result=static_cast<uint64_t>(a.bits)*b.bits;
            if(result>4294967295ull)throw Fault{3};
            return {a.type,static_cast<uint32_t>(result)};
        }
        const auto result=op==Op::Add?left+right:op==Op::Subtract?left-right:left*right;
        if(result<lowInteger(a.type)||result>highInteger(a.type))throw Fault{op==Op::Add?1u:op==Op::Subtract?2u:3u};
        return {a.type,static_cast<uint32_t>(result)};
    }
    if (a.type == Type::S16) {
        const int32_t left=static_cast<int32_t>(a.number()),right=static_cast<int32_t>(b.number());
        const auto result=op==Op::Add?left+right:op==Op::Subtract?left-right:left*right;
        if(result < -32768 || result > 32767) throw Fault{op==Op::Add?1u:op==Op::Subtract?2u:3u};
        return {Type::S16,static_cast<uint32_t>(result)&65535u};
    }
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
    return normalize(floating(result));
}
inline void admit(const Global &global, Value value) {
    value=normalize(value,true);
    if (!std::isfinite(value.number())) throw Fault{NonFinite};
    if (value.number() < global.low || value.number() > global.high)
        throw Fault{BoundsFailure};
}
inline bool reserved(const std::string &word) {
    const auto lower = golem::syntax::lower(word);
    return golem::syntax::reserved(word) || lower == "bounds" || lower == "bool" ||
           lower == "function" || lower == "endfunction" || lower == "return" ||
           lower == "sin" || lower == "cos" || lower == "floor" ||
           lower == "uint32" || lower == "sint32" || lower == "array" || lower == "readonly" || lower == "not" || lower == "and" || lower == "or" || lower == "true" || lower == "false" || lower == "native" || lower == "in" || lower == "runtime_error";
}
class Parser {
    Reader reader_;
    Program program_;
    std::map<std::string, size_t> globals_;
    std::map<std::string, size_t> arrays_;
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
        if (type == Type::Bool) {
            if (!reader_.keyword("true") && !reader_.keyword("false"))
                reader_.error("bool initializer must be true or false");
            const auto bits = reader_.takeKeyword("true") ? 1u : (reader_.needKeyword("false"), 0u);
            if (supplied) *supplied = bits;
            return {Type::Bool, bits};
        }
        const double input = reader_.number();
        if (supplied) *supplied = input;
        if(integer(type)) {
            if(input<lowInteger(type)||input>highInteger(type)||std::floor(input)!=input)
                golem::syntax::fail(at,std::string("literal does not fit ")+name(type));
            return {type,static_cast<uint32_t>(static_cast<int64_t>(input))&(width(type)==2?65535u:UINT32_MAX)};
        }
        const float f = static_cast<float>(input);
        if (!std::isfinite(f) || (input != 0 && f == 0))
            golem::syntax::fail(at, "literal is outside finite nonzero float32 range");
        try { return normalize(floating(f),true); }
        catch(Fault) {golem::syntax::fail(at,"subnormal float32 literals are outside the portable input domain");}
    }
    Expression atom(Type type) {
        const auto at = reader_.peek().begin;
        if(reader_.keyword("sin")||reader_.keyword("cos")||reader_.keyword("floor")) {
            if(type!=Type::F32)golem::syntax::fail(at,"sin/cos/floor require float32 operands and result; convert explicitly");
            const auto op=reader_.takeKeyword("sin")?Op::Sin:reader_.takeKeyword("cos")?Op::Cos:(reader_.needKeyword("floor"),Op::Floor);
            reader_.need('(');auto argument=expression(Type::F32);reader_.need(')');
            auto result=std::make_shared<Expr>();result->type=Type::F32;result->op=op;result->at=at;result->left=argument;
            return result;
        }
        if(reader_.keyword("uint16")||reader_.keyword("sint16")||reader_.keyword("uint32")||reader_.keyword("sint32")||reader_.keyword("float32")) {
            const auto target=reader_.takeKeyword("sint16")?Type::S16:reader_.takeKeyword("uint16")?Type::U16:
                reader_.takeKeyword("uint32")?Type::U32:reader_.takeKeyword("sint32")?Type::S32:(reader_.needKeyword("float32"),Type::F32);
            if(target!=type)reader_.error("conversion result requires matching destination/expression type");
            reader_.need('(');const auto saved=reader_;Expression argument;
            for(auto candidate:{Type::U16,Type::S16,Type::U32,Type::S32,Type::F32}) {
                reader_=saved;
                try {argument=expression(candidate);reader_.need(')');break;}
                catch(const golem::syntax::Error &) {argument.reset();}
            }
            if(!argument)reader_.error("invalid typed conversion argument");
            if((wide(target)&&argument->type==Type::F32)||(target==Type::F32&&wide(argument->type)))
                golem::syntax::fail(at,"wide integer/float conversions are deferred");
            auto result=std::make_shared<Expr>();result->type=target;result->op=Op::Convert;result->left=argument;
            return result;
        }
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
            auto array = arrays_.find(token.text);
            if(array!=arrays_.end()) {
                if(program_.arrays[array->second].type!=type) golem::syntax::fail(at,"array element type requires an explicit conversion");
                result->op=Op::ArrayRead;result->global=array->second;
                result->left=arrayIndex(array->second);return result;
            }
            auto found = globals_.find(token.text);
            if (found == globals_.end()) golem::syntax::fail(at, "undeclared global '" + token.text + "'");
            if (program_.globals[found->second].type != type)
                golem::syntax::fail(at, "mixed variable types require an explicit conversion (not supported in this slice)");
            result->op = Op::Global; result->global = found->second;
        }
        return result;
    }
    Expression arrayIndex(size_t array) {
        reader_.need('(');auto saved=reader_;Expression index;
        try {index=expression(Type::U16);reader_.need(')');}
        catch(const golem::syntax::Error &) {reader_=saved;index=expression(Type::S16);reader_.need(')');}
        const auto &a=program_.arrays[array];
        if(index->constant && (index->constant->number()<a.lower || index->constant->number()>a.upper))
            golem::syntax::fail(index->at,"constant array index is outside inclusive bounds");
        return index;
    }
    void array() {
        const bool immutable=reader_.takeKeyword("ReadOnly");reader_.needKeyword("Array");
        Type type;
        if(reader_.takeKeyword("uint16"))type=Type::U16;
        else if(reader_.takeKeyword("sint16"))type=Type::S16;
        else if(reader_.takeKeyword("uint32"))type=Type::U32;
        else if(reader_.takeKeyword("sint32"))type=Type::S32;
        else reader_.error("Array elements currently require integer types");
        Array a;a.name=declarationName();a.type=type;a.readOnly=immutable;
        reader_.need('(');a.lower=rangeInteger();reader_.need(':');a.upper=rangeInteger();reader_.need(')');
        if(a.lower < -32768 || a.upper>65535 || a.upper<a.lower || a.count()>65535/width(type))
            reader_.error("Array requires ascending inclusive bounds in -32768..65535 and a one-block packed element count");
        auto push=[&](Value v){for(unsigned i=0;i<width(type);++i)a.initial.push_back(static_cast<uint8_t>(v.bits>>(i*8)));};
        if(reader_.take('=')) {
            if(reader_.takeKeyword("Fill")) {
                reader_.need('(');const auto value=literal(type);reader_.need(')');
                for(unsigned n=0;n<a.count();++n)push(value);
            } else {
                reader_.needKeyword("Values");reader_.need('(');push(literal(type));
                while(reader_.take(','))push(literal(type));
                reader_.need(')');
                if(a.initial.size()!=a.bytes())reader_.error("Values count must equal the inclusive Array size");
            }
        }
        arrays_.emplace(a.name,program_.arrays.size());program_.arrays.push_back(std::move(a));
    }
    Expression combine(Op op, Expression left, Expression right, Position at) {
        auto result = std::make_shared<Expr>();
        result->type = left->type; result->op = op; result->at = at;
        result->left = std::move(left); result->right = std::move(right);
        if(op==Op::ShiftRight && result->right->constant && result->right->constant->bits>31)
            golem::syntax::fail(at,"shift count must be 0..31 (language status 32)");
        if (result->left->constant && result->right->constant) {
            try { result->constant = binary(op, *result->left->constant, *result->right->constant); }
            catch (Fault f) { golem::syntax::fail(at, "invalid constant intermediate, language status " + std::to_string(f.status)); }
        }
        return result;
    }
    Expression product(Type type) {
        auto result = atom(type);
        while (reader_.peek().text == "*" || reader_.peek().text == "/") {
            const auto token = reader_.pop();
            if(token.text=="/" && type!=Type::F32)golem::syntax::fail(token.begin,"/ requires float32; use divmod for integers");
            result = combine(token.text=="*"?Op::Multiply:Op::Divide, result, atom(type), token.begin);
        }
        return result;
    }
    Expression additive(Type type) {
        auto result = product(type);
        while (reader_.peek().text == "+" || reader_.peek().text == "-") {
            const auto token = reader_.pop();
            result = combine(token.text == "+" ? Op::Add : Op::Subtract, result, product(type), token.begin);
        }
        return result;
    }
    bool takeShift() {
        const auto saved=reader_;
        if(reader_.peek().text!=">")return false;
        const auto first=reader_.pop();
        if(reader_.peek().text==">" && reader_.peek().begin.offset==first.end.offset){reader_.pop();return true;}
        reader_=saved;return false;
    }
    Expression shifted(Type type) {
        auto result=additive(type);
        while(true) {
            const auto at=reader_.peek().begin;
            if(!takeShift())break;
            if(!wide(type))golem::syntax::fail(at,"right shift requires uint32 or sint32 operand; widen explicitly");
            result=combine(Op::ShiftRight,result,additive(Type::U16),at);
        }
        return result;
    }
    Expression expression(Type type) {
        auto result=shifted(type);
        while(reader_.peek().text=="^") {
            const auto at=reader_.pop().begin;
            if(!integer(type))golem::syntax::fail(at,"XOR requires matching integer operands");
            result=combine(Op::Xor,result,shifted(type),at);
        }
        return result;
    }
    std::string comparison() {
        auto token = reader_.pop();
        auto op = token.text;
        if (reader_.peek().text == "=" && reader_.peek().begin.offset == token.end.offset)
            op += reader_.pop().text;
        if (op != "==" && op != "!=" && op != "<" && op != ">" && op != "<=" && op != ">=")
            golem::syntax::fail(token.begin, "expected uint16 comparison == != < > <= >=");
        return op;
    }
    unsigned booleanDepth_ = 0;
    Expression booleanAtom() {
        if (reader_.peek().text == "(") {
            auto saved = reader_; auto depth = booleanDepth_;
            try {
                reader_.pop(); auto value = boolean();
                reader_.need(')'); return value;
            } catch (const golem::syntax::Error &error) {
                if (std::string(error.what()).find("Boolean nesting") != std::string::npos) throw;
                reader_ = saved; booleanDepth_ = depth;
            }
        }
        auto result = std::make_shared<Expr>(); result->type = Type::Bool; result->at = reader_.peek().begin;
        if (reader_.keyword("true") || reader_.keyword("false")) {
            result->op = Op::Literal; result->literal = literal(Type::Bool); result->constant = result->literal;
        } else if (reader_.peek().kind == Kind::Identifier && globals_.count(reader_.peek().text) &&
                   program_.globals[globals_.at(reader_.peek().text)].type == Type::Bool) {
            result->op = Op::Global; result->global = globals_.at(reader_.pop().text);
        } else {
            result->op=Op::Compare;const auto saved=reader_;bool parsed=false;
            for(auto candidate:{Type::U16,Type::S16,Type::U32,Type::S32,Type::F32}) {
                reader_=saved;
                try {result->left=expression(candidate);result->relation=comparison();result->right=expression(candidate);parsed=true;break;}
                catch(const golem::syntax::Error &) {}
            }
            if(!parsed)reader_.error("invalid same-type comparison");
        }
        return result;
    }
    Expression booleanUnary() {
        if (++booleanDepth_ > 16) reader_.error("Boolean nesting exceeds 16");
        Expression value;
        if (reader_.takeKeyword("Not")) {
            value = std::make_shared<Expr>(); value->type=Type::Bool; value->op=Op::Not;
            value->left=booleanUnary();
        } else value=booleanAtom();
        --booleanDepth_; return value;
    }
    Expression booleanAnd() {
        auto value=booleanUnary();
        while(reader_.takeKeyword("And")) {
            auto next=std::make_shared<Expr>(); next->type=Type::Bool; next->op=Op::And;
            next->left=value; next->right=booleanUnary(); value=next;
        }
        return value;
    }
    Expression boolean() {
        auto value=booleanAnd();
        while(reader_.takeKeyword("Or")) {
            auto next=std::make_shared<Expr>(); next->type=Type::Bool; next->op=Op::Or;
            next->left=value; next->right=booleanAnd(); value=next;
        }
        return value;
    }
    void global() {
        const auto type = reader_.takeKeyword("bool") ? Type::Bool : reader_.takeKeyword("uint16") ? Type::U16 : reader_.takeKeyword("sint16") ? Type::S16 : reader_.takeKeyword("uint32") ? Type::U32 : reader_.takeKeyword("sint32") ? Type::S32 : Type::F32;
        if (type == Type::F32) reader_.needKeyword("float32");
        const auto name = declarationName();
        const double maximum=integer(type)?static_cast<double>(highInteger(type)):type==Type::Bool?1.0:std::numeric_limits<float>::max();
        const double minimum=integer(type)?static_cast<double>(lowInteger(type)):type==Type::F32?-maximum:0.0;
        double low = minimum, high = maximum;
        if (reader_.takeKeyword("Bounds")) {
            if (type == Type::Bool) reader_.error("bool uses canonical false/true, not numeric Bounds");
            reader_.need('('); low = reader_.number(); reader_.need(',');
            high = reader_.number(); reader_.need(')');
            if (low > high || low < minimum || high > maximum ||
                (integer(type) && (std::floor(low) != low || std::floor(high) != high)))
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
            if (reader_.take('(')) {
                auto destination = [&]() {
                    const auto token=reader_.identifier(); auto found=globals_.find(token.text);
                    if(found==globals_.end() || (program_.globals[found->second].type!=Type::U16 && !wide(program_.globals[found->second].type)))
                        golem::syntax::fail(token.begin,"divmod requires matching uint16, uint32 or sint32 global destinations");
                    if(activeCounters_.count(found->second)) reader_.error("cannot assign an active loop counter");
                    return found->second;
                };
                Assignment pair; pair.paired=true; pair.at=at; pair.destination=destination();
                reader_.need(','); pair.remainder=destination();
                if(pair.destination==pair.remainder) reader_.error("divmod destinations must be distinct");
                reader_.need(')'); reader_.need('='); reader_.needKeyword("divmod"); reader_.need('(');
                const auto type=program_.globals[pair.destination].type;
                if(type!=program_.globals[pair.remainder].type)reader_.error("divmod destinations must have the same type");
                pair.left=expression(type); reader_.need(','); pair.right=expression(type); reader_.need(')');
                result.push_back(std::move(pair)); continue;
            }
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
            auto array=arrays_.find(token.text);
            if(array!=arrays_.end()) {
                const auto &a=program_.arrays[array->second];
                if(a.readOnly)golem::syntax::fail(at,"cannot assign a ReadOnly Array element");
                Assignment assignment;assignment.destination=array->second;assignment.at=at;
                assignment.index=arrayIndex(array->second);reader_.need('=');assignment.value=expression(a.type);
                result.push_back(std::move(assignment));continue;
            }
            auto found = globals_.find(token.text);
            if (found == globals_.end()) golem::syntax::fail(at, "undeclared global '" + token.text + "'");
            if (activeCounters_.count(found->second))
                golem::syntax::fail(at, "cannot assign an active loop counter");
            reader_.need('=');
            auto value = program_.globals[found->second].type == Type::Bool ? boolean() : expression(program_.globals[found->second].type);
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
        result.relation = "bool";
        result.left = boolean();
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
        while (reader_.keyword("uint16") || reader_.keyword("float32") || reader_.keyword("sint16") || reader_.keyword("bool") || reader_.keyword("Array") || reader_.keyword("ReadOnly") || reader_.keyword("uint32") || reader_.keyword("sint32")) {
            if(reader_.keyword("Array") || reader_.keyword("ReadOnly"))array();else global();
        }
        if (program_.globals.empty() && program_.arrays.empty()) reader_.error("expected initialized uint16/float32 globals");
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
    Layout result{allocator.allocate("portable.shared_state"), 0, 0, {}, {}, reservations, {}};
    for(const auto &a:program.arrays)result.arrayBuffers.push_back(allocator.allocate("application.array."+a.name));
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
using ArrayState=std::map<unsigned,std::vector<uint8_t>>;
inline unsigned elementOffset(const Array&a,Value index) {
    if(index.number()<a.lower || index.number()>a.upper)throw Fault{5};
    return static_cast<unsigned>(index.number()-a.lower)*width(a.type);
}
inline Value evaluate(const Expression &e, const Layout &layout, const std::vector<uint8_t> &state,
                      const Program *program=nullptr,ArrayState *arrays=nullptr) {
    if (e->op == Op::Literal) return normalize(e->literal);
    if (e->op == Op::Global) return normalize(get(state, layout.offsets.at(e->global), e->type));
    const auto left = evaluate(e->left, layout, state,program,arrays);
    if(e->op==Op::ArrayRead) {
        if(!program || !arrays)throw std::runtime_error("Array reference requires installed data");
        const auto offset=elementOffset(program->arrays.at(e->global),left);
        return get(arrays->at(layout.arrayBuffers.at(e->global)),offset,e->type);
    }
    if (e->op == Op::Convert) {
        return convert(left,e->type);
    }
    if(e->op==Op::Sin||e->op==Op::Cos||e->op==Op::Floor)return mathUnary(e->op,left);
    if (e->op == Op::Not) return {Type::Bool, left.bits ? 0u : 1u};
    if (e->op == Op::And && !left.bits) return {Type::Bool, 0};
    if (e->op == Op::Or && left.bits) return {Type::Bool, 1};
    const auto right = evaluate(e->right, layout, state,program,arrays);
    if (e->op == Op::And || e->op == Op::Or) return {Type::Bool, right.bits};
    if (e->op == Op::Compare) {
        const auto a = left.number(), b = right.number(); const auto &op = e->relation;
        return {Type::Bool, (op == "==" ? a == b : op == "!=" ? a != b : op == "<" ? a < b :
                           op == ">" ? a > b : op == "<=" ? a <= b : a >= b) ? 1u : 0u};
    }
    return binary(e->op, left, right);
}
inline unsigned invokeReference(const Program &program, const Layout &layout,
                                std::vector<uint8_t> &state, size_t entry,ArrayState *arrays=nullptr) {
    if (state.size() != layout.bytes || entry >= program.procedures.size())
        throw std::runtime_error("invalid host reference state or logical entry");
    put(state, layout.errorOffset, {Type::U16, Ok});
    try {
        for (size_t i = 0; i < program.globals.size(); ++i)
            admit(program.globals[i], get(state, layout.offsets[i], program.globals[i].type));
        std::function<void(const std::vector<Assignment>&)> execute = [&](const auto &statements) {
            for (const auto &statement : statements) {
                if (statement.paired) {
                    const auto n=evaluate(statement.left,layout,state,&program,arrays);
                    const auto d=evaluate(statement.right,layout,state,&program,arrays);
                    if(!d.bits) throw Fault{4};
                    Value q{n.type,n.bits/d.bits}, r{n.type,n.bits%d.bits};
                    if(n.type==Type::S32) {
                        const auto numerator=static_cast<int64_t>(n.number()),denominator=static_cast<int64_t>(d.number());
                        if(numerator==-2147483648ll && denominator==-1)throw Fault{6};
                        q.bits=static_cast<uint32_t>(numerator/denominator);r.bits=static_cast<uint32_t>(numerator%denominator);
                    }
                    admit(program.globals[statement.destination],q); admit(program.globals[statement.remainder],r);
                    put(state,layout.offsets[statement.destination],q); put(state,layout.offsets[statement.remainder],r);
                } else if (statement.loop) {
                    for (uint32_t n = 0; n < statement.trips; ++n) {
                        const auto value = statement.start + static_cast<int32_t>(n) * statement.step;
                        put(state, layout.offsets[statement.counter], {Type::U16, static_cast<uint32_t>(value)});
                        execute(statement.yes);
                    }
                } else if (!statement.relation.empty()) {
                    execute(evaluate(statement.left, layout, state,&program,arrays).bits ? statement.yes : statement.no);
                } else {
                    unsigned offset=0;
                    if(statement.index)offset=elementOffset(program.arrays.at(statement.destination),evaluate(statement.index,layout,state,&program,arrays));
                    const auto candidate = evaluate(statement.value, layout, state,&program,arrays);
                    if(statement.index) {
                        if(!arrays)throw std::runtime_error("Array reference requires installed data");
                        put(arrays->at(layout.arrayBuffers.at(statement.destination)),offset,candidate);
                    } else {
                        admit(program.globals[statement.destination], candidate);
                        put(state, layout.offsets[statement.destination], candidate);
                    }
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
    for(size_t i=0;i<program.arrays.size();++i) {
        const auto&a=program.arrays[i];const auto stem="golem_"+a.name;
        equ(stem+"_buffer",layout.arrayBuffers[i]);equ(stem+"_bytes",a.bytes());
        equ(stem+"_count",a.count());equ(stem+"_element_bytes",width(a.type));equ(stem+"_type",tag(a.type));
        // Encode signed bounds as assembler numeric literals, not unsigned casts.
        out << stem << "_lower: equ " << a.lower << '\n' << stem << "_upper: equ " << a.upper << '\n';
        equ(stem+"_access",a.readOnly?1:3);
    }
    return out.str();
}
inline std::string interfaceJson(const Program &program, const Layout &layout) {
    bool wideDivision=false,signedDivision=false;
    std::function<void(const std::vector<Assignment>&)> scan=[&](const auto&body){for(const auto&s:body){
        if(s.paired && wide(s.left->type)){wideDivision=true;signedDivision|=s.left->type==Type::S32;}
        scan(s.yes);scan(s.no);
    }};
    for(const auto&p:program.procedures)scan(p.statements);
    std::ostringstream out;
    out << std::setprecision(17)
        << "{\n  \"schema\": 1, \"profile\": \"portable-scalar-foundation\",\n"
        << "  \"experimental\": true, \"execution_contract\": \"host_reference_only\",\n"
        << "  \"state_buffer\": " << layout.buffer << ", \"state_bytes\": " << layout.bytes << ",\n"
        << "  \"state_owner\": \"application_shared\",\n"
        << "  \"initialization\": \"explicit_install_or_reset\",\n"
        << "  \"error\": {\"offset\": " << layout.errorOffset << ", \"bytes\": 2,"
           " \"codes\": {\"ok\": 0, \"uint16_add_overflow\": 1, \"uint16_sub_underflow\": 2,"
           " \"uint16_mul_overflow\": 3";
    if(!program.arrays.empty())out << ", \"array_index\": 5, \"zero_divisor\": 4";
    else if(wideDivision||requiresNativeMath(program))out << ", \"zero_divisor\": 4";
    if(signedDivision)out << ", \"signed_division_overflow\": 6";
    out << ", \"bounds_failure\": 32, \"nonfinite_float\": 33}},\n"
        << "  \"globals\": [\n";
    for (size_t i = 0; i < program.globals.size(); ++i) {
        const auto &g = program.globals[i];
        out << "    {\"name\": \"" << g.name << "\", \"type\": \"" << name(g.type)
            << "\", \"offset\": " << layout.offsets[i] << ", \"bytes\": " << width(g.type)
            << ", \"bounds\": [" << g.low << ", " << g.high << "]}"
            << (i + 1 == program.globals.size() ? "\n" : ",\n");
    }
    out << "  ],\n";
    if(!program.arrays.empty()) {
        out << "  \"arrays\":[";
        for(size_t i=0;i<program.arrays.size();++i) {
            const auto&a=program.arrays[i];if(i)out << ',';
            out << "{\"name\":\"" << a.name << "\",\"type\":\"" << name(a.type)
                << "\",\"buffer\":" << layout.arrayBuffers[i] << ",\"offset\":0,\"bytes\":" << a.bytes()
                << ",\"lower\":" << a.lower << ",\"upper\":" << a.upper << ",\"count\":" << a.count()
                << ",\"element_bytes\":" << width(a.type) << ",\"access\":" << (a.readOnly?1:3)
                << ",\"owner\":\"application_retained\",\"initial_data\":\"array-" << a.name << ".bin\"}";
        }
        out << "],\n";
    }
    out << "  \"entries\": [\n";
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
    if (!e) { out << "null"; return; }
    out << "{\"type\":\"" << name(e->type) << "\",\"line\":" << e->at.line
        << ",\"column\":" << e->at.column;
    if (e->op == Op::Literal) out << ",\"literal_bits\":" << e->literal.bits;
    else if (e->op == Op::Global) out << ",\"global\":" << e->global;
    else if(e->op==Op::ArrayRead) {out << ",\"array\":" << e->global << ",\"index\":";expressionJson(out,e->left);}
    else {
        out << ",\"operator\":\"" << (e->op == Op::Convert ? name(e->type) : e->op == Op::Not ? "Not" : e->op == Op::And ? "And" : e->op == Op::Or ? "Or" : e->op == Op::Compare ? e->relation : e->op == Op::Add ? "+" : e->op == Op::Subtract ? "-" : e->op == Op::ShiftRight ? ">>" : e->op == Op::Xor ? "^" : e->op==Op::Divide?"/":e->op==Op::Sin?"sin":e->op==Op::Cos?"cos":e->op==Op::Floor?"floor":"*")
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
        if (s.paired) {
            out << "{\"divmod\":{\"quotient\":" << s.destination << ",\"remainder\":" << s.remainder << ",\"numerator\":";
            expressionJson(out,s.left); out << ",\"denominator\":"; expressionJson(out,s.right); out << "}}";
        } else if (s.loop) {
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
            out << (s.index?"{\"array_destination\":":"{\"destination\":") << s.destination;
            if(s.index){out << ",\"index\":";expressionJson(out,s.index);}
            out << ",\"line\":" << s.at.line
                << ",\"column\":" << s.at.column << ",\"value\":";
            expressionJson(out, s.value); out << '}';
        }
    }
    out << ']';
}
inline std::string programJson(const Program &program) {
    std::ostringstream out;
    out << "{\"schema\":4,\"procedures\":[";
    for (size_t i = 0; i < program.procedures.size(); ++i) {
        if (i) out << ',';
        out << "{\"name\":\"" << program.procedures[i].name << "\",\"assignments\":";
        statementsJson(out, program.procedures[i].statements); out << '}';
    }
    out << "]}\n";
    return out.str();
}
} // namespace portable

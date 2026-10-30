#pragma once
#include "portable.hpp"
#include "../ordinary.hpp"
#include <functional>

namespace dual {
using Bytes = std::vector<unsigned char>;
using namespace portable;
using namespace golem::ordinary;
struct Location { unsigned buffer, offset; };
struct Buffered {
    Bytes install, cleanup;
    std::map<unsigned, Bytes> programs, data, ordinaryData;
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
    unsigned wideMulA_=0,wideMulB_=0,wideMulProduct_=0,wideMulKernel_=0;
    unsigned wideDivN_=0,wideDivD_=0,wideDivR_=0,wideDivQ_=0,wideDivKernel_=0;
    unsigned shiftValue_=0,shiftCount_=0,shiftSign_=0,shiftRound_=0;
    struct DecimalFormatter {unsigned input=0,negative=0,text=0,first=0,entry=0;};
    DecimalFormatter decimal16_,decimal32_;
    struct FloatFormatter {unsigned input=0,negative=0,text=0,first=0,entry=0;};
    FloatFormatter floatDecimal_;
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
        if(wide(global.type)) {
            Location ordered=value;
            if(global.type==Type::S32) {
                const auto id=data({0,0,0,0},"signed32_bounds");copy(code,{id,0},value,4);
                emitAdjustMultiLiteral(code,id,7,3,{128});ordered={id,0};
            }
            const auto bias=global.type==Type::S32?2147483648.0:0.0;
            if(global.low>lowInteger(global.type))wideBound(code,ordered,static_cast<uint32_t>(global.low+bias),true);
            if(global.high<highInteger(global.type))wideBound(code,ordered,static_cast<uint32_t>(global.high+bias),false);
            return;
        }
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
    void wideBound(Bytes &code,Location value,uint32_t limit,bool below,unsigned failure=0) {
        const auto id=data(bitsBytes(limit),"wide_bound");compareBytes(code,value,{id,0},below?"<":">",4);
        emitCondJumpBufferValue(code,failure?failure:boundsExit_,3,flag_,0,zero_,0);
    }
    Location wideConvert(Bytes &code,const Expression &e,Location value) {
        Global range{"conversion",e->left->type,static_cast<double>(lowInteger(e->type)),static_cast<double>(highInteger(e->type)),{e->left->type,0}};
        // Check only the intersection constraints meaningful in the source type.
        range.low=std::max(range.low,static_cast<double>(lowInteger(e->left->type)));
        range.high=std::min(range.high,static_cast<double>(highInteger(e->left->type)));
        admit(code,value,range);
        const auto result=data(Bytes(width(e->type),0),"integer_conversion");
        if(width(e->type)==4 && width(e->left->type)==2) {
            emitAdjustMultiLiteral(code,result,kOpSet,0,{0,0,0,0});copy(code,{result,0},value);
            if(e->left->type==Type::S16) {
                const auto negative=sign(code,value);Bytes extend;
                emitAdjustMultiLiteral(extend,result,kOpSet,2,{255,255});
                emitCondCallBufferValue(code,helper(extend,"wide_sign_extend"),3,negative,0,zero_,0);
            }
        } else copy(code,{result,0},value,width(e->type));
        return {result,0};
    }
    void wideNegate(Bytes &code,unsigned value) {
        emitAdjustMultiLiteral(code,value,7,0,{255,255,255,255});
        emitAdjustMultiLiteral(code,value,kOpAddWithCarry,0,{1,0,0,0});
    }
    Location wideMultiply(Bytes &code,const Expression&e,Location left,Location right) {
        if(!wideMulKernel_) {
            wideMulA_=data(Bytes(9,0),"wide_mul_shift");
            wideMulB_=data(Bytes(5,0),"wide_mul_operand");
            wideMulProduct_=data(Bytes(9,0),"wide_mul_product");
            const auto bit=data({0,0},"wide_mul_bit");Bytes add;
            emitAdjustMultiBufferFetched(add,wideMulProduct_,kOpAddWithCarry,0,8,wideMulA_,0);
            const auto addId=helper(add,"wide_mul_add");Bytes kernel;
            // Exactly32 rounds, no data-dependent loop or growing call stack.
            for(unsigned n=0;n<32;++n) {
                copy(kernel,{bit,0},{wideMulB_,n/8},1);
                emitAdjustMultiLiteral(kernel,bit,5,0,{static_cast<unsigned char>(1u<<(n%8)),0});
                emitCondCallBufferValue(kernel,addId,3,bit,0,zero_,0);
                if(n!=31)emitAdjustMultiBufferFetched(kernel,wideMulA_,kOpAddWithCarry,0,8,wideMulA_,0);
            }
            wideMulKernel_=helper(kernel,"wide_mul_kernel");
        }
        emitAdjustMultiLiteral(code,wideMulA_,kOpSet,0,Bytes(9,0));
        copy(code,{wideMulA_,0},left,4);copy(code,{wideMulB_,0},right,4);
        unsigned negative=0;
        if(e->type==Type::S32) {
            negative=sign(code,{left.buffer,left.offset+2});const auto b=sign(code,{right.buffer,right.offset+2});
            Bytes negateA,negateB;wideNegate(negateA,wideMulA_);wideNegate(negateB,wideMulB_);
            // B needs a private carry byte for negation as well.
            emitCondCallBufferValue(code,helper(negateA,"wide_mul_absolute"),3,negative,0,zero_,0);
            emitCondCallBufferValue(code,helper(negateB,"wide_mul_absolute"),3,b,0,zero_,0);
            emitAdjustMultiBufferFetched(code,negative,7,0,2,b,0);
        }
        emitAdjustMultiLiteral(code,wideMulProduct_,kOpSet,0,Bytes(9,0));append(code,call(wideMulKernel_));
        const auto failure=arithmeticExit(3),high=data({0,0},"wide_mul_high");
        copy(code,{high,0},{wideMulProduct_,4},1);
        for(unsigned n=5;n<8;++n)emitAdjustMultiBufferFetched(code,high,6,0,1,wideMulProduct_,n);
        emitCondJumpBufferValue(code,failure,3,high,0,zero_,0);
        // Snapshot per expression: a later nested multiply may reuse the kernel.
        const auto result=data(Bytes(5,0),"wide_mul_result");copy(code,{result,0},{wideMulProduct_,0},4);
        if(e->type==Type::S32) {
            Bytes positive,minus;wideBound(positive,{result,0},2147483647u,false,failure);
            wideBound(minus,{result,0},2147483648u,false,failure);wideNegate(minus,result);
            emitCondCallBufferValue(code,helper(positive,"wide_mul_positive"),2,negative,0,zero_,0);propagate(code);
            emitCondCallBufferValue(code,helper(minus,"wide_mul_negative"),3,negative,0,zero_,0);propagate(code);
        }
        return {result,0};
    }
    std::pair<Location,Location> wideDivmod(Bytes &code,Type type,Location n,Location d) {
        if(!wideDivKernel_) {
            wideDivN_=data(Bytes(5,0),"wide_div_numerator");wideDivD_=data(Bytes(5,0),"wide_div_denominator");
            wideDivR_=data(Bytes(6,0),"wide_div_remainder");wideDivQ_=data(Bytes(5,0),"wide_div_quotient");
            const auto complement=data(Bytes(6,0),"wide_div_complement"),mask=data(Bytes(4,0),"wide_div_mask"),bit=data({0,0},"wide_div_bit");
            Bytes bring;emitAdjustMultiLiteral(bring,wideDivR_,kOpAddWithCarry,0,{1,0,0,0,0});
            const auto bringId=helper(bring,"wide_div_bring");Bytes subtract;
            emitAdjustMultiBufferFetched(subtract,wideDivR_,kOpAddWithCarry,0,5,complement,0);
            emitAdjustMultiBufferFetched(subtract,wideDivQ_,6,0,4,mask,0);
            const auto subtractId=helper(subtract,"wide_div_subtract");Bytes compare;
            compareBytes(compare,{wideDivR_,0},{wideDivD_,0},">=",5);
            const auto compareId=helper(compare,"wide_div_compare");Bytes kernel;
            emitAdjustMultiLiteral(kernel,complement,kOpSet,0,{255,255,255,255,255,0});
            emitAdjustMultiBufferFetched(kernel,complement,7,0,4,wideDivD_,0);
            emitAdjustMultiLiteral(kernel,complement,kOpAddWithCarry,0,{1,0,0,0,0});
            for(int b=31;b>=0;--b) {
                emitAdjustMultiBufferFetched(kernel,wideDivR_,kOpAddWithCarry,0,5,wideDivR_,0);
                copy(kernel,{bit,0},{wideDivN_,static_cast<unsigned>(b)/8},1);
                emitAdjustMultiLiteral(kernel,bit,5,0,{static_cast<unsigned char>(1u<<(b%8)),0});
                emitCondCallBufferValue(kernel,bringId,3,bit,0,zero_,0);
                emitAdjustMultiLiteral(kernel,mask,kOpSet,0,bitsBytes(uint32_t(1)<<b));
                append(kernel,call(compareId));
                emitCondCallBufferValue(kernel,subtractId,3,flag_,0,zero_,0);
            }
            wideDivKernel_=helper(kernel,"wide_div_kernel");
        }
        const auto nonzero=data({0,0},"wide_div_nonzero");copy(code,{nonzero,0},d,1);
        for(unsigned b=1;b<4;++b)emitAdjustMultiBufferFetched(code,nonzero,6,0,1,d.buffer,d.offset+b);
        emitCondJumpBufferValue(code,arithmeticExit(4),2,nonzero,0,zero_,0);
        unsigned signN=0,signQ=0;
        if(type==Type::S32) {
            const auto minimum=data(bitsBytes(0x80000000u),"wide_div_min"),minusOne=data(bitsBytes(0xffffffffu),"wide_div_minus_one");
            Bytes overflow;compareBytes(overflow,d,{minusOne,0},"==",4);
            emitCondJumpBufferValue(overflow,arithmeticExit(6),3,flag_,0,zero_,0);
            const auto check=helper(overflow,"wide_div_overflow");compareBytes(code,n,{minimum,0},"==",4);
            emitCondCallBufferValue(code,check,3,flag_,0,zero_,0);propagate(code);
            signN=sign(code,{n.buffer,n.offset+2});signQ=data({0,0},"wide_div_sign");copy(code,{signQ,0},{signN,0});
            const auto signD=sign(code,{d.buffer,d.offset+2});emitAdjustMultiBufferFetched(code,signQ,7,0,2,signD,0);
            copy(code,{wideDivN_,0},n,4);copy(code,{wideDivD_,0},d,4);Bytes absN,absD;
            wideNegate(absN,wideDivN_);wideNegate(absD,wideDivD_);
            emitCondCallBufferValue(code,helper(absN,"wide_div_absolute"),3,signN,0,zero_,0);
            emitCondCallBufferValue(code,helper(absD,"wide_div_absolute"),3,signD,0,zero_,0);
        } else {copy(code,{wideDivN_,0},n,4);copy(code,{wideDivD_,0},d,4);}
        emitAdjustMultiLiteral(code,wideDivD_,kOpSet,4,{0});
        emitAdjustMultiLiteral(code,wideDivR_,kOpSet,0,Bytes(6,0));emitAdjustMultiLiteral(code,wideDivQ_,kOpSet,0,Bytes(5,0));
        append(code,call(wideDivKernel_));
        if(type==Type::S32) {
            Bytes qNegative,rNegative;wideNegate(qNegative,wideDivQ_);wideNegate(rNegative,wideDivR_);
            emitCondCallBufferValue(code,helper(qNegative,"wide_div_restore_quotient"),3,signQ,0,zero_,0);
            emitCondCallBufferValue(code,helper(rNegative,"wide_div_restore_remainder"),3,signN,0,zero_,0);
        }
        const auto q=data(Bytes(4,0),"wide_quotient"),r=data(Bytes(4,0),"wide_remainder");
        copy(code,{q,0},{wideDivQ_,0},4);copy(code,{r,0},{wideDivR_,0},4);return {{q,0},{r,0}};
    }
    Location shiftRight(Bytes &code,Type type,Location value,Location count) {
        bound(code,count,31,false);
        if(!shiftRound_) {
            shiftValue_=data(Bytes(4,0),"shift_value");shiftCount_=data(Bytes(3,0),"shift_count");shiftSign_=data({0,0},"shift_sign");
            Bytes halves;for(unsigned n=0;n<256;++n)halves.push_back(static_cast<unsigned char>(n/2));
            const auto table=data(halves,"shift_halves"),index=data({0,0},"shift_index"),bit=data({0,0},"shift_bit");Bytes round;
            // Low-to-high writes preserve the not-yet-read neighbouring byte.
            for(unsigned n=0;n<4;++n) {
                Bytes load;copy(load,{shiftValue_,n},{table,0},1);const auto action=helper(load,"shift_lookup");
                copy(round,{index,0},{shiftValue_,n},1);copy(round,{action,13},{index,0});append(round,call(action));
                if(n<3) {
                    copy(round,{bit,0},{shiftValue_,n+1},1);emitAdjustMultiLiteral(round,bit,5,0,{1,0});
                    Bytes carry;emitAdjustMultiLiteral(carry,shiftValue_,6,n,{128});
                    emitCondCallBufferValue(round,helper(carry,"shift_carry"),3,bit,0,zero_,0);
                } else emitAdjustMultiBufferFetched(round,shiftValue_,6,3,1,shiftSign_,0);
            }
            emitAdjustMultiLiteral(round,shiftCount_,kOpAddWithCarry,0,{255,255});
            shiftRound_=helper(round,"shift_round");
        }
        copy(code,{shiftValue_,0},value,4);copy(code,{shiftCount_,0},count);
        emitAdjustMultiLiteral(code,shiftSign_,kOpSet,0,{0,0});
        if(type==Type::S32){copy(code,{shiftSign_,0},{value.buffer,value.offset+3},1);emitAdjustMultiLiteral(code,shiftSign_,5,0,{128,0});}
        for(unsigned n=0;n<31;++n)emitCondCallBufferValue(code,shiftRound_,3,shiftCount_,0,zero_,0);
        const auto result=data(Bytes(4,0),"shift_result");copy(code,{result,0},{shiftValue_,0},4);return {result,0};
    }
    Location wideArithmetic(Bytes &code,const Expression &e,Location left,Location right) {
        if(e->op==Op::Multiply)return wideMultiply(code,e,left,right);
        const auto result=data({0,0,0,0,0},"wide_result");const auto failure=arithmeticExit(e->op==Op::Add?1:2);
        unsigned a=0,b=0;
        if(e->type==Type::S32){a=sign(code,{left.buffer,left.offset+2});b=sign(code,{right.buffer,right.offset+2});}
        if(e->op==Op::Subtract && e->type==Type::U32) {
            compareBytes(code,left,right,"<",4);emitCondJumpBufferValue(code,failure,3,flag_,0,zero_,0);
        }
        copy(code,{result,0},left,4);
        if(e->op==Op::Subtract) {
            const auto complement=data({0,0,0,0,0},"wide_complement");copy(code,{complement,0},right,4);
            emitAdjustMultiLiteral(code,complement,7,0,{255,255,255,255});
            emitAdjustMultiLiteral(code,complement,kOpAddWithCarry,0,{1,0,0,0});
            emitAdjustMultiBufferFetched(code,result,kOpAddWithCarry,0,4,complement,0);
        } else emitAdjustMultiBufferFetched(code,result,kOpAddWithCarry,0,4,right.buffer,right.offset);
        if(e->type==Type::S32) {
            const auto changed=sign(code,{result,2});
            emitAdjustMultiBufferFetched(code,changed,7,0,2,a,0);emitAdjustMultiBufferFetched(code,a,7,0,2,b,0);
            if(e->op==Op::Add)emitAdjustMultiLiteral(code,a,7,0,{128,0});
            emitAdjustMultiBufferFetched(code,changed,5,0,2,a,0);
            emitCondJumpBufferValue(code,failure,3,changed,0,zero_,0);
        } else if(e->op==Op::Add) {
            const auto carry=data({0,0},"wide_carry");copy(code,{carry,0},{result,4},1);
            emitCondJumpBufferValue(code,failure,3,carry,0,zero_,0);
        }
        return {result,0};
    }
    Location arrayOffset(Bytes &code,size_t array,const Expression &index) {
        const auto value=expression(code,index);const auto&a=program_.arrays[array];
        const int lo=index->type==Type::S16?-32768:0,hi=index->type==Type::S16?32767:65535;
        const auto failure=arithmeticExit(5),ordered=data({0,0},"array_index_order");
        copy(code,{ordered,0},value);
        if(index->type==Type::S16)emitAdjustMultiLiteral(code,ordered,7,1,{128});
        if(a.lower>hi || a.upper<lo)append(code,jump(failure));
        else {
            if(a.lower>lo)bound(code,{ordered,0},static_cast<unsigned>(a.lower-lo),true,failure);
            if(a.upper<hi)bound(code,{ordered,0},static_cast<unsigned>(a.upper-lo),false,failure);
        }
        const auto offset=data({0,0,0},"array_offset");copy(code,{offset,0},value);
        // Admission proves normalized/doubled offsets fit uint16; carry is private.
        emitAdjustMultiLiteral(code,offset,kOpAddWithCarry,0,le16Bytes(static_cast<unsigned>(-a.lower)&65535));
        emitAdjustMultiBufferFetched(code,offset,kOpAddWithCarry,0,2,offset,0);
        if(width(a.type)==4)emitAdjustMultiBufferFetched(code,offset,kOpAddWithCarry,0,2,offset,0);
        return {offset,0};
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
        if(mathOperation(expression->op))throw std::runtime_error("float /, sin, cos and floor currently require --target native");
        if (expression->constant)
            return {data(width(expression->type)==4?bitsBytes(expression->constant->bits):le16Bytes(expression->constant->bits), "constant"), 0};
        if(expression->op==Op::ArrayRead) {
            const auto offset=arrayOffset(code,expression->global,expression->left);
            const auto result=data(Bytes(width(expression->type),0),"array_element");Bytes load;
            copy(load,{result,0},{layout_.arrayBuffers.at(expression->global),0},width(expression->type));
            const auto action=helper(load,"array_read");copy(code,{action,13},offset);
            append(code,call(action));return {result,0};
        }
        if (expression->op == Op::Global) {
            Location value{layout_.buffer, layout_.offsets[expression->global]};
            if(expression->type!=Type::F32) return value;
            const auto normalized=data({0,0,0,0},"float_read");copy(code,{normalized,0},value,4);floatCheck(code,{normalized,0});return {normalized,0};
        }
        if(expression->op==Op::Convert) {
            const auto value=this->expression(code,expression->left);
            if(wide(expression->type)||wide(expression->left->type))return wideConvert(code,expression,value);
            if(expression->type==Type::F32 || expression->left->type==Type::F32) return floatConvert(code,expression,value);
            if(expression->type!=expression->left->type) bound(code,value,32767,false);
            return value;
        }
        if(expression->op==Op::BitNot) {
            const auto value=this->expression(code,expression->left);
            const auto bytes=width(expression->type),result=data(Bytes(bytes,0),"bitnot_result");
            copy(code,{result,0},value,bytes);
            emitAdjustMultiLiteral(code,result,7,0,Bytes(bytes,255));
            return {result,0};
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
        if(expression->op==Op::Xor || expression->op==Op::BitAnd || expression->op==Op::BitOr) {
            const auto bytes=width(expression->type),result=data(Bytes(bytes,0),expression->op==Op::Xor?"xor_result":expression->op==Op::BitAnd?"bitand_result":"bitor_result");
            copy(code,{result,0},left,bytes);
            emitAdjustMultiBufferFetched(code,result,expression->op==Op::BitAnd?5:expression->op==Op::BitOr?6:7,0,bytes,right.buffer,right.offset);
            return {result,0};
        }
        if(expression->op==Op::ShiftRight)return shiftRight(code,expression->type,left,right);
        if(wide(expression->type))return wideArithmetic(code,expression,left,right);
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
        if(wide(statement.left->type)) {
            if(statement.left->type==Type::S32) {
                const auto a=data({0,0,0,0},"signed32_order"),b=data({0,0,0,0},"signed32_order");
                copy(code,{a,0},left,4);copy(code,{b,0},right,4);
                emitAdjustMultiLiteral(code,a,7,3,{128});emitAdjustMultiLiteral(code,b,7,3,{128});
                left={a,0};right={b,0};
            }
            compareBytes(code,left,right,statement.relation,4);return;
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
    DecimalFormatter &decimalFormatter(unsigned bytes) {
        auto &f=bytes==2?decimal16_:decimal32_;
        if(f.entry)return f;
        const auto digits=bytes==2?5u:10u;
        f.input=data(Bytes(bytes+1,0),"decimal_magnitude");
        f.negative=data({0,0},"decimal_sign");
        f.text=data(Bytes(digits+1,'0'),"decimal_text");
        f.first=data({0,0,0},"decimal_first");
        const auto ten=data(bytes==2?le16Bytes(10):bitsBytes(10),"decimal_ten");
        const auto remainder=data({0,0,0},"decimal_remainder");
        Bytes division;
        if(bytes==2) {
            const auto operands=compiler_.divmodOperands();
            copy(division,{operands.first,0},{f.input,0});copy(division,{operands.second,0},{ten,0});
            const auto q=data({0,0},"decimal_quotient");
            compiler_.divmod(division,q,remainder);copy(division,{f.input,0},{q,0});
        } else {
            const auto pair=wideDivmod(division,Type::U32,{f.input,0},{ten,0});
            copy(division,{f.input,0},pair.first,4);copy(division,{remainder,0},pair.second,2);
        }
        const auto divide=helper(division,"decimal_divide_ten");
        Bytes code,absolute;
        emitAdjustMultiLiteral(code,f.first,kOpSet,0,{static_cast<unsigned char>(digits),0,0});
        emitAdjustMultiLiteral(absolute,f.input,7,0,Bytes(bytes,255));
        Bytes one(bytes,0);one[0]=1;emitAdjustMultiLiteral(absolute,f.input,kOpAddWithCarry,0,one);
        emitCondCallBufferValue(code,helper(absolute,"decimal_absolute"),3,f.negative,0,zero_,0);
        const auto character=data({0,0,0},"decimal_character");
        for(unsigned position=digits;position;--position) {
            append(code,call(divide));
            Bytes first;emitAdjustMultiLiteral(first,f.first,kOpSet,0,{static_cast<unsigned char>(position),0});
            emitCondCallBufferValue(code,helper(first,"decimal_nonzero"),3,remainder,0,zero_,0);
            copy(code,{character,0},{remainder,0});
            emitAdjustMultiLiteral(code,character,kOpAddWithCarry,0,{48,0});
            copy(code,{f.text,position},{character,0},1);
        }
        Bytes dash;emitAdjustMultiLiteral(dash,f.text,kOpSet,0,{'-'});
        const auto dashID=helper(dash,"decimal_dash");Bytes minus;
        emitAdjustMultiLiteral(minus,f.first,kOpAddWithCarry,0,{255,255});
        copy(minus,{dashID,7},{f.first,0});append(minus,call(dashID));
        emitCondCallBufferValue(code,helper(minus,"decimal_minus"),3,f.negative,0,zero_,0);
        f.entry=helper(code,"decimal_format");return f;
    }
    void printInteger(Bytes &code,const Expression &value) {
        const auto input=expression(code,value);const auto bytes=width(value->type);
        auto &f=decimalFormatter(bytes);
        emitAdjustMultiLiteral(code,f.input,kOpSet,0,Bytes(bytes+1,0));copy(code,{f.input,0},input,bytes);
        emitAdjustMultiLiteral(code,f.negative,kOpSet,0,{0,0});
        if(signedInteger(value->type)) {
            const auto negative=sign(code,{input.buffer,input.offset+(bytes==4?2u:0u)});
            copy(code,{f.negative,0},{negative,0});
        }
        append(code,call(f.entry));propagate(code);
        const auto length=bytes==2?6u:11u;
        const auto text=data(Bytes(length,0),"str_temporary");copy(code,{text,0},{f.text,0},length);
        Bytes invoke;emitBufferHeader(invoke,text);invoke.insert(invoke.end(),{11,0,0,0});
        const auto action=helper(invoke,"print_text");copy(code,{action,6},{f.first,0});
        append(code,call(action));
        emitAdjustMultiLiteral(code,text,kOpSet,0,Bytes(length,0));
    }
    FloatFormatter &floatFormatter() {
        auto &f=floatDecimal_;
        if(f.entry)return f;
        // x=M*2^p, 2^23<=M<2^24, -149<=p<=104.  For p<0,
        // A=M*5^-p and x=A*10^p; A<2^24*5^149<10^112.
        // For p>=0 A<2^128 has at most39 digits.  One spare digit
        // guards the fixed113-byte exact decimal coefficient.
        f.input=data(Bytes(4,0),"float_decimal_input");
        f.negative=data({0,0},"float_decimal_sign");
        f.text=data(Bytes(16,0),"float_decimal_text");
        f.first=data({0,0,0},"float_decimal_first");
        auto word=[&](const std::string &role){return data({0,0,0},"float_decimal_"+role);};
        auto constant=[&](unsigned value){return data(le16Bytes(value),"float_decimal_constant");};
        auto set=[&](Bytes &c,unsigned id,unsigned value){emitAdjustMultiLiteral(c,id,kOpSet,0,{static_cast<unsigned char>(value),static_cast<unsigned char>(value>>8),0});};
        auto add=[&](Bytes &c,unsigned id,unsigned value){emitAdjustMultiLiteral(c,id,kOpAddWithCarry,0,le16Bytes(value));};
        auto define=[&](unsigned id,Bytes c){
            if(c.empty()||c.size()>65535)throw std::runtime_error("float decimal helper exceeds WRITE block");
            output_.programs[id]=std::move(c);
        };
        auto readAction=[&](unsigned source,unsigned result,const std::string &role){Bytes c;copy(c,{result,0},{source,0},1);emitAdjustMultiLiteral(c,result,5,0,{255,0});return helper(c,"float_decimal_"+role);};
        auto readAt=[&](Bytes &c,unsigned action,unsigned offset){copy(c,{action,13},{offset,0});append(c,call(action));};
        auto storeAction=[&](unsigned destination,unsigned source,const std::string &role){Bytes c;copy(c,{destination,0},{source,0},1);return helper(c,"float_decimal_"+role);};
        auto storeAt=[&](Bytes &c,unsigned action,unsigned offset){copy(c,{action,7},{offset,0});append(c,call(action));};
        const auto one=constant(1),five=constant(5),nine=constant(9),ten=constant(10),oneFifty=constant(150);
        const auto digits=data(Bytes(113,0),"float_decimal_digits"),length=word("length"),scale=word("scale"),rounds=word("rounds");
        const auto index=word("index"),digit=word("digit"),packed=word("packed"),carry=word("carry"),tableIndex=word("table_index");
        Bytes tables(512,0),decode(80,0);
        for(unsigned mode=0;mode<2;++mode)for(unsigned c=0;c<(mode?5u:2u);++c)for(unsigned d=0;d<10;++d) {
            const auto product=d*(mode?5u:2u)+c;
            tables[256*mode+16*c+d]=static_cast<unsigned char>((product/10)*16+product%10);
        }
        for(unsigned n=0;n<decode.size();++n)decode[n]=static_cast<unsigned char>(n/16);
        const auto table=data(tables,"float_decimal_multiply_table"),carryTable=data(decode,"float_decimal_carry_table");
        const auto readDigit=readAction(digits,digit,"read_digit"),readProduct=readAction(table,packed,"read_product"),readCarry=readAction(carryTable,packed,"read_carry");
        const auto storeDigit=storeAction(digits,packed,"store_digit");
        const auto digitLoop=allocate("float_decimal_digit_loop");Bytes multiplyDigit;
        readAt(multiplyDigit,readDigit,index);copy(multiplyDigit,{tableIndex,0},{digit,0},1);
        emitAdjustMultiBufferFetched(multiplyDigit,tableIndex,6,0,1,carry,0);
        readAt(multiplyDigit,readProduct,tableIndex);
        copy(multiplyDigit,{carry,0},{packed,0},1);emitAdjustMultiLiteral(multiplyDigit,carry,5,0,{240});
        emitAdjustMultiLiteral(multiplyDigit,packed,5,0,{15});storeAt(multiplyDigit,storeDigit,index);add(multiplyDigit,index,1);
        emitCondJumpBufferValue(multiplyDigit,digitLoop,4,index,0,length,0);define(digitLoop,std::move(multiplyDigit));
        Bytes appendCarry;readAt(appendCarry,readCarry,carry);storeAt(appendCarry,storeDigit,length);add(appendCarry,length,1);
        const auto appendCarryID=helper(appendCarry,"float_decimal_append_carry");
        const auto scaleLoop=allocate("float_decimal_scale_loop");Bytes multiply;
        set(multiply,index,0);set(multiply,carry,0);append(multiply,call(digitLoop));
        emitCondCallBufferValue(multiply,appendCarryID,3,carry,0,zero_,0);add(multiply,rounds,65535);
        emitCondJumpBufferValue(multiply,scaleLoop,3,rounds,0,zero_,0);define(scaleLoop,std::move(multiply));

        // Retain9 leading digits, one guard digit and a sticky OR of every
        // discarded lower digit.  All loops below tail-jump with finite counts.
        const auto significant=data(Bytes(11,0),"float_decimal_significant"),count=word("count"),remaining=word("remaining"),cursor=word("cursor");
        const auto guard=word("guard"),sticky=word("sticky"),roundUp=word("round_up"),exponent=word("exponent");
        const auto readSignificant=readAction(significant,digit,"read_significant"),storeSignificant=storeAction(significant,digit,"store_significant");
        const auto extractLoop=allocate("float_decimal_extract_loop");Bytes extract;
        readAt(extract,readDigit,cursor);storeAt(extract,storeSignificant,count);add(extract,count,1);add(extract,cursor,65535);add(extract,remaining,65535);
        emitCondJumpBufferValue(extract,65535,2,remaining,0,zero_,0);
        emitCondJumpBufferValue(extract,extractLoop,4,count,0,nine,0);define(extractLoop,std::move(extract));
        const auto stickyLoop=allocate("float_decimal_sticky_loop");Bytes collectSticky;
        readAt(collectSticky,readDigit,cursor);emitAdjustMultiBufferFetched(collectSticky,sticky,6,0,1,digit,0);
        add(collectSticky,cursor,65535);add(collectSticky,remaining,65535);
        emitCondJumpBufferValue(collectSticky,stickyLoop,3,remaining,0,zero_,0);define(stickyLoop,std::move(collectSticky));
        Bytes yes;set(yes,roundUp,1);const auto roundYes=helper(yes,"float_decimal_round_yes");
        Bytes tie;emitCondCallBufferValue(tie,roundYes,3,sticky,0,zero_,0);
        copy(tie,{digit,0},{significant,8},1);emitAdjustMultiLiteral(tie,digit,5,0,{1});
        emitCondCallBufferValue(tie,roundYes,3,digit,0,zero_,0);const auto tieID=helper(tie,"float_decimal_tie");
        Bytes rounding;readAt(rounding,readDigit,cursor);copy(rounding,{guard,0},{digit,0});
        add(rounding,cursor,65535);add(rounding,remaining,65535);set(rounding,sticky,0);set(rounding,roundUp,0);
        emitCondCallBufferValue(rounding,stickyLoop,3,remaining,0,zero_,0);
        emitCondCallBufferValue(rounding,roundYes,5,guard,0,five,0);
        emitCondCallBufferValue(rounding,tieID,2,guard,0,five,0);
        Bytes increment;
        for(int position=8;position>=0;--position) {
            Bytes next,reset,stop;
            emitAdjustMultiLiteral(next,significant,kOpAddWithCarry,static_cast<unsigned>(position),{1});
            copy(next,{digit,0},{significant,static_cast<unsigned>(position)},1);
            emitAdjustMultiLiteral(reset,significant,kOpSet,static_cast<unsigned>(position),{0});
            set(stop,roundUp,0);
            emitCondCallBufferValue(next,helper(reset,"float_decimal_round_carry"),2,digit,0,ten,0);
            emitCondCallBufferValue(next,helper(stop,"float_decimal_round_stop"),3,digit,0,ten,0);
            emitCondCallBufferValue(increment,helper(next,"float_decimal_round_digit"),3,roundUp,0,zero_,0);
        }
        Bytes grow;emitAdjustMultiLiteral(grow,significant,kOpSet,0,{1});add(grow,exponent,1);
        emitCondCallBufferValue(increment,helper(grow,"float_decimal_round_grow"),3,roundUp,0,zero_,0);
        const auto incrementID=helper(increment,"float_decimal_increment");
        emitCondCallBufferValue(rounding,incrementID,3,roundUp,0,zero_,0);
        const auto roundID=helper(rounding,"float_decimal_round");
        const auto trimLoop=allocate("float_decimal_trim_loop");Bytes trim;
        emitCondJumpBufferValue(trim,65535,2,count,0,one,0);
        readAt(trim,readSignificant,cursor);emitCondJumpBufferValue(trim,65535,3,digit,0,zero_,0);
        add(trim,count,65535);add(trim,cursor,65535);append(trim,jump(trimLoop));define(trimLoop,std::move(trim));

        // Build backwards into16 bytes.  Both notation forms use at most15
        // characters including sign; the final text ends at byte15.
        const auto character=word("character"),position=word("position"),zeros=word("zeros"),pointAt=word("point_at");
        Bytes emit;add(emit,f.first,65535);const auto storeCharacter=storeAction(f.text,character,"store_character");storeAt(emit,storeCharacter,f.first);
        const auto emitID=helper(emit,"float_decimal_emit_character");
        auto put=[&](Bytes &c,unsigned ch){set(c,character,ch);append(c,call(emitID));};
        Bytes point;put(point,'.');const auto pointID=helper(point,"float_decimal_point");
        const auto zeroLoop=allocate("float_decimal_zero_loop");Bytes emitZero;put(emitZero,'0');add(emitZero,zeros,65535);
        emitCondJumpBufferValue(emitZero,zeroLoop,3,zeros,0,zero_,0);define(zeroLoop,std::move(emitZero));
        const auto reverseLoop=allocate("float_decimal_reverse_loop");Bytes reverse;
        readAt(reverse,readSignificant,cursor);copy(reverse,{character,0},{digit,0},1);add(reverse,character,48);append(reverse,call(emitID));
        emitCondCallBufferValue(reverse,pointID,2,cursor,0,pointAt,0);
        emitCondJumpBufferValue(reverse,65535,2,cursor,0,zero_,0);
        add(reverse,cursor,65535);append(reverse,jump(reverseLoop));define(reverseLoop,std::move(reverse));
        auto reverseDigits=[&](Bytes &c){copy(c,{cursor,0},{count,0});add(c,cursor,65535);append(c,call(reverseLoop));};
        Bytes negativeFixed;set(negativeFixed,pointAt,255);reverseDigits(negativeFixed);
        copy(negativeFixed,{zeros,0},{exponent,0});negate(negativeFixed,zeros);add(negativeFixed,zeros,65535);
        emitCondCallBufferValue(negativeFixed,zeroLoop,3,zeros,0,zero_,0);put(negativeFixed,'.');put(negativeFixed,'0');
        const auto negativeFixedID=helper(negativeFixed,"float_decimal_negative_fixed");
        Bytes integerZeros;copy(integerZeros,{zeros,0},{position,0});
        // zeros=position-count is nonnegative in this branch.
        copy(integerZeros,{digit,0},{count,0});negate(integerZeros,digit);emitAdjustMultiBufferFetched(integerZeros,zeros,kOpAddWithCarry,0,2,digit,0);
        emitCondCallBufferValue(integerZeros,zeroLoop,3,zeros,0,zero_,0);set(integerZeros,pointAt,255);
        const auto integerZerosID=helper(integerZeros,"float_decimal_integer_zeros");
        Bytes positiveFixed;copy(positiveFixed,{position,0},{exponent,0});add(positiveFixed,position,1);
        copy(positiveFixed,{pointAt,0},{position,0});
        emitCondCallBufferValue(positiveFixed,integerZerosID,6,count,0,position,0);reverseDigits(positiveFixed);
        const auto positiveFixedID=helper(positiveFixed,"float_decimal_positive_fixed");
        const auto exponentSign=word("exponent_sign"),exponentMagnitude=word("exponent_magnitude"),exponentIndex=word("exponent_index"),exponentPair=word("exponent_pair");
        Bytes exponentCharacters;for(unsigned n=0;n<40;++n){exponentCharacters.push_back(static_cast<unsigned char>('0'+n/10));exponentCharacters.push_back(static_cast<unsigned char>('0'+n%10));}
        const auto exponentTable=data(exponentCharacters,"float_decimal_exponent_text");
        Bytes loadPair;copy(loadPair,{exponentPair,0},{exponentTable,0},2);const auto loadPairID=helper(loadPair,"float_decimal_exponent_pair");
        Bytes scientific;copy(scientific,{exponentMagnitude,0},{exponent,0});
        Bytes absExponent;negate(absExponent,exponentMagnitude);const auto absExponentID=helper(absExponent,"float_decimal_absolute_exponent");
        emitCondCallBufferValue(scientific,absExponentID,3,exponentSign,0,zero_,0);
        copy(scientific,{exponentIndex,0},{exponentMagnitude,0});emitAdjustMultiBufferFetched(scientific,exponentIndex,kOpAddWithCarry,0,2,exponentMagnitude,0);
        copy(scientific,{loadPairID,13},{exponentIndex,0});append(scientific,call(loadPairID));
        copy(scientific,{character,0},{exponentPair,1},1);append(scientific,call(emitID));
        copy(scientific,{character,0},{exponentPair,0},1);append(scientific,call(emitID));
        set(scientific,character,'+');Bytes minus;set(minus,character,'-');const auto minusID=helper(minus,"float_decimal_exponent_minus");
        emitCondCallBufferValue(scientific,minusID,3,exponentSign,0,zero_,0);append(scientific,call(emitID));put(scientific,'e');
        set(scientific,pointAt,1);Bytes noPoint;set(noPoint,pointAt,255);
        emitCondCallBufferValue(scientific,helper(noPoint,"float_decimal_no_point"),2,count,0,one,0);reverseDigits(scientific);
        const auto scientificID=helper(scientific,"float_decimal_scientific");
        const auto scientificFlag=word("scientific");
        Bytes fixed;emitCondCallBufferValue(fixed,negativeFixedID,3,exponentSign,0,zero_,0);
        emitCondCallBufferValue(fixed,positiveFixedID,2,exponentSign,0,zero_,0);const auto fixedID=helper(fixed,"float_decimal_fixed");
        Bytes markScientific;set(markScientific,scientificFlag,1);const auto markScientificID=helper(markScientific,"float_decimal_mark_scientific");
        Bytes minusRange;emitCondCallBufferValue(minusRange,markScientificID,5,exponentMagnitude,0,constant(4),0);
        const auto minusRangeID=helper(minusRange,"float_decimal_negative_range");
        Bytes plusRange;emitCondCallBufferValue(plusRange,markScientificID,5,exponent,0,constant(8),0);
        const auto plusRangeID=helper(plusRange,"float_decimal_positive_range");

        Bytes code;emitAdjustMultiLiteral(code,f.text,kOpSet,0,Bytes(16,0));set(code,f.first,16);
        floatCheck(code,{f.input,0});
        // floatCheck only needs zero/all-ones and rotates the exponent's low
        // bit into bit7.  Decode the actual biased exponent independently.
        const auto biasedExponent=word("biased_exponent"),exponentLow=word("exponent_low_bit");
        set(code,biasedExponent,0);copy(code,{biasedExponent,0},{f.input,3},1);emitAdjustMultiLiteral(code,biasedExponent,5,0,{127});
        emitAdjustMultiBufferFetched(code,biasedExponent,kOpAddWithCarry,0,2,biasedExponent,0);
        set(code,exponentLow,0);copy(code,{exponentLow,0},{f.input,2},1);emitAdjustMultiLiteral(code,exponentLow,5,0,{128});
        Bytes oddExponent;add(oddExponent,biasedExponent,1);
        emitCondCallBufferValue(code,helper(oddExponent,"float_decimal_odd_exponent"),3,exponentLow,0,zero_,0);
        Bytes zero;put(zero,'0');const auto zeroID=helper(zero,"float_decimal_zero");
        emitCondJumpBufferValue(code,zeroID,2,biasedExponent,0,zero_,0);
        copy(code,{f.negative,0},{f.input,3},1);emitAdjustMultiLiteral(code,f.negative,5,0,{128});
        auto &integer=decimalFormatter(4);
        emitAdjustMultiLiteral(code,integer.input,kOpSet,0,Bytes(5,0));copy(code,{integer.input,0},{f.input,0},3);
        emitAdjustMultiLiteral(code,integer.input,6,2,{128});emitAdjustMultiLiteral(code,integer.negative,kOpSet,0,{0,0});append(code,call(integer.entry));
        emitAdjustMultiLiteral(code,digits,kOpSet,0,Bytes(113,0));
        for(unsigned n=0;n<8;++n)copy(code,{digits,n},{integer.text,10-n},1);
        emitAdjustMultiLiteral(code,digits,5,0,Bytes(8,15));set(code,length,8);
        Bytes seven;set(seven,length,7);copy(code,{digit,0},{digits,7},1);
        emitCondCallBufferValue(code,helper(seven,"float_decimal_seven_digits"),2,digit,0,zero_,0);
        set(code,rounds,0);set(code,scale,0);set(code,tableIndex,0);
        Bytes scalePositive;copy(scalePositive,{rounds,0},{biasedExponent,0});add(scalePositive,rounds,65536-150);
        Bytes scaleNegative;copy(scaleNegative,{rounds,0},{biasedExponent,0});negate(scaleNegative,rounds);add(scaleNegative,rounds,150);
        copy(scaleNegative,{scale,0},{biasedExponent,0});add(scaleNegative,scale,65536-150);emitAdjustMultiLiteral(scaleNegative,tableIndex,kOpSet,1,{1});
        emitCondCallBufferValue(code,helper(scalePositive,"float_decimal_scale_positive"),7,biasedExponent,0,oneFifty,0);
        emitCondCallBufferValue(code,helper(scaleNegative,"float_decimal_scale_negative"),4,biasedExponent,0,oneFifty,0);
        emitCondCallBufferValue(code,scaleLoop,3,rounds,0,zero_,0);
        set(code,exponent,0);copy(code,{exponent,0},{length,0});add(code,exponent,65535);emitAdjustMultiBufferFetched(code,exponent,kOpAddWithCarry,0,2,scale,0);
        emitAdjustMultiLiteral(code,significant,kOpSet,0,Bytes(11,0));set(code,count,0);copy(code,{remaining,0},{length,0});copy(code,{cursor,0},{length,0});add(code,cursor,65535);
        append(code,call(extractLoop));emitCondCallBufferValue(code,roundID,3,remaining,0,zero_,0);
        copy(code,{cursor,0},{count,0});add(code,cursor,65535);append(code,call(trimLoop));
        set(code,exponentSign,0);copy(code,{exponentSign,0},{exponent,1},1);emitAdjustMultiLiteral(code,exponentSign,5,0,{128});
        copy(code,{exponentMagnitude,0},{exponent,0});emitCondCallBufferValue(code,absExponentID,3,exponentSign,0,zero_,0);
        set(code,scientificFlag,0);emitCondCallBufferValue(code,minusRangeID,3,exponentSign,0,zero_,0);emitCondCallBufferValue(code,plusRangeID,2,exponentSign,0,zero_,0);
        emitCondCallBufferValue(code,scientificID,3,scientificFlag,0,zero_,0);emitCondCallBufferValue(code,fixedID,2,scientificFlag,0,zero_,0);
        Bytes signText;put(signText,'-');emitCondCallBufferValue(code,helper(signText,"float_decimal_sign_text"),3,f.negative,0,zero_,0);
        f.entry=helper(code,"float_decimal_format");return f;
    }
    void printFloat(Bytes &code,const Expression &value) {
        const auto input=expression(code,value);auto &f=floatFormatter();
        copy(code,{f.input,0},input,4);append(code,call(f.entry));propagate(code);
        const auto text=data(Bytes(16,0),"str_float_temporary");copy(code,{text,0},{f.text,0},16);
        Bytes invoke;emitBufferHeader(invoke,text);invoke.insert(invoke.end(),{11,0,0,0});
        const auto action=helper(invoke,"print_float_text");copy(code,{action,6},{f.first,0});append(code,call(action));
        emitAdjustMultiLiteral(code,text,kOpSet,0,Bytes(16,0));
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
                    if(statement.printing) {
                        if(statement.value) {
                            if(statement.value->type==Type::F32)printFloat(code,statement.value);
                            else printInteger(code,statement.value);
                        }
                        else code.insert(code.end(),statement.text.begin(),statement.text.end());
                        continue;
                    }
                    if(statement.paired) {
                        const auto n=expression(code,statement.left);
                        const auto d=expression(code,statement.right);
                        if(wide(statement.left->type)) {
                            const auto pair=wideDivmod(code,statement.left->type,n,d);
                            admit(code,pair.first,program_.globals[statement.destination]);admit(code,pair.second,program_.globals[statement.remainder]);
                            copy(code,{layout_.buffer,layout_.offsets[statement.destination]},pair.first,4);
                            copy(code,{layout_.buffer,layout_.offsets[statement.remainder]},pair.second,4);continue;
                        }
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
                    if(statement.index) {
                        const auto offset=arrayOffset(code,statement.destination,statement.index);
                        const auto candidate=expression(code,statement.value);Bytes store;
                        copy(store,{layout_.arrayBuffers.at(statement.destination),0},candidate,width(program_.arrays[statement.destination].type));
                        const auto action=helper(store,"array_write");copy(code,{action,7},offset);
                        append(code,call(action));continue;
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
            output_.programs[item.first] = std::move(code);
        }
        std::set<unsigned> owned(output_.owned.begin(), output_.owned.end());
        for (const auto &claim : allocation_.claims())
            if (!claim.second.external && claim.second.owner.rfind("dual.native.", 0) != 0) owned.insert(claim.first);
        output_.owned.assign(owned.begin(), owned.end());
        for (auto id : output_.owned) append(output_.cleanup, clear(id));
        output_.ordinaryData=compiler_.initialBuffers();
        append(output_.install, output_.cleanup); append(output_.install, compiler_.out);
        for (const auto &item : output_.data) emitWriteBlock(output_.install, item.first, item.second);
        for (const auto &item : output_.programs) emitWriteBlock(output_.install, item.first, item.second);
        return output_;
    }
};
inline std::string nativeC(const Program &program, const Layout &layout) {
    if(containsPrint(program))throw std::runtime_error("print currently requires the stock buffered target");
    std::function<bool(const Expression&)> usesWideExpression=[&](const Expression&e){return e&&(wide(e->type)||usesWideExpression(e->left)||usesWideExpression(e->right));};
    std::function<bool(const std::vector<Assignment>&)> usesWideBody=[&](const auto&body){
        for(const auto&s:body)if(usesWideExpression(s.value)||usesWideExpression(s.left)||usesWideExpression(s.right)||usesWideExpression(s.index)||usesWideBody(s.yes)||usesWideBody(s.no))return true;
        return false;
    };
    bool usesWide=false;for(const auto&g:program.globals)usesWide|=wide(g.type);
    for(const auto&a:program.arrays)usesWide|=wide(a.type);
    for(const auto&p:program.procedures)usesWide|=usesWideBody(p.statements);
    std::ostringstream out;
    if(requiresNativeMath(program))out << "#include <math.h>\n";
    out << "/* Generated from the shared Golem model; module state stays in VDP buffers. */\n"
        << "#include \"vdplet.h\"\nstatic volatile uint32_t identity=0x474c3332u;\n"
        << "static uint16_t get(const uint8_t*p){return (uint16_t)p[0]|(uint16_t)p[1]<<8;}\n"
        << "static void put(uint8_t*p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}\n"
        << "static inline int32_t signed16(uint32_t v){return (int32_t)v-((v&32768u)?65536:0); }\n"
        << (usesWide?"static inline int32_t signed32(uint32_t v){return v>2147483647u?-1-(int32_t)(4294967295u-v):(int32_t)v;}\n":"")
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
        if(wide(global.type)) {
            const auto decoded=global.type==Type::S32?"signed32("+value+")":value;
            if(global.low>lowInteger(global.type))out << "if((int64_t)(" << decoded << ")<" << static_cast<int64_t>(global.low) << "ll)FAIL(32);\n";
            if(global.high<highInteger(global.type))out << "if((int64_t)(" << decoded << ")>" << static_cast<int64_t>(global.high) << "ll)FAIL(32);\n";
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
        admit(program.globals[i], std::string(width(program.globals[i].type)==4?"get32(state+":"get(state+") + std::to_string(layout.offsets[i]) + ")",true);
    out << "switch(r->entry_id){\n";
    unsigned temporary = 0;
    std::function<std::string(const Expression &)> expression = [&](const Expression &e) -> std::string {
        if (e->constant) return std::to_string(e->constant->bits) + "u";
        if (e->op == Op::Global) {
            const auto result = "v" + std::to_string(temporary++);
            out << "uint32_t " << result << "=" << (width(e->type)==4?"get32":"get") << "(state+" << layout.offsets[e->global] << ");\n";
            if(e->type==Type::F32) out << "FP_RESULT(" << result << ");\n";
            return result;
        }
        if(e->op==Op::ArrayRead) {
            const auto raw=expression(e->left);const auto index="index"+std::to_string(temporary++);
            const auto&a=program.arrays.at(e->global);const auto result="v"+std::to_string(temporary++);
            out << "int32_t " << index << "=" << (e->left->type==Type::S16?"signed16("+raw+")":"(int32_t)("+raw+")") << ";\n"
                << "if(" << index << '<' << a.lower << "||" << index << '>' << a.upper << ")FAIL(5);\n"
                << "uint32_t " << result << ";{vdplet_view_v1 a" << result << "={sizeof(a" << result << "),0,0,0,0};\n"
                << "rc=h->buffer_acquire(h->context," << layout.arrayBuffers.at(e->global) << ",VDPL_READ,(uint32_t)(" << index << "-(" << a.lower << "))*" << width(a.type) << "u," << width(a.type) << ",&a" << result << ");\n"
                << "if(rc){ROOT_FAIL(8,rc);}\n" << result << "=" << (width(e->type)==4?"get32":"get") << "(a" << result << ".data);\n"
                << "rc=h->buffer_release(h->context,a" << result << ".lease);if(rc){ROOT_FAIL(8,rc);}}\n";
            return result;
        }
        const auto left = expression(e->left);
        if(e->op==Op::Convert) {
            if(wide(e->type)||wide(e->left->type)) {
                const auto decoded=e->left->type==Type::S32?"signed32("+left+")":e->left->type==Type::S16?"signed16("+left+")":left;
                const auto result="v"+std::to_string(temporary++);
                out << "int64_t n" << result << "=(int64_t)(" << decoded << ");\n"
                    << "if(n" << result << '<' << lowInteger(e->type) << "ll||n" << result << '>' << highInteger(e->type) << "ll)FAIL(32);\n"
                    << "uint32_t " << result << "=(uint32_t)n" << result;
                if(width(e->type)==2)out << "&65535u";
                out << ";\n";return result;
            }
            if(e->type==Type::F32 && e->left->type!=Type::F32) {
                const auto result="v"+std::to_string(temporary++);
                out << "uint32_t " << result << "=fbits((float)" << (e->left->type==Type::S32?"signed32("+left+")":e->left->type==Type::S16?"signed16("+left+")":left) << ");\n";
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
        if(e->op==Op::BitNot) {
            const auto result="v"+std::to_string(temporary++);
            out << "uint32_t " << result << "=(~" << left << ")";
            if(width(e->type)==2)out << "&65535u";
            out << ";\n";return result;
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
        if(e->op==Op::Sin||e->op==Op::Cos||e->op==Op::Floor) {
            const auto result="v"+std::to_string(temporary++);
            out << "uint32_t " << result << "=fbits(" << (e->op==Op::Sin?"sinf":e->op==Op::Cos?"cosf":"floorf")
                << "(fvalue(" << left << ")));\nFP_RESULT(" << result << ");\n";
            return result;
        }
        const auto right = expression(e->right);
        const auto result = "v" + std::to_string(temporary++);
        if (e->op == Op::Compare) {
            if(e->left->type==Type::F32) {
                out << "uint32_t " << result << "=(ordered(" << left << ")" << e->relation << "ordered(" << right << "))?1u:0u;\n";
                return result;
            }
            out << "uint32_t " << result << "=(" << (e->left->type==Type::S32?"signed32("+left+")":e->left->type==Type::S16?"signed16("+left+")":left)
                << e->relation << (e->right->type==Type::S32?"signed32("+right+")":e->right->type==Type::S16?"signed16("+right+")":right) << ")?1u:0u;\n";
            return result;
        }
        if(e->op==Op::Xor || e->op==Op::BitAnd || e->op==Op::BitOr) {
            out << "uint32_t " << result << "=(" << left << (e->op==Op::BitAnd?"&":e->op==Op::BitOr?"|":"^") << right << ")";
            if(width(e->type)==2)out << "&65535u";
            out << ";\n";return result;
        }
        if(e->op==Op::ShiftRight) {
            out << "if(" << right << ">31u)FAIL(32);\nuint32_t " << result << "=" << left << ">>" << right << ";\n";
            if(e->type==Type::S32)out << "if((" << left << "&2147483648u)&&" << right << ")" << result << "|=4294967295u<<(32u-" << right << ");\n";
            return result;
        }
        if(wide(e->type)) {
            if(e->type==Type::S32) {
                out << "int64_t n" << result << "=(int64_t)signed32(" << left << ")" << (e->op==Op::Add?'+':e->op==Op::Subtract?'-':'*') << "(int64_t)signed32(" << right << ");\n"
                    << "if(n" << result << " < -2147483648ll || n" << result << " >2147483647ll)FAIL(" << (e->op==Op::Add?1:e->op==Op::Subtract?2:3) << ");\n"
                    << "uint32_t " << result << "=(uint32_t)n" << result << ";\n";
            } else if(e->op==Op::Subtract) {
                out << "if(" << left << '<' << right << ")FAIL(2);\nuint32_t " << result << '=' << left << '-' << right << ";\n";
            } else {
                out << "uint64_t n" << result << "=(uint64_t)(" << left << ")" << (e->op==Op::Multiply?'*':'+') << "(uint64_t)(" << right << ");\n"
                    << "if(n" << result << ">4294967295ull)FAIL(" << (e->op==Op::Multiply?3:1) << ");\nuint32_t " << result << "=(uint32_t)n" << result << ";\n";
            }
            return result;
        }
        if(e->type==Type::F32) {
            if(e->op==Op::Divide)out << "if(!(" << right << "&2147483647u))FAIL(4);\n";
            out << "uint32_t " << result << "=fbits(fvalue(" << left << ")" << (e->op==Op::Add?'+':e->op==Op::Subtract?'-':e->op==Op::Divide?'/':'*') << "fvalue(" << right << "));\nFP_RESULT(" << result << ");\n";
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
                if(statement.left->type==Type::S32) {
                    out << "if(" << n << "==2147483648u && " << d << "==4294967295u)FAIL(6);\n"
                        << "uint32_t " << q << "=(uint32_t)(signed32(" << n << ")/signed32(" << d << ")),"
                        << r << "=(uint32_t)(signed32(" << n << ")%signed32(" << d << "));\n";
                } else out << "uint32_t " << q << "=" << n << "/" << d << "," << r << "=" << n << "%" << d << ";\n";
                admit(program.globals[statement.destination],q); admit(program.globals[statement.remainder],r);
                const auto store=wide(statement.left->type)?"put32":"put";
                out << store << "(state+" << layout.offsets[statement.destination] << "," << q << ");\n"
                    << store << "(state+" << layout.offsets[statement.remainder] << "," << r << ");\n";
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
                if(statement.index) {
                    const auto raw=expression(statement.index);const auto index="index"+std::to_string(temporary++);
                    const auto&a=program.arrays.at(statement.destination);const auto lease="store"+std::to_string(temporary++);
                    out << "int32_t " << index << "=" << (statement.index->type==Type::S16?"signed16("+raw+")":"(int32_t)("+raw+")") << ";\n"
                        << "if(" << index << '<' << a.lower << "||" << index << '>' << a.upper << ")FAIL(5);\n";
                    const auto candidate=expression(statement.value);
                    out << "{vdplet_view_v1 " << lease << "={sizeof(" << lease << "),0,0,0,0};\n"
                        << "rc=h->buffer_acquire(h->context," << layout.arrayBuffers.at(statement.destination) << ",VDPL_WRITE,(uint32_t)(" << index << "-(" << a.lower << "))*" << width(a.type) << "u," << width(a.type) << ",&" << lease << ");\n"
                        << "if(rc){ROOT_FAIL(8,rc);}\n" << (width(a.type)==4?"put32":"put") << "(" << lease << ".data," << candidate << ");\n"
                        << "rc=h->buffer_release(h->context," << lease << ".lease);if(rc){ROOT_FAIL(8,rc);}}\n";
                    continue;
                }
                const auto candidate = expression(statement.value);
                admit(program.globals[statement.destination], candidate);
                out << (width(program.globals[statement.destination].type)==4?"put32":"put") << "(state+" << layout.offsets[statement.destination] << "," << candidate << ");\n";
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

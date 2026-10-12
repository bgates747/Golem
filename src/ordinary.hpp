#pragma once
// Ordinary parsing/emission shared by raw and resident compilation.
// Supports literal print, uint16 declarations/assignment/addition and For/Next.
// A For counter must be declared first. Each loop body ends in a conditional
// self-call that the stock VDP converts to a jump, bounding call-stack use.
// Raw output remains a payload for upload and call through the existing loader.
#include "token_reader.hpp"
#include "buffer_allocator.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace golem::ordinary {

using CompileError = golem::syntax::Error;
using Reader = golem::syntax::Reader;
using Kind = golem::syntax::Kind;

// --- Buffered Commands API byte-level emission helpers ---
//
// These emit real VDU Buffered Commands API command bytes (see
// /home/smith/Agon/agon-docs/docs/vdp/Buffered-Commands-API.md) directly
// into the compiled program's own byte stream - the compiled program
// buffer is itself replayed as a raw VDU byte stream when called (see
// program.asm's header comment), so these commands are just as valid to
// embed there as the literal print-text bytes are.

inline void emitLE16(std::vector<unsigned char> &out, unsigned value) {
    out.push_back(static_cast<unsigned char>(value & 0xFF));
    out.push_back(static_cast<unsigned char>((value >> 8) & 0xFF));
}

// `VDU 23, 0, &A0, bufferId;` - the common header for every Buffered
// Commands API command.
inline void emitBufferHeader(std::vector<unsigned char> &out, int bufferId) {
    out.push_back(23);
    out.push_back(0);
    out.push_back(0xA0);
    emitLE16(out, static_cast<unsigned>(bufferId));
}

// Command 0: write a data block to a buffer (this also creates the buffer
// if it doesn't already exist yet - see command 0's docs).
inline void emitWriteBlock(std::vector<unsigned char> &out, int bufferId,
                    const std::vector<unsigned char> &data) {
    emitBufferHeader(out, bufferId);
    out.push_back(0);
    emitLE16(out, static_cast<unsigned>(data.size()));
    out.insert(out.end(), data.begin(), data.end());
}

constexpr unsigned char kOpSet = 2;
constexpr unsigned char kOpAddWithCarry = 4;

constexpr unsigned char kCmdCondCall = 6;          // Buffered Commands API command 6
constexpr unsigned char kCondLE = 6;               // VBUF_COND_LE
constexpr unsigned char kCondFlagBufValue = 0x20;  // VBUF_COND_F_BUFVALUE
constexpr unsigned char kCondFlag16Bit = 0x80;     // VBUF_COND_F_16BIT

// Command 5 (Adjust), "multiple target values" (&40) + "multiple operand
// values" (&80) modifiers, operand given as `literalOperand.size()`
// literal bytes (not buffer-fetched) - see
// docs/design/language-type-proposals.md's Axis 13 for why these two
// modifier bits together are used for every uint16-width (2-byte)
// Set/Add here, mirroring Buffered-Commands-API.md's chained-add example.
inline void emitAdjustMultiLiteral(std::vector<unsigned char> &out, int bufferId,
                             unsigned char operationBase, unsigned offset,
                             const std::vector<unsigned char> &literalOperand) {
    emitBufferHeader(out, bufferId);
    out.push_back(5);
    out.push_back(static_cast<unsigned char>(operationBase | 0x40 | 0x80));
    emitLE16(out, offset);
    emitLE16(out, static_cast<unsigned>(literalOperand.size()));
    out.insert(out.end(), literalOperand.begin(), literalOperand.end());
}

// Command 5 (Adjust), "multiple target values" + "multiple operand
// values" + "operand is a buffer-fetched value" modifiers - a genuine
// buffer-to-buffer, byte-for-byte Set/Add-with-carry, mirroring
// Buffered-Commands-API.md's cross-buffer AND example (operation `AND`
// there; `Set`/`Add with carry` used the same way here - see Axis 13's
// note that this specific combination isn't spelled out verbatim for
// `Set` in the prose docs, and is worth confirming against
// vdu_buffered.h).
inline void emitAdjustMultiBufferFetched(std::vector<unsigned char> &out, int bufferId,
                                   unsigned char operationBase, unsigned offset,
                                   unsigned count, int srcBufferId, unsigned srcOffset) {
    emitBufferHeader(out, bufferId);
    out.push_back(5);
    out.push_back(static_cast<unsigned char>(operationBase | 0x40 | 0x80 | 0x20));
    emitLE16(out, offset);
    emitLE16(out, count);
    emitLE16(out, static_cast<unsigned>(srcBufferId));
    emitLE16(out, srcOffset);
}

// Command 6 (Conditionally call a buffer), buffer-fetched-operand + 16-bit
// form: compares two REAL buffers against each other, never a raw buffer
// byte against a literal operand.
//
// This is a hard correctness requirement, not a style choice, found while
// hand-verifying the loop mechanism this codegen mirrors (see
// docs/devlog/2026-07-29-loop-tail-call-prototype.md): `vdu_buffered.h`'s
// bufferConditional() initialises its internal `sourceValue`/`operandValue`
// accumulators to -1 (all bits set), then for a "basic" (non-buffer-value)
// check only overwrites the low 1-2 bytes via a plain `memcpy` - the
// untouched upper bytes stay 0xFF. A checked byte/word compared against a
// literal is therefore always contaminated on one side only (the literal
// operand's upper bits are clean, the buffer-read side's are not), making
// ordering/zero comparisons unreliable. Comparing two *buffer-fetched*
// values instead contaminates both sides identically (same -1 init, same
// partial-write pattern), so the contamination is a shared additive
// offset that cancels out of every comparison operator (equality AND
// ordering alike) - this is the only reliable form for a raw numeric
// buffer compare for ordinary values. The raw 0xffff word still collides
// with the firmware's -1 invalid-operand sentinel; GOLEM-004 records this
// existing loop-domain limitation. This parser tranche preserves emission.
inline void emitCondCallBufferValue(std::vector<unsigned char> &out, int targetBufferId,
                              unsigned char condOpBase, int checkBufferId,
                              unsigned checkOffset, int operandBufferId,
                              unsigned operandOffset) {
    emitBufferHeader(out, targetBufferId);
    out.push_back(kCmdCondCall);
    out.push_back(static_cast<unsigned char>(condOpBase | kCondFlagBufValue | kCondFlag16Bit));
    emitLE16(out, static_cast<unsigned>(checkBufferId));
    emitLE16(out, checkOffset);
    emitLE16(out, static_cast<unsigned>(operandBufferId));
    emitLE16(out, operandOffset);
}

inline std::vector<unsigned char> le16Bytes(unsigned value) {
    return {static_cast<unsigned char>(value & 0xFF),
            static_cast<unsigned char>((value >> 8) & 0xFF)};
}

// --- Symbol table / variable storage ---
//
// Each uint16 variable gets its own dedicated 2-byte buffer (offset 0),
// per Axis 13's "this toy compiler only" note: buffer IDs are abundant
// (65534 available per Buffered-Commands-API.md), so a shared/offset-
// tracked "globals arena" isn't needed yet to get real declarations
// working. Buffer ID 0 is reserved for the compiled program itself (see
// loader.asm's `program_buffer_id`), so variable/scratch buffer IDs start
// at 1 (`kAccBufferId`).

class Compiler {
public:
    std::vector<unsigned char> out;

    Compiler() { allocator_->claim(1, "ordinary.accumulator"); }
    explicit Compiler(golem::BufferAllocator &allocator) : allocator_(&allocator) {
        accBufferId_ = static_cast<int>(allocator_->allocate("ordinary.accumulator"));
    }
    int accumulatorId() const { return accBufferId_; }
    const std::map<std::string, int> &variables() const { return variables_; }
    const std::map<unsigned, unsigned> &dataBuffers() const { return dataBuffers_; }

    // 3 bytes: 2 data bytes + the carry-out landing byte "Add with carry"
    // always writes at offset+2 - see Axis 10's addendum and Axis 13.
    // Allocated lazily so print-only programs (like hello.golem) don't
    // pay for an unused ACC buffer.
    void ensureAcc() {
        if (!accInitialized_) {
            emitWriteBlock(out, accBufferId_, {0, 0, 0});
            dataBuffers_[accBufferId_] = 3;
            accInitialized_ = true;
        }
    }

    int declareVariable(const std::string &name, const golem::syntax::Position &at) {
        if (golem::syntax::reserved(name)) {
            golem::syntax::fail(at, "reserved identifier '" + name + "'");
        }
        if (variables_.count(name)) {
            golem::syntax::fail(at, "redeclaration of variable '" + name + "'");
        }
        int bufferId;
        try { bufferId = static_cast<int>(allocator_->allocate("ordinary.global." + name)); }
        catch (const golem::AllocationError &error) { golem::syntax::fail(at, error.what()); }
        variables_[name] = bufferId;
        dataBuffers_[bufferId] = 2;
        return bufferId;
    }

    int lookupVariable(const std::string &name, const golem::syntax::Position &at) const {
        auto it = variables_.find(name);
        if (it == variables_.end()) {
            golem::syntax::fail(at, "use of undeclared variable '" + name + "'");
        }
        return it->second;
    }

    // Allocates a fresh buffer ID for compiler-internal use (e.g. a
    // `For` loop's body buffer or upper-bound-constant buffer) - not
    // associated with any user-visible variable name.
    int allocInternalBuffer(const golem::syntax::Position &at) {
        try { return static_cast<int>(allocator_->allocate("ordinary.loop")); }
        catch (const golem::AllocationError &error) { golem::syntax::fail(at, error.what()); }
    }

private:
    std::map<std::string, int> variables_;
    golem::BufferAllocator localAllocator_;
    golem::BufferAllocator *allocator_ = &localAllocator_;
    std::map<unsigned, unsigned> dataBuffers_; // Only materialized ordinary data.
    int accBufferId_ = 1; // Default raw-payload compatibility policy.
    bool accInitialized_ = false;
};

// A single term in a `uint16` expression: either a literal value or a
// reference to an already-declared variable's buffer.
struct Term {
    bool isLiteral = false;
    unsigned literalValue = 0;
    int varBufferId = 0;
};

// Parses `term ('+' term)*` where `term` is an identifier or an integer
// literal - see docs/design/language-type-proposals.md Axis 10's
// addendum ("Golem does allow multiple operations within a single
// expression").
inline std::vector<Term> parseExpr(Reader &reader, const Compiler &compiler) {
    std::vector<Term> terms;
    do {
        Term term;
        if (reader.peek().kind == Kind::Number) {
            term.isLiteral = true;
            term.literalValue = reader.uint16();
        } else {
            if (reader.peek().kind != Kind::Identifier || golem::syntax::reserved(reader.peek().text))
                reader.error("expected expression operand (integer or declared uint16 variable)");
            auto name = reader.pop();
            term.varBufferId = compiler.lookupVariable(name.text, name.begin);
        }
        terms.push_back(term);
    } while (reader.take('+'));
    return terms;
}

// Compiles `terms` (already parsed by parseExpr) so their sum ends up in
// the 2-byte buffer `destBufferId` at offset 0, emitting into `out`
// (the surrounding buffer currently being compiled - the top-level
// program buffer, or a `For` loop's own body buffer). Implements Axis 13's
// addition-primitive design: a single term is `Set` directly into the
// destination (`Set` never produces a trailing side-effect byte, so this
// is always safe); two or more terms are combined in the dedicated `ACC`
// scratch buffer first (since `Add with carry` - needed for correct
// multi-byte addition - always writes a carry-out byte immediately after
// its target, which would otherwise silently clobber whichever variable
// happens to be laid out next), and only the final 2-byte result is
// copied into the real destination.
inline void compileExprInto(Compiler &compiler, std::vector<unsigned char> &out, int destBufferId,
                      const std::vector<Term> &terms) {
    auto setInto = [&](int bufferId, const Term &term) {
        if (term.isLiteral) {
            emitAdjustMultiLiteral(out, bufferId, kOpSet, 0, le16Bytes(term.literalValue));
        } else {
            emitAdjustMultiBufferFetched(out, bufferId, kOpSet, 0, 2, term.varBufferId, 0);
        }
    };
    auto addInto = [&](int bufferId, const Term &term) {
        if (term.isLiteral) {
            emitAdjustMultiLiteral(out, bufferId, kOpAddWithCarry, 0,
                                    le16Bytes(term.literalValue));
        } else {
            emitAdjustMultiBufferFetched(out, bufferId, kOpAddWithCarry, 0, 2,
                                          term.varBufferId, 0);
        }
    };

    if (terms.size() == 1) {
        setInto(destBufferId, terms[0]);
        return;
    }

    compiler.ensureAcc();
    setInto(compiler.accumulatorId(), terms[0]);
    for (size_t i = 1; i < terms.size(); ++i) {
        addInto(compiler.accumulatorId(), terms[i]);
    }
    emitAdjustMultiBufferFetched(out, destBufferId, kOpSet, 0, 2, compiler.accumulatorId(), 0);
}

// Parsing preserves the vector-of-terms emission boundary and allocation order.
inline void compileBlock(Compiler &compiler, Reader &reader, std::vector<unsigned char> &out);

inline void compileStatement(Compiler &compiler, Reader &reader, std::vector<unsigned char> &out) {
    if (reader.takeKeyword("print")) {
        if (reader.peek().kind != Kind::String) reader.error("expected string literal after 'print'");
        auto text = reader.pop().text;
        out.insert(out.end(), text.begin(), text.end());
    } else if (reader.takeKeyword("uint16")) {
        auto name = reader.name();
        int bufferId = compiler.declareVariable(name.text, name.begin);
        if (reader.take('=')) {
            auto terms = parseExpr(reader, compiler);
            if (terms.size() == 1 && terms[0].isLiteral) {
                emitWriteBlock(out, bufferId, le16Bytes(terms[0].literalValue));
            } else {
                emitWriteBlock(out, bufferId, {0, 0});
                compileExprInto(compiler, out, bufferId, terms);
            }
        } else {
            emitWriteBlock(out, bufferId, {0, 0});
        }
    } else if (reader.takeKeyword("For")) {
        auto loopVar = reader.name();
        int iBufferId = compiler.lookupVariable(loopVar.text, loopVar.begin);
        reader.need('=');
        auto startTerms = parseExpr(reader, compiler);
        reader.needKeyword("To");
        auto boundTerms = parseExpr(reader, compiler);
        compileExprInto(compiler, out, iBufferId, startTerms);
        int boundBufferId = compiler.allocInternalBuffer(loopVar.begin);
        if (boundTerms.size() == 1 && boundTerms[0].isLiteral) {
            emitWriteBlock(out, boundBufferId, le16Bytes(boundTerms[0].literalValue));
        } else {
            emitWriteBlock(out, boundBufferId, {0, 0});
            compileExprInto(compiler, out, boundBufferId, boundTerms);
        }
        int bodyBufferId = compiler.allocInternalBuffer(loopVar.begin);
        std::vector<unsigned char> bodyOut;
        compileBlock(compiler, reader, bodyOut);
        if (!reader.takeKeyword("Next")) reader.error("expected 'Next' to close 'For " + loopVar.text + "'");
        const auto closing = reader.peek();
        auto closingName = reader.name();
        if (closingName.text != loopVar.text)
            golem::syntax::fail(closing.begin, "'Next " + closingName.text +
                                "' does not match 'For " + loopVar.text + "'");
        compileExprInto(compiler, bodyOut, iBufferId, {Term{false, 0, iBufferId}, Term{true, 1, 0}});
        // The trailing conditional self-call remains the last body command.
        emitCondCallBufferValue(bodyOut, bodyBufferId, kCondLE, iBufferId, 0, boundBufferId, 0);
        emitWriteBlock(out, bodyBufferId, bodyOut);
        emitCondCallBufferValue(out, bodyBufferId, kCondLE, iBufferId, 0, boundBufferId, 0);
    } else if (reader.peek().kind == Kind::Identifier) {
        if (golem::syntax::reserved(reader.peek().text))
            reader.error("unsupported statement or unexpected block delimiter '" + reader.peek().text + "'");
        auto name = reader.pop();
        int bufferId = compiler.lookupVariable(name.text, name.begin);
        reader.need('=');
        auto terms = parseExpr(reader, compiler);
        compileExprInto(compiler, out, bufferId, terms);
    } else {
        reader.error("expected a statement ('print', 'uint16', 'For', or an assignment)");
    }
}

inline void compileBlock(Compiler &compiler, Reader &reader, std::vector<unsigned char> &out) {
    while (!reader.end() && !reader.keyword("Next")) compileStatement(compiler, reader, out);
}

inline std::vector<unsigned char> compile(const std::string &src) {
    Compiler compiler;
    Reader reader(src);
    compileBlock(compiler, reader, compiler.out);
    if (!reader.end()) reader.error("unexpected 'Next' with no matching 'For'");
    return compiler.out;
}

inline std::string readFile(const std::string &path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw CompileError("could not open input file: " + path);
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

} // namespace golem::ordinary

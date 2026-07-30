// golemc.cpp
//
// Toy Golem compiler (proof of concept).
//
// Supports:
//   print "string literal";
//   uint16 <ident> [= expr];        (naked declaration defaults to 0)
//   <ident> = expr;
//   For <ident> = expr To expr; <statements> Next <ident>;
// where expr := term ('+' term)*, term := ident | integer-literal, and
// every *statement* requires a trailing `;` (not every *line* - newlines
// are not significant; see docs/design/language-type-proposals.md's
// statement-terminator note, Axis 5, for why: with only one statement
// form an optional terminator was harmless, but once more statement
// forms exist that don't each start with a unique leading keyword, an
// optional terminator becomes ambiguous. A mandatory semicolon keeps the
// grammar unambiguous for a simple recursive-descent parser, Pascal/
// Wirth-family style, with no JS-style automatic-semicolon-insertion
// footguns.). Per Axis 15's now-settled "every statement ends in `;`,
// including block-delimiting keyword statements themselves" note, a
// `For` loop's header line and its `Next <ident>` closer each end in
// their own `;` too - `For i = 1 To 50; ... Next i;`.
//
// The `For` loop's variable must already be declared (`uint16 i;`) -
// `For` never doubles as a declaration form (Author's stated preference:
// keep declaration and loop-iteration concerns separate). Its codegen
// mirrors examples/loop_test/loop_test.asm's hand-verified mechanism
// exactly: the loop body compiles into its own dedicated buffer ending
// in a trailing conditional self-call, which the VDP automatically
// converts into a jump (see docs/devlog/2026-07-29-loop-tail-call-
// prototype.md), so the loop costs one real call frame no matter how
// many iterations it runs. The continuation check (`i <= bound`) always
// compares two real buffers against each other (the loop variable's
// buffer and a dedicated buffer holding the upper bound), never a raw
// buffer byte against a literal operand - see emitCondCallBufferValue's
// comment below for why that's a hard *correctness* requirement, not a
// style choice: `vdu_buffered.h`'s bufferConditional() leaves the upper
// bytes of its internal comparison value contaminated with a sentinel
// unless both sides are buffer-fetched.
//
// `print` still only accepts a string literal - printing a `uint16`
// (`print str(qux);`) is NOT implemented yet; see Axis 7/8's still-open
// questions (no first-class `string` type, no int-to-decimal runtime
// routine) - that's real follow-on work, not done here.
//
// File extensions are purely a project convention, never enforced here:
// golemc reads whatever bytes are at argv[1] and writes whatever bytes it
// produces to argv[2], with zero extension-based logic anywhere in this
// file - a `.golem`, `.txt`, or extensionless source file all compile
// identically, and the output can be written to a `.golemb` (informal
// project convention for a compiled Golem binary - short for "Golem
// binary"; not required, and this compiler doesn't care what you call it),
// `.bin`, or any other name/extension a given loader happens to expect.
//
// Compiles a .golem source file down to the *compiled program* format
// demonstrated by examples/hello_golem/program.asm: a flat binary blob
// containing the raw bytes that get written into a VDP buffer and called.
// `print`'s codegen is literal-byte passthrough (see that file's header
// comment for why printable text needs no escape interpretation to become
// "print this" when replayed via the Buffered Commands API's buffer-call
// mechanism). Declarations/assignments/addition, by contrast, emit real
// VDU Buffered Commands API command bytes (command 0 "write block",
// command 5 "adjust buffer contents") directly into that same byte
// stream - this is just as valid, since the compiled program buffer is
// itself replayed as a raw VDU byte stream when called, so an embedded
// `VDU 23, 0, &A0, ...` command sequence works exactly like the literal
// print-text bytes do. See docs/design/language-type-proposals.md's
// Axis 13 for the concrete design (the addition primitive, its
// dedicated-accumulator storage model, and the exact wire encoding used).
//
// This is NOT the real Golem compiler - it has no general lexer/parser,
// no widths beyond `uint16`, no comparisons/control flow, no `str()`.
// It exists to close the loop first opened by examples/hello_golem/: that
// example's program.bin was hand-authored to *stand in* for compiler
// output; this program is a genuine (if minimal) compiler that produces
// the same kind of output for real, from real .golem source.
//
// Usage: golemc <input.golem> <output.bin>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct CompileError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

int lineNumberAt(const std::string &src, size_t pos) {
    return 1 + static_cast<int>(std::count(src.begin(), src.begin() + pos, '\n'));
}

// Advance pos past any whitespace and `//`-to-end-of-line comments. This
// toy grammar has no statement terminators other than the `print` keyword
// itself (plus an optional `;`), so newlines are not otherwise significant.
void skipWhitespaceAndComments(const std::string &src, size_t &pos) {
    for (;;) {
        while (pos < src.size() && std::isspace(static_cast<unsigned char>(src[pos]))) {
            ++pos;
        }
        if (pos + 1 < src.size() && src[pos] == '/' && src[pos + 1] == '/') {
            while (pos < src.size() && src[pos] != '\n') {
                ++pos;
            }
            continue;
        }
        break;
    }
}

// Parse a double-quoted string literal starting at src[pos] (which must be
// '"'). Supports \\, \", and \n escapes only. Advances pos past the
// closing quote and returns the literal's decoded bytes.
std::string parseStringLiteral(const std::string &src, size_t &pos) {
    int startLine = lineNumberAt(src, pos);
    if (pos >= src.size() || src[pos] != '"') {
        throw CompileError("line " + std::to_string(startLine) +
                            ": expected '\"' to start a string literal");
    }
    ++pos;
    std::string out;
    for (;;) {
        if (pos >= src.size()) {
            throw CompileError("line " + std::to_string(startLine) +
                                ": unterminated string literal");
        }
        char c = src[pos];
        if (c == '"') {
            ++pos;
            break;
        }
        if (c == '\\' && pos + 1 < src.size()) {
            char next = src[pos + 1];
            if (next == '"' || next == '\\') {
                out.push_back(next);
                pos += 2;
                continue;
            }
            if (next == 'n') {
                out.push_back('\n');
                pos += 2;
                continue;
            }
        }
        out.push_back(c);
        ++pos;
    }
    return out;
}

bool isIdentStart(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}

bool isIdentCont(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

std::string parseIdentifier(const std::string &src, size_t &pos) {
    int startLine = lineNumberAt(src, pos);
    if (pos >= src.size() || !isIdentStart(src[pos])) {
        throw CompileError("line " + std::to_string(startLine) + ": expected an identifier");
    }
    size_t start = pos;
    ++pos;
    while (pos < src.size() && isIdentCont(src[pos])) {
        ++pos;
    }
    return src.substr(start, pos - start);
}

// Parses an unsigned decimal integer literal, range-checked to fit a
// uint16 (0-65535) - see docs/design/language-type-proposals.md's Axis 2
// static/range-checked typing lean.
unsigned parseUint16Literal(const std::string &src, size_t &pos) {
    int startLine = lineNumberAt(src, pos);
    if (pos >= src.size() || !std::isdigit(static_cast<unsigned char>(src[pos]))) {
        throw CompileError("line " + std::to_string(startLine) + ": expected an integer literal");
    }
    size_t start = pos;
    while (pos < src.size() && std::isdigit(static_cast<unsigned char>(src[pos]))) {
        ++pos;
    }
    unsigned long value = std::stoul(src.substr(start, pos - start));
    if (value > 0xFFFFu) {
        throw CompileError("line " + std::to_string(startLine) + ": integer literal " +
                            std::to_string(value) + " does not fit in a uint16 (0-65535)");
    }
    return static_cast<unsigned>(value);
}

// --- Buffered Commands API byte-level emission helpers ---
//
// These emit real VDU Buffered Commands API command bytes (see
// /home/smith/Agon/agon-docs/docs/vdp/Buffered-Commands-API.md) directly
// into the compiled program's own byte stream - the compiled program
// buffer is itself replayed as a raw VDU byte stream when called (see
// program.asm's header comment), so these commands are just as valid to
// embed there as the literal print-text bytes are.

void emitLE16(std::vector<unsigned char> &out, unsigned value) {
    out.push_back(static_cast<unsigned char>(value & 0xFF));
    out.push_back(static_cast<unsigned char>((value >> 8) & 0xFF));
}

// `VDU 23, 0, &A0, bufferId;` - the common header for every Buffered
// Commands API command.
void emitBufferHeader(std::vector<unsigned char> &out, int bufferId) {
    out.push_back(23);
    out.push_back(0);
    out.push_back(0xA0);
    emitLE16(out, static_cast<unsigned>(bufferId));
}

// Command 0: write a data block to a buffer (this also creates the buffer
// if it doesn't already exist yet - see command 0's docs).
void emitWriteBlock(std::vector<unsigned char> &out, int bufferId,
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
void emitAdjustMultiLiteral(std::vector<unsigned char> &out, int bufferId,
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
void emitAdjustMultiBufferFetched(std::vector<unsigned char> &out, int bufferId,
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
// buffer compare.
void emitCondCallBufferValue(std::vector<unsigned char> &out, int targetBufferId,
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

std::vector<unsigned char> le16Bytes(unsigned value) {
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

// Reserved-keyword check, case-insensitive (Axis 18: keywords are
// case-insensitive, so a variable can't dodge the reservation by typing
// e.g. `Print` or `PRINT` instead of `print`).
bool isReservedKeyword(const std::string &name) {
    static const std::vector<std::string> kKeywords = {"print", "uint16", "for", "to", "next"};
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return std::find(kKeywords.begin(), kKeywords.end(), lower) != kKeywords.end();
}

class Compiler {
public:
    std::vector<unsigned char> out;

    // 3 bytes: 2 data bytes + the carry-out landing byte "Add with carry"
    // always writes at offset+2 - see Axis 10's addendum and Axis 13.
    // Allocated lazily so print-only programs (like hello.golem) don't
    // pay for an unused ACC buffer.
    void ensureAcc() {
        if (!accInitialized_) {
            emitWriteBlock(out, kAccBufferId, {0, 0, 0});
            accInitialized_ = true;
        }
    }

    int declareVariable(const std::string &name, int lineNo) {
        if (isReservedKeyword(name)) {
            throw CompileError("line " + std::to_string(lineNo) + ": '" + name +
                                "' is a reserved keyword and cannot be used as a variable name");
        }
        if (variables_.count(name)) {
            throw CompileError("line " + std::to_string(lineNo) +
                                ": redeclaration of variable '" + name + "'");
        }
        int bufferId = nextBufferId_++;
        variables_[name] = bufferId;
        return bufferId;
    }

    int lookupVariable(const std::string &name, int lineNo) const {
        auto it = variables_.find(name);
        if (it == variables_.end()) {
            throw CompileError("line " + std::to_string(lineNo) +
                                ": use of undeclared variable '" + name + "'");
        }
        return it->second;
    }

    // Allocates a fresh buffer ID for compiler-internal use (e.g. a
    // `For` loop's body buffer or upper-bound-constant buffer) - not
    // associated with any user-visible variable name.
    int allocInternalBuffer() { return nextBufferId_++; }

    static constexpr int kAccBufferId = 1;

private:
    std::map<std::string, int> variables_;
    int nextBufferId_ = 2; // 1 is reserved for ACC
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
std::vector<Term> parseExpr(const std::string &src, size_t &pos, const Compiler &compiler) {
    std::vector<Term> terms;
    for (;;) {
        int lineNo = lineNumberAt(src, pos);
        Term term;
        if (pos < src.size() && std::isdigit(static_cast<unsigned char>(src[pos]))) {
            term.isLiteral = true;
            term.literalValue = parseUint16Literal(src, pos);
        } else {
            std::string name = parseIdentifier(src, pos);
            term.isLiteral = false;
            term.varBufferId = compiler.lookupVariable(name, lineNo);
        }
        terms.push_back(term);
        skipWhitespaceAndComments(src, pos);
        if (pos < src.size() && src[pos] == '+') {
            ++pos;
            skipWhitespaceAndComments(src, pos);
            continue;
        }
        break;
    }
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
void compileExprInto(Compiler &compiler, std::vector<unsigned char> &out, int destBufferId,
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
    setInto(Compiler::kAccBufferId, terms[0]);
    for (size_t i = 1; i < terms.size(); ++i) {
        addInto(Compiler::kAccBufferId, terms[i]);
    }
    emitAdjustMultiBufferFetched(out, destBufferId, kOpSet, 0, 2, Compiler::kAccBufferId, 0);
}

// Compile Golem source into the raw payload-byte "compiled output" this toy
// model uses (see program.asm's header comment). Supports:
//   print "string literal";           (literal bytes, no CR/LF appended -
//                                       see Axis 21: consecutive `print`s
//                                       run together on one line unless the
//                                       literal itself includes `\n`)
//   uint16 <ident> [= expr];          (naked declaration defaults to 0)
//   <ident> = expr;
//   For <ident> = expr To expr; <statements> Next <ident>;
// where expr := term ('+' term)*, term := ident | integer-literal.

bool matchesKeywordAt(const std::string &src, size_t pos, const std::string &keyword) {
    // Keywords are matched case-insensitively (see
    // docs/design/language-type-proposals.md's Axis 18) - `for`, `For`,
    // and `FOR` are all the same keyword. Identifiers (variable names)
    // are NOT case-folded here; `parseIdentifier`/`declareVariable`/
    // `lookupVariable` compare/store names exactly as written.
    if (pos + keyword.size() > src.size()) {
        return false;
    }
    for (size_t i = 0; i < keyword.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(src[pos + i])) !=
            std::tolower(static_cast<unsigned char>(keyword[i]))) {
            return false;
        }
    }
    return pos + keyword.size() >= src.size() || !isIdentCont(src[pos + keyword.size()]);
}

void compileBlock(Compiler &compiler, const std::string &src, size_t &pos,
                   std::vector<unsigned char> &out);

// Compiles exactly one statement, emitting into `out`, and consumes that
// statement's own trailing `;` (per Axis 5/Axis 15: every *statement* -
// including a `For` header line and its `Next` closer - ends in `;`,
// independent of line breaks).
void compileStatement(Compiler &compiler, const std::string &src, size_t &pos,
                       std::vector<unsigned char> &out) {
    static const std::string kPrint = "print";
    static const std::string kUint16 = "uint16";
    static const std::string kFor = "For";
    static const std::string kTo = "To";
    static const std::string kNext = "Next";

    int lineNo = lineNumberAt(src, pos);

    auto expectSemicolon = [&]() {
        skipWhitespaceAndComments(src, pos);
        if (pos >= src.size() || src[pos] != ';') {
            throw CompileError("line " + std::to_string(lineNumberAt(src, pos)) +
                                ": expected ';' after statement");
        }
        ++pos;
        skipWhitespaceAndComments(src, pos);
    };

    if (matchesKeywordAt(src, pos, kPrint)) {
        pos += kPrint.size();
        skipWhitespaceAndComments(src, pos);
        std::string text = parseStringLiteral(src, pos);
        // No automatic CR/LF - see Axis 21: `print` emits exactly the
        // literal's decoded bytes, nothing more, so a programmer issuing
        // several `print`s in a row (e.g. inside a loop body) gets them
        // concatenated on one line unless a literal itself ends in `\n`.
        for (unsigned char c : text) {
            out.push_back(c);
        }
        expectSemicolon();
    } else if (matchesKeywordAt(src, pos, kUint16)) {
        pos += kUint16.size();
        skipWhitespaceAndComments(src, pos);
        std::string name = parseIdentifier(src, pos);
        int bufferId = compiler.declareVariable(name, lineNo);
        skipWhitespaceAndComments(src, pos);
        if (pos < src.size() && src[pos] == '=') {
            ++pos;
            skipWhitespaceAndComments(src, pos);
            std::vector<Term> terms = parseExpr(src, pos, compiler);
            if (terms.size() == 1 && terms[0].isLiteral) {
                // Common case: write the literal value directly as
                // the buffer's initial content, no adjust needed.
                emitWriteBlock(out, bufferId, le16Bytes(terms[0].literalValue));
            } else {
                emitWriteBlock(out, bufferId, {0, 0});
                compileExprInto(compiler, out, bufferId, terms);
            }
        } else {
            // Naked declaration - defaults to 0, see Axis 6.
            emitWriteBlock(out, bufferId, {0, 0});
        }
        expectSemicolon();
    } else if (matchesKeywordAt(src, pos, kFor)) {
        pos += kFor.size();
        skipWhitespaceAndComments(src, pos);
        std::string loopVarName = parseIdentifier(src, pos);
        // The loop variable must already be declared - `For` never
        // doubles as a declaration form (Author's stated preference: keep
        // declaration and loop-iteration concerns separate).
        int iBufferId = compiler.lookupVariable(loopVarName, lineNo);
        skipWhitespaceAndComments(src, pos);
        if (pos >= src.size() || src[pos] != '=') {
            throw CompileError("line " + std::to_string(lineNumberAt(src, pos)) +
                                ": expected '=' after 'For " + loopVarName + "'");
        }
        ++pos;
        skipWhitespaceAndComments(src, pos);
        std::vector<Term> startTerms = parseExpr(src, pos, compiler);
        skipWhitespaceAndComments(src, pos);
        if (!matchesKeywordAt(src, pos, kTo)) {
            throw CompileError("line " + std::to_string(lineNumberAt(src, pos)) +
                                ": expected 'To' in 'For' loop");
        }
        pos += kTo.size();
        skipWhitespaceAndComments(src, pos);
        std::vector<Term> boundTerms = parseExpr(src, pos, compiler);
        expectSemicolon(); // closes the `For ... To ...` header line

        // Initialise the loop variable, in the surrounding buffer - same
        // codegen as a plain `i = expr;` assignment.
        compileExprInto(compiler, out, iBufferId, startTerms);

        // Materialise the (possibly-computed) upper bound into its own
        // dedicated buffer - never compare the loop variable directly
        // against a literal (see emitCondCallBufferValue's comment for
        // why that's unreliable).
        int boundBufferId = compiler.allocInternalBuffer();
        if (boundTerms.size() == 1 && boundTerms[0].isLiteral) {
            emitWriteBlock(out, boundBufferId, le16Bytes(boundTerms[0].literalValue));
        } else {
            emitWriteBlock(out, boundBufferId, {0, 0});
            compileExprInto(compiler, out, boundBufferId, boundTerms);
        }

        // The loop body gets its own dedicated buffer so it can
        // conditionally call itself as the very last thing it does - see
        // docs/devlog/2026-07-29-loop-tail-call-prototype.md: the VDP
        // automatically converts a trailing conditional call into a jump,
        // so this loop costs only one real call frame no matter how many
        // iterations it runs.
        int bodyBufferId = compiler.allocInternalBuffer();
        std::vector<unsigned char> bodyOut;
        compileBlock(compiler, src, pos, bodyOut);

        skipWhitespaceAndComments(src, pos);
        if (!matchesKeywordAt(src, pos, kNext)) {
            throw CompileError("line " + std::to_string(lineNumberAt(src, pos)) +
                                ": expected 'Next' to close 'For " + loopVarName + "'");
        }
        pos += kNext.size();
        skipWhitespaceAndComments(src, pos);
        std::string closingName = parseIdentifier(src, pos);
        if (closingName != loopVarName) {
            throw CompileError("line " + std::to_string(lineNumberAt(src, pos)) +
                                ": 'Next " + closingName + "' does not match 'For " +
                                loopVarName + "'");
        }
        expectSemicolon(); // closes `Next i`

        // Increment the loop variable - reuses the exact same addition
        // codegen as a plain `i = i + 1;` assignment (Axis 13's ACC
        // scratch buffer keeps the carry-out byte from spilling into
        // whatever buffer happens to follow `i`'s).
        compileExprInto(compiler, bodyOut, iBufferId, {Term{false, 0, iBufferId}, Term{true, 1, 0}});

        // Trailing conditional self-call: continue while i <= bound. This
        // MUST be the last thing written to bodyOut for the VDP's
        // tail-call optimisation to trigger.
        emitCondCallBufferValue(bodyOut, bodyBufferId, kCondLE, iBufferId, 0, boundBufferId, 0);

        // Materialise the body buffer as a literal data blob in the
        // surrounding buffer, then conditionally call it once to enter
        // the loop (checked, not unconditional, so `For i = 10 To 5`
        // correctly runs zero iterations) - this first call is a real,
        // non-tail call, but it only happens once regardless of
        // iteration count; the per-iteration tail-call optimisation lives
        // inside bodyOut itself, above.
        emitWriteBlock(out, bodyBufferId, bodyOut);
        emitCondCallBufferValue(out, bodyBufferId, kCondLE, iBufferId, 0, boundBufferId, 0);
    } else if (pos < src.size() && isIdentStart(src[pos])) {
        std::string name = parseIdentifier(src, pos);
        int bufferId = compiler.lookupVariable(name, lineNo);
        skipWhitespaceAndComments(src, pos);
        if (pos >= src.size() || src[pos] != '=') {
            throw CompileError("line " + std::to_string(lineNumberAt(src, pos)) +
                                ": expected '=' after '" + name + "'");
        }
        ++pos;
        skipWhitespaceAndComments(src, pos);
        std::vector<Term> terms = parseExpr(src, pos, compiler);
        compileExprInto(compiler, out, bufferId, terms);
        expectSemicolon();
    } else {
        throw CompileError("line " + std::to_string(lineNo) +
                            ": expected a statement ('print', 'uint16', 'For', "
                            "or an assignment)");
    }
}

// Compiles zero or more statements into `out`, stopping (without consuming)
// as soon as the upcoming token is the `Next` keyword, or at end-of-input.
// Used both at the top level (which should never legitimately see a bare
// `Next`) and to compile a `For` loop's body.
void compileBlock(Compiler &compiler, const std::string &src, size_t &pos,
                   std::vector<unsigned char> &out) {
    static const std::string kNext = "Next";
    for (;;) {
        skipWhitespaceAndComments(src, pos);
        if (pos >= src.size() || matchesKeywordAt(src, pos, kNext)) {
            break;
        }
        compileStatement(compiler, src, pos, out);
    }
}

std::vector<unsigned char> compile(const std::string &src) {
    Compiler compiler;
    size_t pos = 0;
    compileBlock(compiler, src, pos, compiler.out);
    skipWhitespaceAndComments(src, pos);
    if (pos < src.size()) {
        throw CompileError("line " + std::to_string(lineNumberAt(src, pos)) +
                            ": unexpected 'Next' with no matching 'For'");
    }
    return compiler.out;
}

std::string readFile(const std::string &path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw CompileError("could not open input file: " + path);
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

void writeFile(const std::string &path, const std::vector<unsigned char> &data) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw CompileError("could not open output file for writing: " + path);
    }
    out.write(reinterpret_cast<const char *>(data.data()),
              static_cast<std::streamsize>(data.size()));
}

} // namespace

int main(int argc, char **argv) {
    if (argc != 3) {
        // Extensions shown here are just illustrative - golemc does not
        // inspect or require any particular file extension on either
        // argument; see this file's header comment.
        std::cerr << "usage: golemc <input> <output>\n";
        return 2;
    }
    try {
        std::string src = readFile(argv[1]);
        std::vector<unsigned char> compiled = compile(src);
        writeFile(argv[2], compiled);
        std::cout << "wrote " << compiled.size() << " bytes to " << argv[2] << "\n";
    } catch (const CompileError &e) {
        std::cerr << "golemc: error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

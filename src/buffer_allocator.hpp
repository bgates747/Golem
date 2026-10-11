#pragma once
#include <map>
#include <stdexcept>
#include <string>

namespace golem {
struct AllocationError : std::runtime_error { using std::runtime_error::runtime_error; };
struct BufferClaim { std::string owner; bool external; };

// Compilation-local ID ownership. No deallocation or runtime heap. External
// reservations must precede compiler claims; loader root 0 and special 65535
// are never allocatable. Bounds permit small deterministic exhaustion checks.
class BufferAllocator {
    std::map<unsigned, BufferClaim> claims_{{0, {"loader.root", true}}};
    unsigned next_, last_;
    bool started_ = false;
    static void valid(unsigned id) {
        if (id == 0 || id >= 65535) throw AllocationError("buffer ID must be in 1..65534 (0 and 65535 are reserved)");
    }
    void available(unsigned id) const {
        valid(id);
        auto found = claims_.find(id);
        if (found != claims_.end())
            throw AllocationError("buffer ID " + std::to_string(id) + " already claimed by " + found->second.owner);
    }
public:
    explicit BufferAllocator(unsigned first = 1, unsigned last = 65534) : next_(first), last_(last) {
        valid(first); valid(last);
        if (first > last) throw AllocationError("reversed allocation bounds");
    }
    void reserve(unsigned first, unsigned last, const std::string &owner) {
        if (started_) throw AllocationError("external reservations must precede compiler allocation");
        valid(first); valid(last);
        if (first > last) throw AllocationError("reversed reservation range");
        // Check the entire range before changing ownership on a collision.
        for (unsigned id = first; id <= last; ++id) available(id);
        for (unsigned id = first; id <= last; ++id) claims_.emplace(id, BufferClaim{owner, true});
    }
    unsigned claim(unsigned id, const std::string &owner) {
        available(id);
        claims_.emplace(id, BufferClaim{owner, false});
        started_ = true;
        return id;
    }
    unsigned allocate(const std::string &owner) {
        while (next_ <= last_ && claims_.count(next_)) ++next_;
        if (next_ > last_) throw AllocationError("buffer ID namespace exhausted");
        return claim(next_++, owner);
    }
    const std::map<unsigned, BufferClaim> &claims() const { return claims_; }
};
} // namespace golem

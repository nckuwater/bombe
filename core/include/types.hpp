#pragma once
#include <array>
#include <optional>

namespace bombe {

inline constexpr int kAlphabetSize = 26;

// Complete plugboard: plug[i] = j means letter i <-> j.
// plug[i] = i means the letter is not swapped (identity).
using PlugBoard = std::array<int, kAlphabetSize>;

// Partial plugboard used during Bombe constraint propagation.
// nullopt = connection for that letter not yet determined.
using PartialPlug = std::array<std::optional<int>, kAlphabetSize>;

constexpr int  char_to_idx(char c) noexcept { return c - 'A'; }
constexpr char idx_to_char(int  i) noexcept { return static_cast<char>('A' + i); }

inline PlugBoard identity_plug() noexcept {
    PlugBoard pb;
    for (int i = 0; i < kAlphabetSize; ++i) pb[i] = i;
    return pb;
}

inline PartialPlug empty_partial_plug() noexcept {
    PartialPlug pp;
    pp.fill(std::nullopt);
    return pp;
}

} // namespace bombe

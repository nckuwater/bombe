#pragma once
#include "types.hpp"
#include <string>
#include <string_view>
#include <vector>

namespace bombe {

// ---------------------------------------------------------------------------
// MenuEdge — one entry in the Bombe menu.
//
// Derived from one character position in the crib:
//   plain[crib_offset] == letter_a,  cipher[crib_offset] == letter_b
//
// This means: enigma_core(plug[letter_a], crib_offset) == plug[letter_b]
// The relationship is bidirectional because enigma_core is an involution.
// ---------------------------------------------------------------------------
struct MenuEdge {
    int crib_offset;  // position in the crib (0-based)
    int letter_a;     // plain letter index (0-25)
    int letter_b;     // cipher letter index (0-25)
};

// ---------------------------------------------------------------------------
// Menu — the graph of letter connections derived from a set of crib pairs.
//
// Nodes are letters (0-25). Each crib character produces one undirected edge.
// The Menu also finds all cycles and selects the best test letter
// (the letter with the highest degree, giving the most constraints for the
// Bombe's constraint-propagation loop).
// ---------------------------------------------------------------------------
class Menu {
public:
    // Add a known plain/cipher pair. Both strings must be the same length
    // and contain only uppercase A-Z.
    void add_crib(std::string_view plain, std::string_view cipher);

    // Build the menu graph (edges + cycles + test-letter selection).
    // Call once after all cribs have been added.
    void build();

    // ---- Accessors ----
    const std::vector<MenuEdge>&              edges()        const noexcept { return edges_; }
    const std::vector<std::vector<MenuEdge>>& cycles()       const noexcept { return cycles_; }
    int                                        test_letter()  const noexcept { return test_letter_; }

    // Crib texts (needed for final verification)
    const std::vector<std::string>& plain_texts()  const noexcept { return plains_; }
    const std::vector<std::string>& cipher_texts() const noexcept { return ciphers_; }

    // Human-readable summary for terminal output
    std::string summary() const;

private:
    std::vector<std::string> plains_;
    std::vector<std::string> ciphers_;
    std::vector<MenuEdge>    edges_;

    std::vector<std::vector<MenuEdge>> cycles_;
    int test_letter_ = 0;

    // Adjacency list: adjacency_[letter] = list of (edge_index, other_letter)
    std::vector<std::pair<int,int>> adjacency_[kAlphabetSize];

    void find_cycles();
    void select_test_letter();
};

} // namespace bombe

#pragma once
#include "enigma.hpp"
#include "menu.hpp"
#include "types.hpp"
#include <functional>
#include <optional>
#include <vector>

namespace bombe {

// ---------------------------------------------------------------------------
// BombeStop — one candidate result found by the Bombe.
//
// A "stop" is a combination of rotor settings for which the constraint
// propagation did not produce a contradiction. Stops must be verified
// against the full crib text to filter false positives.
// ---------------------------------------------------------------------------
struct BombeStop {
    std::vector<int> rotor_indices;    // 0-based, left → right
    std::vector<int> start_positions;  // Grundstellung, 0-25, left → right
    std::vector<int> ring_settings;    // Ringstellung, 0-25, left → right
    PlugBoard        plugboard;        // derived plugboard (may be partial for false stops)
};

// ---------------------------------------------------------------------------
// BombeSettings — runtime options loaded from settings.toml.
// ---------------------------------------------------------------------------
struct BombeSettings {
    int  max_scramblers       = 0;      // 0 = unlimited
    bool search_ring_settings = false;  // if true, search r0 and r1 (r2 fixed at 0)
    std::vector<int> fixed_ring_settings = {0, 0, 0};  // used when not searching
    std::vector<int> fixed_rotor_indices;               // empty = try all permutations
    int reflector_index = 0;
};

// ---------------------------------------------------------------------------
// Bombe — the constraint-propagation cracker.
//
// Physical model:
//   • Each MenuEdge corresponds to one scrambler drum.
//   • For a given rotor configuration and start position, each of the 26
//     possible plug-connections for the test letter is tried as an assumption.
//   • The assumption is propagated through the scrambler circuit using the
//     Diagonal Board's symmetry constraint (plug[a]=b ↔ plug[b]=a).
//   • A contradiction anywhere in the propagation eliminates the assumption.
//   • A stop occurs when propagation completes without contradiction.
//
// Search space:
//   Rotor permutations × 26³ start positions × (26² ring-setting pairs if
//   search_ring_settings is enabled, otherwise fixed) × 26 assumptions.
//   The rightmost ring setting is always fixed at 0 (it is equivalent to a
//   start-position shift and adds no new solutions).
// ---------------------------------------------------------------------------
class Bombe {
public:
    // Called after each position is tested.
    using ProgressCallback = std::function<void(
        const std::vector<int>& rotors,
        const std::vector<int>& current_positions,
        int tested, int total)>;

    // Called immediately when a verified stop is found (before crack() returns).
    using StopCallback = std::function<void(const BombeStop&)>;

    Bombe(Menu menu, BombeSettings settings)
        : menu_(std::move(menu)), settings_(std::move(settings)) {}

    // Run the full search. Returns all verified stops.
    std::vector<BombeStop> crack(
        const ProgressCallback& on_progress = {},
        const StopCallback&     on_stop     = {}) const;

private:
    Menu         menu_;
    BombeSettings settings_;

    // Test one specific Enigma configuration (rotors + start + rings).
    // Returns verified stops found at this configuration.
    std::vector<BombeStop> test_config(
        const EnigmaConfig& cfg) const;

    // Core constraint propagation (the Bombe circuit + Diagonal Board).
    // Returns the derived partial plugboard if consistent, nullopt if contradiction.
    std::optional<PartialPlug> propagate(
        int test_letter,
        int assumed_partner,
        const Enigma& enigma,
        const std::vector<MenuEdge>& edges) const;

    // Resolve a PartialPlug into a full PlugBoard (unset letters → self-connected)
    // and verify it produces the correct crib output.
    std::optional<PlugBoard> verify_and_complete(
        const PartialPlug& partial,
        const EnigmaConfig& cfg) const;
};

} // namespace bombe

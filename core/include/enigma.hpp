#pragma once
#include "types.hpp"
#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace bombe {

// ---------------------------------------------------------------------------
// Rotor / Reflector — pure value types loaded from config.
// ---------------------------------------------------------------------------

struct Rotor {
    std::string name;                                // historical designation e.g. "I", "IV"
    std::array<int, kAlphabetSize> forward_wiring;   // index i -> output letter
    std::array<int, kAlphabetSize> inverse_wiring;   // derived inverse
    int turnover_notch;  // window position (0-25) that causes the left neighbour to step
};

struct Reflector {
    std::string name;
    std::array<int, kAlphabetSize> wiring;  // involution: wiring[wiring[i]] == i
};

// ---------------------------------------------------------------------------
// EnigmaConfig — all parameters that define an Enigma setting.
//
// rotor_indices / ring_settings / start_positions are ordered left → right
// (index 0 = slowest rotor, last = rightmost / fastest).
// ---------------------------------------------------------------------------
struct EnigmaConfig {
    std::vector<int> rotor_indices;    // 0-based index into Enigma::catalog
    std::vector<int> ring_settings;    // Ringstellung, one per rotor (0-25)
    std::vector<int> start_positions;  // Grundstellung, one per rotor (0-25)
    PlugBoard        plugboard  = identity_plug();
    int              reflector_index = 0;
};

// ---------------------------------------------------------------------------
// Enigma machine.
//
// Two distinct operations:
//   process()    — full stateful encipher/decipher with plugboard.
//   route_at()   — stateless, plugboard-free routing used by the Bombe.
//
// The global rotor/reflector catalog is loaded once via load_catalog().
// ---------------------------------------------------------------------------
class Enigma {
public:
    // ---- Catalog (shared across all instances) ----
    struct Catalog {
        std::vector<Rotor>     rotors;
        std::vector<Reflector> reflectors;
    };

    static bool          load_catalog(std::string_view path);
    static const Catalog& catalog() noexcept { return catalog_; }

    // ---- Construction ----
    explicit Enigma(EnigmaConfig config) : config_(std::move(config)) {}

    // ---- Full encipher / decipher (with plugboard, stateful stepping) ----
    std::string process(std::string_view text) const;

    // ---- Bombe: stateless routing without plugboard ----
    // Returns the output letter index when letter 'c' passes through
    // rotors + reflector, with rotors having stepped (crib_offset + 1)
    // times from start_positions.  Plugboard is intentionally excluded.
    int route_at(int c, int crib_offset) const;

    // ---- Accessors ----
    const EnigmaConfig& config() const noexcept { return config_; }

private:
    static Catalog catalog_;

    EnigmaConfig config_;

    // Advance a position vector by exactly one step, respecting the
    // double-stepping anomaly of the Enigma pawl mechanism.
    void advance_one(std::vector<int>& positions) const;

    // Advance positions by n steps from their current state.
    std::vector<int> advance_n(std::vector<int> positions, int n) const;

    // Route letter c through rotors + reflector at the given explicit positions.
    // Does NOT apply the plugboard.
    int route_through(int c, const std::vector<int>& positions) const;
};

} // namespace bombe

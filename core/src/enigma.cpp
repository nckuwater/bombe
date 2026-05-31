#include "enigma.hpp"

#define TOML_HEADER_ONLY 1
#include <toml.hpp>

namespace bombe {

Enigma::Catalog Enigma::catalog_;

bool Enigma::load_catalog(std::string_view path) {
    auto cfg = toml::parse_file(path);

    catalog_.rotors.clear();
    catalog_.reflectors.clear();

    if (auto* arr = cfg["rotors"].as_array()) {
        for (const auto& node : *arr) {
            const auto& tbl = *node.as_table();
            Rotor r;
            r.name = tbl["name"].value<std::string>().value();
            r.thin = tbl["thin"].value<bool>().value_or(false);
            auto wiring = tbl["wiring"].value<std::string>().value();
            for (int i = 0; i < kAlphabetSize; ++i) {
                r.forward_wiring[i]                      = char_to_idx(wiring[i]);
                r.inverse_wiring[char_to_idx(wiring[i])] = i;
            }
            if (auto* arr = tbl["notch"].as_array()) {
                for (const auto& v : *arr)
                    if (auto s = v.value<std::string>())
                        r.notches.push_back(char_to_idx((*s)[0]));
            }
            catalog_.rotors.push_back(std::move(r));
        }
    }

    if (auto* arr = cfg["reflectors"].as_array()) {
        for (const auto& node : *arr) {
            const auto& tbl = *node.as_table();
            Reflector rf;
            rf.name = tbl["name"].value<std::string>().value();
            auto wiring = tbl["wiring"].value<std::string>().value();
            for (int i = 0; i < kAlphabetSize; ++i)
                rf.wiring[i] = char_to_idx(wiring[i]);
            catalog_.reflectors.push_back(std::move(rf));
        }
    }

    return !catalog_.rotors.empty() && !catalog_.reflectors.empty();
}

// ---------------------------------------------------------------------------
// advance_one — Enigma double-stepping rule:
//
//   Rotor ordering: positions[0] = leftmost (slow), positions[n-1] = rightmost.
//
//   1. If the middle rotor is at its notch → left steps AND middle steps.
//      (This is the double-step anomaly: middle steps from both triggers.)
//   2. If the right rotor is at its notch → middle steps.
//   3. Right rotor always steps.
//
//   The turnover notch is on the outer alphabet ring and is therefore
//   independent of ring setting (ring setting only shifts the internal wiring).
// ---------------------------------------------------------------------------
// Returns true if a rotor's current position is at any of its notches.
static bool at_notch(const bombe::Rotor& r, int pos) {
    for (int n : r.notches) if (pos == n) return true;
    return false;
}

void Enigma::advance_one(std::vector<int>& pos) const {
    const int n = static_cast<int>(pos.size());
    const auto& rotors = catalog_.rotors;
    const auto& idx    = config_.rotor_indices;

    // Thin rotors (M4 Beta/Gamma) never step.
    // The effective stepping rotors are the rightmost non-thin ones.
    // For a standard 3-rotor or M4 4-rotor setup we find the first
    // three non-thin rotors from the right.
    std::vector<int> step_slots;   // indices into pos[] that can step
    for (int i = n - 1; i >= 0 && static_cast<int>(step_slots.size()) < 3; --i) {
        if (!rotors[idx[i]].thin) step_slots.push_back(i);
    }
    // step_slots[0] = rightmost stepping slot, [1] = middle, [2] = left

    if (step_slots.size() >= 2) {
        bool mid_at_notch   = step_slots.size() >= 2 &&
                              at_notch(rotors[idx[step_slots[1]]], pos[step_slots[1]]);
        bool right_at_notch = at_notch(rotors[idx[step_slots[0]]], pos[step_slots[0]]);

        if (mid_at_notch && step_slots.size() >= 3) {
            pos[step_slots[2]] = (pos[step_slots[2]] + 1) % kAlphabetSize;  // left steps
            pos[step_slots[1]] = (pos[step_slots[1]] + 1) % kAlphabetSize;  // mid double-steps
        }
        if (right_at_notch) {
            pos[step_slots[1]] = (pos[step_slots[1]] + 1) % kAlphabetSize;  // mid steps
        }
    }
    if (!step_slots.empty())
        pos[step_slots[0]] = (pos[step_slots[0]] + 1) % kAlphabetSize;  // rightmost always steps
}

std::vector<int> Enigma::advance_n(std::vector<int> pos, int n) const {
    for (int i = 0; i < n; ++i) advance_one(pos);
    return pos;
}

// ---------------------------------------------------------------------------
// route_through — signal path through rotors + reflector (no plugboard).
//
//   For rotor at position p with ring setting r:
//     effective_offset = (p - r + kAlphabetSize) % kAlphabetSize
//     forward:  out = (forward_wiring[(c + offset) % N] - offset + N) % N
//     backward: out = (inverse_wiring[(c + offset) % N] - offset + N) % N
// ---------------------------------------------------------------------------
int Enigma::route_through(int c, const std::vector<int>& pos) const {
    const int n = static_cast<int>(pos.size());
    const auto& rotors = catalog_.rotors;
    const auto& rings  = config_.ring_settings;

    // Right → left
    for (int i = n - 1; i >= 0; --i) {
        const int offset = (pos[i] - rings[i] + kAlphabetSize) % kAlphabetSize;
        c = (rotors[config_.rotor_indices[i]].forward_wiring[(c + offset) % kAlphabetSize]
             - offset + kAlphabetSize) % kAlphabetSize;
    }

    // Reflector
    c = catalog_.reflectors[config_.reflector_index].wiring[c];

    // Left → right
    for (int i = 0; i < n; ++i) {
        const int offset = (pos[i] - rings[i] + kAlphabetSize) % kAlphabetSize;
        c = (rotors[config_.rotor_indices[i]].inverse_wiring[(c + offset) % kAlphabetSize]
             - offset + kAlphabetSize) % kAlphabetSize;
    }

    return c;
}

std::string Enigma::process(std::string_view text) const {
    std::vector<int> pos = config_.start_positions;
    const auto& pb = config_.plugboard;
    std::string out;
    out.reserve(text.size());
    for (char ch : text) {
        advance_one(pos);
        int c = pb[char_to_idx(ch)];
        c = route_through(c, pos);
        c = pb[c];
        out += idx_to_char(c);
    }
    return out;
}

int Enigma::route_at(int c, int crib_offset) const {
    auto pos = advance_n(config_.start_positions, crib_offset + 1);
    return route_through(c, pos);
}

} // namespace bombe

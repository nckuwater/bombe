#include "bombe.hpp"
#include <algorithm>
#include <queue>

namespace bombe {

// ---------------------------------------------------------------------------
// propagate — the Bombe circuit + Diagonal Board, modelled as BFS constraint
// propagation.
//
// Starting from the assumption plug[test_letter] = assumed_partner:
//   1. Assign plug[a] = b.
//   2. Diagonal Board (plugboard symmetry): also assign plug[b] = a.
//   3. For every MenuEdge incident to letter a:
//        other_letter connects via enigma_core at crib_offset.
//        Derived constraint: plug[other] = enigma.route_at(b, offset).
//   4. Repeat until queue empty or contradiction found.
//
// This faithfully models the physical Bombe's electrical circuit: each
// MenuEdge is a scrambler drum, and the Diagonal Board wires provide the
// symmetric plugboard constraint in hardware.
// ---------------------------------------------------------------------------
std::optional<PartialPlug> Bombe::propagate(
        int test_letter,
        int assumed_partner,
        const Enigma& enigma,
        const std::vector<MenuEdge>& edges) const
{
    auto plug = empty_partial_plug();

    // Adjacency: letter -> [(crib_offset, other_letter)]
    std::vector<std::pair<int,int>> adj[kAlphabetSize];
    for (const auto& e : edges) {
        adj[e.letter_a].emplace_back(e.crib_offset, e.letter_b);
        adj[e.letter_b].emplace_back(e.crib_offset, e.letter_a);
    }

    // BFS queue of (letter, assumed_plug_partner)
    std::queue<std::pair<int,int>> q;
    q.emplace(test_letter, assumed_partner);

    while (!q.empty()) {
        auto [letter, partner] = q.front();
        q.pop();

        if (plug[letter].has_value()) {
            if (*plug[letter] != partner) return std::nullopt;  // contradiction
            continue;  // already propagated from here
        }

        plug[letter] = partner;

        // Diagonal Board: plug is symmetric
        if (!plug[partner].has_value()) {
            q.emplace(partner, letter);
        } else if (*plug[partner] != letter) {
            return std::nullopt;
        }

        // Propagate through every scrambler connected to this letter
        for (auto [crib_offset, other] : adj[letter]) {
            int derived = enigma.route_at(partner, crib_offset);
            if (!plug[other].has_value()) {
                q.emplace(other, derived);
            } else if (*plug[other] != derived) {
                return std::nullopt;
            }
        }
    }

    return plug;
}

// ---------------------------------------------------------------------------
// verify_and_complete — convert a PartialPlug to a full PlugBoard and verify
// it against every crib plain/cipher pair.
// ---------------------------------------------------------------------------
std::optional<PlugBoard> Bombe::verify_and_complete(
        const PartialPlug& partial, const EnigmaConfig& base_cfg) const
{
    // Fill unresolved letters with identity (letter maps to itself)
    PlugBoard pb = identity_plug();
    for (int i = 0; i < kAlphabetSize; ++i)
        if (partial[i].has_value()) pb[i] = *partial[i];

    EnigmaConfig cfg = base_cfg;
    cfg.plugboard = pb;
    Enigma machine(cfg);

    const auto& plains  = menu_.plain_texts();
    const auto& ciphers = menu_.cipher_texts();
    for (int i = 0; i < static_cast<int>(plains.size()); ++i) {
        if (machine.process(plains[i]) != ciphers[i])
            return std::nullopt;
    }

    return pb;
}

// ---------------------------------------------------------------------------
// test_config — try all 26 assumptions for the test letter at this config.
// ---------------------------------------------------------------------------
std::vector<BombeStop> Bombe::test_config(const EnigmaConfig& cfg) const {
    Enigma enigma(cfg);

    // Respect max_scramblers setting
    const auto& all_edges = menu_.edges();
    std::vector<MenuEdge> active_edges =
        (settings_.max_scramblers > 0 && static_cast<int>(all_edges.size()) > settings_.max_scramblers)
        ? std::vector<MenuEdge>(all_edges.begin(), all_edges.begin() + settings_.max_scramblers)
        : all_edges;

    const int test_letter = menu_.test_letter();
    std::vector<BombeStop> stops;

    for (int assumption = 0; assumption < kAlphabetSize; ++assumption) {
        auto partial = propagate(test_letter, assumption, enigma, active_edges);
        if (!partial) continue;

        auto full_pb = verify_and_complete(*partial, cfg);
        if (!full_pb) continue;

        stops.push_back({
            cfg.rotor_indices,
            cfg.start_positions,
            cfg.ring_settings,
            *full_pb
        });
    }

    return stops;
}

// ---------------------------------------------------------------------------
// crack — outer search over all rotor permutations × positions × ring settings.
// ---------------------------------------------------------------------------
std::vector<BombeStop> Bombe::crack(const ProgressCallback& on_progress,
                                     const StopCallback&     on_stop) const {
    std::vector<BombeStop> results;

    const int num_rotors = static_cast<int>(Enigma::catalog().rotors.size());

    // Build rotor combinations to try
    std::vector<std::vector<int>> rotor_combos;
    if (!settings_.fixed_rotor_indices.empty()) {
        rotor_combos.push_back(settings_.fixed_rotor_indices);
    } else {
        for (int r0 = 0; r0 < num_rotors; ++r0)
            for (int r1 = 0; r1 < num_rotors; ++r1) {
                if (r1 == r0) continue;
                for (int r2 = 0; r2 < num_rotors; ++r2) {
                    if (r2 == r0 || r2 == r1) continue;
                    rotor_combos.push_back({r0, r1, r2});
                }
            }
    }

    // Ring-setting search ranges.
    // In search mode: right rotor ring is fixed at 0 because it is equivalent
    // to a start-position shift — the exhaustive position search already covers
    // all equivalences, so searching it only produces duplicates.
    // In fixed mode: all three values from fixed_ring_settings are used exactly
    // as specified (the optimization does NOT apply to known ring settings).
    const int rs_range = settings_.search_ring_settings ? kAlphabetSize : 1;
    const auto& fixed_rings = settings_.fixed_ring_settings;

    constexpr int kTotalPositions = kAlphabetSize * kAlphabetSize * kAlphabetSize;

    for (const auto& rotors : rotor_combos) {
        if (on_progress) on_progress(rotors, {0,0,0}, 0, kTotalPositions);

        for (int rs0 = 0; rs0 < rs_range; ++rs0) {
            for (int rs1 = 0; rs1 < rs_range; ++rs1) {
                std::vector<int> rings;
                if (settings_.search_ring_settings) {
                    rings = {rs0, rs1, 0};
                } else {
                    rings = fixed_rings;
                }

                int tested = 0;
                for (int p0 = 0; p0 < kAlphabetSize; ++p0) {
                    for (int p1 = 0; p1 < kAlphabetSize; ++p1) {
                        for (int p2 = 0; p2 < kAlphabetSize; ++p2) {
                            EnigmaConfig cfg;
                            cfg.rotor_indices   = rotors;
                            cfg.ring_settings   = rings;
                            cfg.start_positions = {p0, p1, p2};
                            cfg.reflector_index = settings_.reflector_index;
                            cfg.plugboard       = identity_plug();

                            auto stops = test_config(cfg);
                            for (auto& s : stops) {
                                if (on_stop) on_stop(s);
                                results.push_back(std::move(s));
                            }

                            ++tested;
                            if (on_progress)
                                on_progress(rotors, {p0, p1, p2}, tested, kTotalPositions);
                        }
                    }
                }
            }
        }
    }

    return results;
}

} // namespace bombe

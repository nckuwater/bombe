#include "enigma.hpp"
#include "menu.hpp"
#include "bombe.hpp"
#include <gtest/gtest.h>

using namespace bombe;

class BombeTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        ASSERT_TRUE(Enigma::load_catalog("config/machine.toml"));
    }

    // Build a Menu from crib pairs encrypted with the given config.
    static Menu make_menu(const EnigmaConfig& cfg,
                          const std::vector<std::string>& plains) {
        Menu m;
        for (const auto& plain : plains) {
            Enigma enc(cfg);
            m.add_crib(plain, enc.process(plain));
        }
        m.build();
        return m;
    }

    static BombeSettings default_settings() {
        BombeSettings s;
        s.max_scramblers       = 0;
        s.search_ring_settings = false;
        s.fixed_ring_settings  = {0, 0, 0};
        s.reflector_index      = 0;
        return s;
    }
};

// ---------------------------------------------------------------------------
// Constraint propagation: a single assumption that is consistent with all
// edges should not return nullopt.
// ---------------------------------------------------------------------------
TEST_F(BombeTest, PropagationNoFalseContradiction) {
    EnigmaConfig cfg;
    cfg.rotor_indices   = {0, 1, 2};
    cfg.ring_settings   = {0, 0, 0};
    cfg.start_positions = {2, 0, 19};  // C, A, T
    cfg.reflector_index = 0;
    cfg.plugboard       = identity_plug();

    auto menu = make_menu(cfg, {"HELLOWORLD"});

    BombeSettings s = default_settings();
    s.fixed_rotor_indices = {0, 1, 2};

    Bombe bombe(std::move(menu), s);
    auto results = bombe.crack();

    // Must find at least one result matching the known settings
    bool found = false;
    for (const auto& r : results) {
        if (r.rotor_indices   == cfg.rotor_indices &&
            r.start_positions == cfg.start_positions) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Bombe did not recover the known rotor settings";
}

// ---------------------------------------------------------------------------
// Full crack: Bombe must recover the exact settings used to generate cribs.
// Uses the same parameters as generate_crib.cpp.
// ---------------------------------------------------------------------------
TEST_F(BombeTest, RecoverKnownSettings) {
    EnigmaConfig known;
    known.rotor_indices   = {0, 1, 2};
    known.ring_settings   = {0, 0, 0};
    known.start_positions = {2, 0, 19};  // C, A, T
    known.reflector_index = 0;
    known.plugboard       = identity_plug();
    known.plugboard[char_to_idx('A')] = char_to_idx('Z');
    known.plugboard[char_to_idx('Z')] = char_to_idx('A');
    known.plugboard[char_to_idx('B')] = char_to_idx('Y');
    known.plugboard[char_to_idx('Y')] = char_to_idx('B');
    known.plugboard[char_to_idx('C')] = char_to_idx('X');
    known.plugboard[char_to_idx('X')] = char_to_idx('C');

    const std::vector<std::string> plains = {
        "TURING",
        "ENIGMAISAMACHINE",
        "BOMBECRACKSIT",
        "WEDIDIT",
        "ADDITIONALMSG",
    };

    auto menu = make_menu(known, plains);

    BombeSettings s = default_settings();
    s.fixed_rotor_indices = {0, 1, 2};  // restrict search to known rotors

    Bombe bombe(std::move(menu), s);
    auto results = bombe.crack();

    ASSERT_FALSE(results.empty()) << "No solution found";

    // Check that the first (and presumably only) result matches
    const auto& r = results[0];
    EXPECT_EQ(r.rotor_indices,   known.rotor_indices);
    EXPECT_EQ(r.start_positions, known.start_positions);
    EXPECT_EQ(r.ring_settings,   known.ring_settings);
    EXPECT_EQ(r.plugboard,       known.plugboard);
}

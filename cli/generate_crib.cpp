// generate_crib — randomly generate Enigma settings, encrypt plaintexts built
// from a WWII-era word list, and write a self-contained crib TOML file.
//
// Usage:
//   ./generate_crib [options]
//
// Options:
//   --seed N          fix the random seed for reproducibility
//   --count N         generate N crib files (default: 1)
//   --out <path>      output path for a single file (default: bombe_pc.toml)
//   --out-dir <dir>   output directory for batch mode (files named crib_<seed>.toml)
//   --machine <path>  machine config (default: config/machine.toml)

#include "enigma.hpp"

#define TOML_HEADER_ONLY 1
#include <toml.hpp>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace bombe;

// ---------------------------------------------------------------------------
// WWII-era military/operational vocabulary (all uppercase, no spaces).
// Phrases are realistic intercept vocabulary from the period.
// ---------------------------------------------------------------------------
static const std::vector<std::string> kWordList = {
    "ATTACK", "ADVANCE", "RETREAT", "HALT", "ENGAGE",
    "WEATHER", "REPORT", "STATION", "SECTOR", "POSITION",
    "AIRCRAFT", "CONVOY", "HARBOUR", "AIRFIELD", "COMMAND",
    "DIVISION", "REGIMENT", "BATTALION", "SQUADRON", "FLEET",
    "NORTH", "SOUTH", "EAST", "WEST", "GRID",
    "URGENT", "PRIORITY", "CONFIRM", "DENIED", "RECEIVED",
    "AMMUNITION", "SUPPLIES", "REINFORCEMENT", "WITHDRAWAL",
    "OPERATION", "MISSION", "TARGET", "OBJECTIVE", "RENDEZVOUS",
    "DAWN", "DUSK", "MIDNIGHT", "ZERO", "HOUR",
    "TORPEDO", "SUBMARINE", "DESTROYER", "CRUISER", "CARRIER",
};

// ---------------------------------------------------------------------------
// Random helpers
// ---------------------------------------------------------------------------

static std::mt19937 rng;

static int rand_int(int lo, int hi) {
    return std::uniform_int_distribution<int>(lo, hi)(rng);
}

static std::vector<int> random_rotor_selection(int total, int count) {
    std::vector<int> pool(total);
    std::iota(pool.begin(), pool.end(), 0);
    std::shuffle(pool.begin(), pool.end(), rng);
    return {pool.begin(), pool.begin() + count};
}

static PlugBoard random_plugboard(int num_pairs) {
    PlugBoard pb = identity_plug();
    std::vector<int> letters(kAlphabetSize);
    std::iota(letters.begin(), letters.end(), 0);
    std::shuffle(letters.begin(), letters.end(), rng);
    for (int i = 0; i < num_pairs * 2; i += 2) {
        pb[letters[i]] = letters[i + 1];
        pb[letters[i + 1]] = letters[i];
    }
    return pb;
}

// Build a plaintext of at least min_len characters by concatenating random
// words from the word list.
static std::string random_plaintext(int min_len) {
    std::string result;
    while (static_cast<int>(result.size()) < min_len) {
        result += kWordList[rand_int(0, static_cast<int>(kWordList.size()) - 1)];
    }
    return result;
}

// ---------------------------------------------------------------------------
// Generate one crib file with fully random settings.
// Returns the seed used so the caller can print it.
// ---------------------------------------------------------------------------
static unsigned generate_one(const std::string& out_path, unsigned seed) {
    rng.seed(seed);

    const int num_rotors       = static_cast<int>(Enigma::catalog().rotors.size());
    const int num_reflectors   = static_cast<int>(Enigma::catalog().reflectors.size());

    EnigmaConfig cfg;
    cfg.rotor_indices   = random_rotor_selection(num_rotors, 3);
    cfg.start_positions = {rand_int(0,25), rand_int(0,25), rand_int(0,25)};
    cfg.ring_settings   = {rand_int(0,25), rand_int(0,25), rand_int(0,25)};
    cfg.reflector_index = rand_int(0, num_reflectors - 1);
    cfg.plugboard       = random_plugboard(rand_int(6, 10));

    // Build 5 random plaintexts of realistic lengths (6–20 chars)
    const std::vector<int> lengths = {6, 16, 13, 7, 13};
    std::vector<std::string> plains;
    for (int len : lengths)
        plains.push_back(random_plaintext(len));

    // Print settings to stdout
    std::cout << "seed=" << seed << "  rotors=[";
    for (int i = 0; i < static_cast<int>(cfg.rotor_indices.size()); ++i) {
        if (i) std::cout << ',';
        std::cout << Enigma::catalog().rotors[cfg.rotor_indices[i]].name;
    }
    std::cout << ']';
    std::cout << "  pos=";
    for (int p : cfg.start_positions) std::cout << idx_to_char(p);
    std::cout << "  ring=";
    for (int r : cfg.ring_settings) std::cout << idx_to_char(r);
    std::cout << "  plug=";
    for (int i = 0; i < kAlphabetSize; ++i)
        if (cfg.plugboard[i] > i)
            std::cout << idx_to_char(i) << idx_to_char(cfg.plugboard[i]);
    std::cout << '\n';

    // Write TOML — fully self-contained: puzzle + ground-truth answer.
    std::ofstream out(out_path);
    out << "# Bombe crib — generated with seed " << seed << "\n";
    out << "# Reproduce: ./build/cli/generate_crib --seed " << seed << "\n\n";

    out << "[settings]\n";

    // Enigma configuration used for encryption (ground truth)
    out << "rotors        = [\"";
    for (int i = 0; i < static_cast<int>(cfg.rotor_indices.size()); ++i) {
        if (i) out << "\", \"";
        out << Enigma::catalog().rotors[cfg.rotor_indices[i]].name;
    }
    out << "\"]\n";
    out << "positions     = \"";
    for (int p : cfg.start_positions) out << idx_to_char(p);
    out << "\"\n";
    out << "ring_settings = ["
        << cfg.ring_settings[0] << ", "
        << cfg.ring_settings[1] << ", "
        << cfg.ring_settings[2] << "]\n";
    out << "reflector     = \""
        << Enigma::catalog().reflectors[cfg.reflector_index].name << "\"\n";
    out << "plugboard     = \"";
    for (int i = 0; i < kAlphabetSize; ++i)
        if (cfg.plugboard[i] > i)
            out << idx_to_char(i) << idx_to_char(cfg.plugboard[i]);
    out << "\"\n\n";

    for (const auto& plain : plains) {
        Enigma machine(cfg);
        std::string cipher = machine.process(plain);
        out << "[[cribs]]\n";
        out << "plain  = \"" << plain  << "\"\n";
        out << "cipher = \"" << cipher << "\"\n\n";
    }

    return seed;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    std::string machine_path = "config/machine.toml";
    std::string out_path     = "cribs/bombe_pc.toml";
    std::string out_dir      = "cribs";
    std::optional<unsigned> fixed_seed;
    int count = 1;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if      (a == "--seed"    && i+1 < argc) fixed_seed = static_cast<unsigned>(std::stoul(argv[++i]));
        else if (a == "--count"   && i+1 < argc) count      = std::stoi(argv[++i]);
        else if (a == "--out"     && i+1 < argc) out_path   = argv[++i];
        else if (a == "--out-dir" && i+1 < argc) out_dir    = argv[++i];
        else if (a == "--machine" && i+1 < argc) machine_path = argv[++i];
    }

    if (!Enigma::load_catalog(machine_path)) {
        std::cerr << "Failed to load " << machine_path << '\n';
        return 1;
    }

    if (!out_dir.empty())
        fs::create_directories(out_dir);

    const auto base_seed = static_cast<unsigned>(
        std::chrono::steady_clock::now().time_since_epoch().count());

    for (int i = 0; i < count; ++i) {
        unsigned seed = fixed_seed ? (*fixed_seed + static_cast<unsigned>(i))
                                   : (base_seed + static_cast<unsigned>(i));

        std::string path;
        if (count > 1) {
            std::ostringstream ss;
            ss << out_dir << "/crib_" << seed << ".toml";
            path = ss.str();
        } else {
            path = out_path;
        }

        generate_one(path, seed);
        std::cout << "  -> " << path << '\n';
    }

    return 0;
}

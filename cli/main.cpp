#include "enigma.hpp"
#include "menu.hpp"
#include "bombe.hpp"
#include "display.hpp"

#define TOML_HEADER_ONLY 1
#include <toml.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;
using namespace bombe;

// ---------------------------------------------------------------------------
// Terminal helpers
// ---------------------------------------------------------------------------

static void print_separator() {
    std::cout << std::string(60, '-') << '\n';
}

static void print_plugboard(const PlugBoard& pb) {
    for (int i = 0; i < kAlphabetSize; ++i) {
        if (pb[i] > i)  // print each pair once
            std::cout << idx_to_char(i) << idx_to_char(pb[i]) << ' ';
    }
    std::cout << '\n';
}

static std::string rotor_name(int idx) {
    const auto& rotors = Enigma::catalog().rotors;
    if (idx < static_cast<int>(rotors.size()))
        return "Rotor-" + rotors[idx].name;
    return "?";
}

static void print_stop(const BombeStop& stop) {
    std::cout << "Rotors   : ";
    for (int r : stop.rotor_indices) std::cout << rotor_name(r) << ' ';
    std::cout << '\n';

    std::cout << "Position : ";
    for (int p : stop.start_positions) std::cout << idx_to_char(p) << ' ';
    std::cout << '\n';

    std::cout << "Ring     : ";
    for (int r : stop.ring_settings) std::cout << idx_to_char(r) << ' ';
    std::cout << '\n';

    std::cout << "Plugboard: ";
    print_plugboard(stop.plugboard);
}

// ---------------------------------------------------------------------------
// Load crib file.
// Supports two formats:
//   TOML (.toml):  [[cribs]] sections with plain = "..." / cipher = "..."
//   CSV  (other):  one PLAIN,CIPHER pair per line (legacy format)
// ---------------------------------------------------------------------------
static Menu load_crib_file(const std::string& path) {
    Menu menu;

    if (path.size() >= 5 && path.substr(path.size() - 5) == ".toml") {
        auto cfg = toml::parse_file(path);
        if (auto* arr = cfg["cribs"].as_array()) {
            for (const auto& node : *arr) {
                const auto& tbl = *node.as_table();
                auto plain  = tbl["plain"].value<std::string>().value();
                auto cipher = tbl["cipher"].value<std::string>().value();
                menu.add_crib(plain, cipher);
            }
        }
    } else {
        std::ifstream f(path);
        if (!f) throw std::runtime_error("Cannot open crib file: " + path);
        std::string line;
        while (std::getline(f, line)) {
            if (line.empty() || line[0] == '#') continue;
            auto comma = line.find(',');
            if (comma == std::string::npos) continue;
            auto plain  = line.substr(0, comma);
            auto cipher = line.substr(comma + 1);
            if (!cipher.empty() && cipher.back() == '\r') cipher.pop_back();
            menu.add_crib(plain, cipher);
        }
    }

    menu.build();
    return menu;
}

// ---------------------------------------------------------------------------
// Load BombeSettings from settings.toml
// ---------------------------------------------------------------------------
static BombeSettings load_settings(const std::string& path,
                                   const Enigma::Catalog& catalog) {
    auto cfg = toml::parse_file(path);
    BombeSettings s;

    s.max_scramblers       = cfg["bombe"]["max_scramblers"].value_or(0);
    s.search_ring_settings = cfg["bombe"]["search_ring_settings"].value_or(false);

    if (auto* arr = cfg["bombe"]["fixed_ring_settings"].as_array()) {
        s.fixed_ring_settings.clear();
        for (const auto& v : *arr)
            s.fixed_ring_settings.push_back(v.value_or(0));
    }

    // Resolve reflector name → index
    auto reflector_name = cfg["enigma"]["reflector"].value_or(std::string("UKW-B"));
    s.reflector_index = 0;
    for (int i = 0; i < static_cast<int>(catalog.reflectors.size()); ++i) {
        if (catalog.reflectors[i].name == reflector_name) {
            s.reflector_index = i;
            break;
        }
    }

    // Optional fixed rotor list (names matching machine.toml e.g. "I", "IV")
    if (auto* arr = cfg["enigma"]["rotors"].as_array()) {
        for (const auto& v : *arr) {
            auto rotor_name = v.value<std::string>().value_or("");
            for (int i = 0; i < static_cast<int>(catalog.rotors.size()); ++i) {
                if (catalog.rotors[i].name == rotor_name) {
                    s.fixed_rotor_indices.push_back(i);
                    break;
                }
            }
        }
    }

    return s;
}

// ---------------------------------------------------------------------------
// Parse ring settings from a 3-character string e.g. "BIN" → {1, 8, 13}
// ---------------------------------------------------------------------------
static std::optional<std::vector<int>> parse_ring_str(const std::string& s) {
    if (s.size() != 3) return std::nullopt;
    return std::vector<int>{
        char_to_idx(static_cast<char>(std::toupper(s[0]))),
        char_to_idx(static_cast<char>(std::toupper(s[1]))),
        char_to_idx(static_cast<char>(std::toupper(s[2])))
    };
}

// ---------------------------------------------------------------------------
// Read optional ring settings embedded in the TOML crib file.
// generate_crib writes a [settings] ring_settings = [...] section.
// ---------------------------------------------------------------------------
static std::optional<std::vector<int>> crib_ring_settings(const std::string& path) {
    if (path.size() < 5 || path.substr(path.size() - 5) != ".toml")
        return std::nullopt;
    auto cfg = toml::parse_file(path);
    if (auto* arr = cfg["settings"]["ring_settings"].as_array()) {
        std::vector<int> r;
        for (const auto& v : *arr) r.push_back(v.value_or(0));
        if (r.size() == 3) return r;
    }
    return std::nullopt;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    std::string machine_path  = "config/machine.toml";
    std::string settings_path = "config/settings.toml";
    std::string crib_path     = "cribs/bombe_pc.toml";

    // Ring-settings override from CLI:
    //   --ring ABC          use known ring settings (no search)
    //   --ring-search       search all 26^2 combinations
    std::optional<std::vector<int>> cli_ring_override;
    bool cli_ring_search = false;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--ring-search") {
            cli_ring_search = true;
        } else if (a == "--ring" && i + 1 < argc) {
            auto r = parse_ring_str(argv[++i]);
            if (!r) { std::cerr << "Error: --ring expects a 3-letter string, e.g. --ring BIN\n"; return 1; }
            cli_ring_override = r;
        } else if (a == "--settings" && i + 1 < argc) {
            settings_path = argv[++i];
        } else if (a == "--machine" && i + 1 < argc) {
            machine_path = argv[++i];
        } else if (a[0] != '-') {
            crib_path = a;
        }
    }

    std::cout << "=== Bombe Enigma Cracker ===\n\n";

    // 1. Load rotor/reflector catalog
    std::cout << "Loading machine config: " << machine_path << '\n';
    if (!Enigma::load_catalog(machine_path)) {
        std::cerr << "Error: failed to load " << machine_path << '\n';
        return 1;
    }
    const auto& cat = Enigma::catalog();
    std::cout << "  " << cat.rotors.size() << " rotors, "
              << cat.reflectors.size() << " reflectors loaded.\n\n";

    // 2. Load crib
    std::cout << "Loading crib: " << crib_path << '\n';
    Menu menu = load_crib_file(crib_path);
    std::cout << "  " << menu.plain_texts().size() << " crib pairs loaded.\n";
    std::cout << menu.summary();
    print_separator();

    // 3. Load settings, then apply overrides in priority order:
    //    CLI flag > crib-embedded settings > settings.toml
    std::cout << "Loading settings: " << settings_path << '\n';
    BombeSettings settings = load_settings(settings_path, cat);

    if (cli_ring_search) {
        settings.search_ring_settings = true;
        std::cout << "  Ring mode       : search (--ring-search)\n";
    } else if (cli_ring_override) {
        settings.search_ring_settings = false;
        settings.fixed_ring_settings  = *cli_ring_override;
        std::cout << "  Ring mode       : known (--ring "
                  << idx_to_char((*cli_ring_override)[0])
                  << idx_to_char((*cli_ring_override)[1])
                  << idx_to_char((*cli_ring_override)[2]) << ")\n";
    } else if (auto embedded = crib_ring_settings(crib_path)) {
        settings.search_ring_settings = false;
        settings.fixed_ring_settings  = *embedded;
        std::cout << "  Ring mode       : known from crib ("
                  << idx_to_char((*embedded)[0])
                  << idx_to_char((*embedded)[1])
                  << idx_to_char((*embedded)[2]) << ")\n";
        // Also pick up reflector from crib if present
        if (crib_path.size() >= 5 && crib_path.substr(crib_path.size()-5) == ".toml") {
            auto cfg = toml::parse_file(crib_path);
            if (auto ref_name = cfg["settings"]["reflector"].value<std::string>()) {
                for (int i = 0; i < static_cast<int>(cat.reflectors.size()); ++i) {
                    if (cat.reflectors[i].name == *ref_name) {
                        settings.reflector_index = i;
                        break;
                    }
                }
            }
        }
    } else {
        std::cout << "  Ring mode       : "
                  << (settings.search_ring_settings ? "search" : "fixed AAA") << '\n';
    }

    std::cout << "  Reflector index : " << settings.reflector_index << '\n';
    std::cout << "  Max scramblers  : "
              << (settings.max_scramblers == 0 ? "unlimited" : std::to_string(settings.max_scramblers))
              << '\n';
    print_separator();

    // 4. Run the Bombe with live display
    Display display;
    display.init(menu);

    // Rotor-combo counting: track combo index for display
    int combo_index = 0;
    std::vector<int> last_rotors;

    // Collect stops as they come in so we can announce them immediately
    std::vector<BombeStop> found_stops;

    const int num_rotors = static_cast<int>(Enigma::catalog().rotors.size());
    int combo_total = settings.fixed_rotor_indices.empty()
        ? num_rotors * (num_rotors-1) * (num_rotors-2)
        : 1;

    Bombe bombe(menu, settings);
    found_stops = bombe.crack(
        [&](const std::vector<int>& rotors,
            const std::vector<int>& positions,
            int tested, int total) {
            if (rotors != last_rotors) { ++combo_index; last_rotors = rotors; }
            display.update(rotors, positions, tested, total,
                           combo_index, combo_total,
                           static_cast<int>(found_stops.size()));
        },
        [&](const BombeStop& stop) {
            display.announce_stop(stop);   // real-time stop highlight
        }
    );

    display.finish();

    // 5. Print results below the dashboard
    print_separator();
    if (found_stops.empty()) {
        std::cout << "No solution found.\n"
                  << "  - Check crib text correctness\n"
                  << "  - Ring settings may differ (try --ring-search)\n"
                  << "  - Rotor subset may not match actual settings\n";
    } else {
        std::cout << "Found " << found_stops.size() << " solution(s):\n\n";
        for (int i = 0; i < static_cast<int>(found_stops.size()); ++i) {
            std::cout << "--- Solution " << (i + 1) << " ---\n";
            print_stop(found_stops[i]);
            std::cout << '\n';
        }
    }

    return 0;
}

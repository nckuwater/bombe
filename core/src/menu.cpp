#include "menu.hpp"
#include <algorithm>
#include <functional>
#include <sstream>
#include <stdexcept>

namespace bombe {

void Menu::add_crib(std::string_view plain, std::string_view cipher) {
    if (plain.size() != cipher.size())
        throw std::invalid_argument("plain and cipher must be the same length");

    plains_.emplace_back(plain);
    ciphers_.emplace_back(cipher);

    for (int i = 0; i < static_cast<int>(plain.size()); ++i) {
        MenuEdge e;
        // Each crib is an independent message encrypted from the same starting
        // position, so offsets are local (0-based) within each crib.
        e.crib_offset = i;
        e.letter_a    = char_to_idx(plain[i]);
        e.letter_b    = char_to_idx(cipher[i]);

        int edge_idx = static_cast<int>(edges_.size());
        adjacency_[e.letter_a].emplace_back(edge_idx, e.letter_b);
        adjacency_[e.letter_b].emplace_back(edge_idx, e.letter_a);

        edges_.push_back(e);
    }
}

// ---------------------------------------------------------------------------
// Cycle finding using DFS on the undirected menu graph.
// A cycle is detected when we reach a node that is already on the current
// DFS path (back-edge). We extract the cycle as the sub-path from that node.
//
// Only unique cycles are kept (checked by sorted edge-index sets).
// ---------------------------------------------------------------------------
void Menu::find_cycles() {
    cycles_.clear();

    // DFS state
    std::vector<int>       path_nodes;   // current DFS path (letter indices)
    std::vector<int>       path_edges;   // edge taken to reach each node
    std::vector<bool>      on_stack(kAlphabetSize, false);
    std::vector<bool>      visited(kAlphabetSize, false);

    // Keep track of unique cycles by their sorted edge-index signature
    std::vector<std::vector<int>> seen_signatures;

    std::function<void(int, int)> dfs = [&](int node, int parent_edge) {
        visited[node]  = true;
        on_stack[node] = true;
        path_nodes.push_back(node);
        path_edges.push_back(parent_edge);

        for (auto [edge_idx, neighbour] : adjacency_[node]) {
            if (edge_idx == parent_edge) continue;  // don't go back on same edge

            if (on_stack[neighbour]) {
                // Found a cycle: extract from path_nodes
                auto it = std::find(path_nodes.begin(), path_nodes.end(), neighbour);
                if (it == path_nodes.end()) continue;

                int cycle_start = static_cast<int>(it - path_nodes.begin());
                std::vector<MenuEdge> cycle;
                std::vector<int>      sig;

                for (int k = cycle_start + 1; k < static_cast<int>(path_nodes.size()); ++k) {
                    cycle.push_back(edges_[path_edges[k]]);
                    sig.push_back(path_edges[k]);
                }
                // closing edge
                cycle.push_back(edges_[edge_idx]);
                sig.push_back(edge_idx);

                std::sort(sig.begin(), sig.end());
                if (std::find(seen_signatures.begin(), seen_signatures.end(), sig)
                        == seen_signatures.end()) {
                    seen_signatures.push_back(sig);
                    cycles_.push_back(cycle);
                }
            } else if (!visited[neighbour]) {
                dfs(neighbour, edge_idx);
            }
        }

        path_nodes.pop_back();
        path_edges.pop_back();
        on_stack[node] = false;
    };

    for (int start = 0; start < kAlphabetSize; ++start) {
        if (!visited[start] && !adjacency_[start].empty())
            dfs(start, -1);
    }

    // Sort cycles: longest first (more edges → stronger constraints)
    std::sort(cycles_.begin(), cycles_.end(),
              [](const auto& a, const auto& b) { return a.size() > b.size(); });
}

void Menu::select_test_letter() {
    // Choose the letter with the highest degree in the menu graph.
    // Higher degree means more constraint propagation from a single assumption.
    int best = 0;
    for (int i = 1; i < kAlphabetSize; ++i) {
        if (adjacency_[i].size() > adjacency_[best].size())
            best = i;
    }
    test_letter_ = best;
}

void Menu::build() {
    find_cycles();
    select_test_letter();
}

std::string Menu::summary() const {
    std::ostringstream ss;
    ss << "Menu: " << edges_.size() << " edges, "
       << cycles_.size() << " cycles found.\n";
    ss << "Test letter: " << idx_to_char(test_letter_)
       << " (degree " << adjacency_[test_letter_].size() << ")\n";
    return ss.str();
}

} // namespace bombe

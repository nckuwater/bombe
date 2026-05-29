#include "menu.hpp"
#include <gtest/gtest.h>

using namespace bombe;

TEST(MenuTest, EdgeCountMatchesCribLength) {
    Menu m;
    m.add_crib("HELLO", "WORLD");  // 5 chars → 5 edges
    m.build();
    EXPECT_EQ(m.edges().size(), 5u);
}

TEST(MenuTest, MultiCribEdgeCount) {
    Menu m;
    m.add_crib("AB", "CD");   // 2 edges
    m.add_crib("EFG", "HIJ"); // 3 edges
    m.build();
    EXPECT_EQ(m.edges().size(), 5u);
}

TEST(MenuTest, EdgeLettersCorrect) {
    Menu m;
    m.add_crib("ACE", "BDF");
    m.build();

    const auto& edges = m.edges();
    ASSERT_EQ(edges.size(), 3u);

    EXPECT_EQ(edges[0].letter_a, char_to_idx('A'));
    EXPECT_EQ(edges[0].letter_b, char_to_idx('B'));
    EXPECT_EQ(edges[0].crib_offset, 0);

    EXPECT_EQ(edges[1].letter_a, char_to_idx('C'));
    EXPECT_EQ(edges[1].letter_b, char_to_idx('D'));
    EXPECT_EQ(edges[1].crib_offset, 1);
}

TEST(MenuTest, EachCribHasLocalOffsets) {
    // Each crib is an independent message that starts with the same Enigma
    // position, so offsets reset to 0 for every new crib.
    Menu m;
    m.add_crib("AB", "CD");   // offsets 0, 1
    m.add_crib("EF", "GH");   // also offsets 0, 1 (new message, same start)
    m.build();

    const auto& edges = m.edges();
    EXPECT_EQ(edges[2].crib_offset, 0);
    EXPECT_EQ(edges[3].crib_offset, 1);
}

TEST(MenuTest, CycleFoundInSimpleLoop) {
    // A-B-A creates a loop: two edges both touching A and B.
    Menu m;
    m.add_crib("AB", "BA");   // edge(0): A<->B at offset 0
                               // edge(1): B<->A at offset 1
    m.build();
    // Both edges connect A and B — there should be at least one cycle.
    EXPECT_GE(m.cycles().size(), 1u);
}

TEST(MenuTest, TestLetterHasHighestDegree) {
    Menu m;
    // Make A appear 3 times, B and C appear twice, D once
    m.add_crib("AAAB", "BCDB");  // A:B(0), A:C(1), A:D(2), B:B(3)
    m.build();
    // Test letter should be A (degree 3) or B (degree 3, tie — either is fine)
    int tl = m.test_letter();
    // Count degree of chosen test letter
    int degree = 0;
    for (const auto& e : m.edges()) {
        if (e.letter_a == tl || e.letter_b == tl) ++degree;
    }
    // Should be the max degree
    int max_degree = 0;
    for (int i = 0; i < kAlphabetSize; ++i) {
        int d = 0;
        for (const auto& e : m.edges())
            if (e.letter_a == i || e.letter_b == i) ++d;
        max_degree = std::max(max_degree, d);
    }
    EXPECT_EQ(degree, max_degree);
}

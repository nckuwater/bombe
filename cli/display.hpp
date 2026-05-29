#pragma once
#include "enigma.hpp"
#include "menu.hpp"
#include "bombe.hpp"
#include "types.hpp"

#include <chrono>
#include <cstdio>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// ANSI helpers
// ---------------------------------------------------------------------------
namespace ansi {
    inline const char* reset        = "\033[0m";
    inline const char* bold         = "\033[1m";
    inline const char* dim          = "\033[2m";
    inline const char* red          = "\033[31m";
    inline const char* green        = "\033[32m";
    inline const char* yellow       = "\033[33m";
    inline const char* cyan         = "\033[36m";
    inline const char* white        = "\033[97m";
    inline const char* hide_cursor  = "\033[?25l";
    inline const char* show_cursor  = "\033[?25h";
    inline const char* clear_eol    = "\033[K";

    inline std::string up(int n)   { return "\033[" + std::to_string(n) + "A"; }
    inline std::string col1()      { return "\r"; }
    inline std::string clear_line(){ return "\r\033[K"; }
}

// ---------------------------------------------------------------------------
// Display
//
// Layout (kRows = 14 lines, rows are 0-indexed):
//  0  +=====+  top border
//  1  | title
//  2  +-----+
//  3  | menu info
//  4  +-----+
//  5  | rotor names + combo               ← dynamic
//  6  | notch positions                   ← dynamic
//  7  |
//  8  | drums (position letters)          ← dynamic
//  9  |
// 10  +-----+
// 11  | progress bar                      ← dynamic
// 12  | speed / elapsed                   ← dynamic
// 13  +-----+
// 14  | stops                             ← dynamic
// 15  +=====+  bottom border
//
// Strategy: reserve kRows blank lines at `init()`, then use cursor-up
// to paint back over them. This avoids absolute screen coordinates so
// multiple consecutive runs never overlap.
// ---------------------------------------------------------------------------
class Display {
public:
    using Clock = std::chrono::steady_clock;

    static constexpr int kWidth  = 62;
    static constexpr int kRows   = 16;   // total lines in the block
    static constexpr int kFps    = 20;
    static constexpr int kMinMs  = 1000 / kFps;  // 50 ms between redraws

    void init(const bombe::Menu& menu) {
        menu_summary_ = build_menu_summary(menu);
        start_time_   = Clock::now();
        last_frame_   = Clock::now() - std::chrono::milliseconds(kMinMs + 1);
        last_rotors_.clear();
        stops_count_  = 0;

        std::fputs(ansi::hide_cursor, stdout);

        // Reserve kRows lines by printing blank lines, then move cursor
        // back to the top of the block.  This ensures the display always
        // sits in its own contiguous region regardless of prior output.
        for (int i = 0; i < kRows; ++i) std::fputs("\n", stdout);
        std::fputs(ansi::up(kRows).c_str(), stdout);

        draw_static_frame();
        std::fflush(stdout);
    }

    void finish() {
        // Park cursor below the block and restore it.
        move_to_row(kRows);
        std::fputs("\n", stdout);
        std::fputs(ansi::show_cursor, stdout);
        std::fflush(stdout);
    }

    void update(const std::vector<int>& rotors,
                const std::vector<int>& positions,
                int tested, int total,
                int combo_index, int combo_total,
                int stops_found)
    {
        auto now = Clock::now();
        int elapsed_ms = static_cast<int>(
            std::chrono::duration_cast<std::chrono::milliseconds>(now - last_frame_).count());
        if (elapsed_ms < kMinMs) return;
        last_frame_ = now;

        int total_ms = static_cast<int>(
            std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_).count());

        if (rotors != last_rotors_) {
            last_rotors_ = rotors;
            draw_rotor_row(rotors, combo_index, combo_total);
        }
        draw_drums(positions);
        draw_progress(tested, total, total_ms);
        if (stops_found != stops_count_) {
            stops_count_ = stops_found;
            draw_stops(stops_count_);
        }
        std::fflush(stdout);
    }

    void announce_stop(const bombe::BombeStop& stop) {
        ++stops_count_;

        // Update the count in row 14 first.
        draw_stops(stops_count_);

        // Append the stop details as a new line BELOW the bottom border.
        // Cursor is currently parked at (kRows + extra_lines_) below block top.
        // Print on a fresh line outside the fixed block so it persists.
        const auto& cat = bombe::Enigma::catalog();
        std::string info = std::string(ansi::bold) + std::string(ansi::green)
                         + "  STOP " + std::to_string(stops_count_) + ":  ";
        for (int r : stop.rotor_indices) info += cat.rotors[r].name + "-";
        if (!info.empty() && info.back() == '-') info.pop_back();
        info += "  pos:";
        for (int p : stop.start_positions) info += " " + std::string(1, bombe::idx_to_char(p));
        info += "  ring:";
        for (int r : stop.ring_settings)   info += " " + std::string(1, bombe::idx_to_char(r));
        info += std::string(ansi::reset);

        std::fputs(("\n\033[K" + info).c_str(), stdout);
        ++extra_lines_;

        std::fflush(stdout);
    }

private:
    std::string menu_summary_;
    Clock::time_point start_time_;
    Clock::time_point last_frame_;
    std::vector<int> last_rotors_;
    int stops_count_ = 0;

    // Lines appended below the fixed block (one per stop announcement).
    // update_row() adds this offset so cursor-up arithmetic stays correct.
    int extra_lines_ = 0;

    // Sliding window for speed calculation.
    // Reset when the rotor combo changes so stale tested-counts from the
    // previous combo don't produce negative speed values.
    static constexpr int kSpeedWindow = 8;
    struct Sample { Clock::time_point t; int tested; };
    Sample speed_buf_[kSpeedWindow]{};
    int speed_head_ = 0;
    int speed_size_ = 0;

    // -------------------------------------------------------------------------
    // Move cursor to a row within the reserved block (0 = top border).
    // Uses relative cursor-up from the BOTTOM of the block to avoid
    // dependence on absolute screen row numbers.
    // After this call, cursor is at column 1 of the target row.
    // -------------------------------------------------------------------------
    void move_to_row(int row) const {
        // Go to column 1 of the last row in the block, then up as needed.
        int up_count = kRows - 1 - row;
        // Move to bottom of block: move down (kRows-1 - current_row_from_top)
        // Since we don't track current row, we go to bottom and come back up.
        // Simpler: always go to top of block first using a known reference.
        // We track current position implicitly by always going "bottom then up".
        // Actually: just use relative moves from wherever we are.
        // We stay at the bottom of the drawn content between calls, so:
        std::fputs(ansi::up(kRows - 1 - row).c_str(), stdout);
        std::fputs(ansi::col1().c_str(), stdout);
    }

    // Draw from the current cursor position (assumes cursor is at target row col 1).
    void draw_line(const std::string& content) {
        std::fputs(ansi::clear_eol, stdout);
        std::fputs(content.c_str(), stdout);
        std::fputs("\n", stdout);
    }

    // -------------------------------------------------------------------------
    // Go to a specific row by moving from row 0 (we redraw sequentially).
    // Used only during static frame draw where we go top-to-bottom in order.
    // -------------------------------------------------------------------------
    void seek_to(int target_row, int& current_row) {
        while (current_row < target_row) {
            std::fputs("\n", stdout);
            ++current_row;
        }
        std::fputs(ansi::col1().c_str(), stdout);
    }

    // -------------------------------------------------------------------------
    static std::string pad(const std::string& s, int w) {
        if (static_cast<int>(s.size()) >= w) return s;
        return s + std::string(w - s.size(), ' ');
    }
    static std::string center(const std::string& s, int w) {
        if (static_cast<int>(s.size()) >= w) return s;
        int p = w - static_cast<int>(s.size()), l = p/2;
        return std::string(l, ' ') + s + std::string(p - l, ' ');
    }
    static std::string elapsed_str(int ms) {
        int s = ms / 1000;
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%02d:%02d", s/60, s%60);
        return buf;
    }
    static std::string fmt_int(int n) {
        std::string s = std::to_string(n);
        for (int i = static_cast<int>(s.size()) - 3; i > 0; i -= 3)
            s.insert(i, ",");
        return s;
    }
    static std::string build_menu_summary(const bombe::Menu& m) {
        return "edges " + std::to_string(m.edges().size())
             + "  ·  cycles " + std::to_string(m.cycles().size())
             + "  ·  test " + std::string(1, bombe::idx_to_char(m.test_letter()));
    }

    // -------------------------------------------------------------------------
    // Static frame — drawn once in sequential top-to-bottom order
    // -------------------------------------------------------------------------
    void draw_static_frame() {
        const int W = kWidth;
        auto border  = [&](char c){ return "+" + std::string(W, c) + "+"; };
        auto content = [&](const std::string& s){ return "|" + pad(s, W) + "|"; };

        auto ln = [&](const std::string& s) {
            std::fputs(ansi::clear_eol, stdout);
            std::fputs(s.c_str(), stdout);
            std::fputs("\n", stdout);
        };

        ln(border('='));  // row 0
        ln("|" + std::string(ansi::bold) + std::string(ansi::yellow)
           + center("B O M B E   —   Enigma Cracker", W)
           + std::string(ansi::reset) + "|");  // row 1
        ln(border('-'));  // row 2
        ln(content("  Menu · " + menu_summary_));  // row 3
        ln(border('-'));  // row 4
        ln(content(""));  // row 5  — rotor names (dynamic)
        ln(content(""));  // row 6  — notch
        ln(content(""));  // row 7  — blank
        ln(content(""));  // row 8  — drums
        ln(content(""));  // row 9  — blank
        ln(border('-'));  // row 10
        ln(content(""));  // row 11 — progress
        ln(content(""));  // row 12 — speed/elapsed
        ln(border('-'));  // row 13
        ln("|  " + std::string(ansi::dim) + "Stops: 0"
           + std::string(ansi::reset) + std::string(W - 8, ' ') + "|");  // row 14
        ln(border('='));  // row 15
        // cursor is now one line below the block
    }

    // -------------------------------------------------------------------------
    // Dynamic rows — each jumps to its row from the current (bottom) position
    // -------------------------------------------------------------------------

    // Move up from the current parked position (bottom of block + extra_lines_)
    // to target_row, write content, then return to the parked position.
    void update_row(int target_row, const std::string& content) {
        int up = kRows + extra_lines_ - target_row;
        fprintf(stdout, "\033[%dA", up);
        std::fputs("\r\033[K", stdout);
        std::fputs(content.c_str(), stdout);
        fprintf(stdout, "\033[%dB", up);
        std::fputs("\r", stdout);
    }

    void draw_rotor_row(const std::vector<int>& rotors, int combo, int total) {
        // Reset sliding-window speed buffer: previous combo's tested counts
        // must not bleed into this combo's speed calculation.
        speed_size_ = 0;
        speed_head_ = 0;

        const auto& cat = bombe::Enigma::catalog();
        std::string names;
        for (int r : rotors) names += "[ " + cat.rotors[r].name + " ] ";

        std::string notches = "  notch:";
        for (int r : rotors) notches += "  " + std::string(1, bombe::idx_to_char(cat.rotors[r].turnover_notch));

        std::string combo_s = "combo " + std::to_string(combo) + " / " + std::to_string(total);

        // Row 5: rotor names right-aligned combo counter
        int name_w = kWidth - 2 - static_cast<int>(combo_s.size()) - 2;
        update_row(5, "|  " + std::string(ansi::bold) + std::string(ansi::cyan)
                      + pad(names, name_w) + std::string(ansi::reset)
                      + std::string(ansi::dim) + combo_s + std::string(ansi::reset) + "  |");

        // Row 6: notch
        update_row(6, "|" + std::string(ansi::dim) + pad(notches, kWidth)
                     + std::string(ansi::reset) + "|");
    }

    void draw_drums(const std::vector<int>& positions) {
        static const char spin[] = "|/-\\";
        static int si = 0;
        si = (si + 1) % 4;

        std::string d = "  ";
        for (int p : positions)
            d += " [" + std::string(ansi::bold) + std::string(ansi::white)
               + std::string(1, bombe::idx_to_char(p))
               + std::string(ansi::reset) + "]";
        d += "  " + std::string(ansi::dim) + std::string(1, spin[si]) + std::string(ansi::reset);

        update_row(8, "|" + pad(d, kWidth) + "|");
    }

    void draw_progress(int tested, int total, int elapsed_ms) {
        const int bar_w = 28;
        int filled = (total > 0) ? (tested * bar_w / total) : 0;
        int pct    = (total > 0) ? (tested * 100  / total) : 0;

        std::string bar =
            std::string(ansi::green) + std::string(filled, '#') + std::string(ansi::reset) +
            std::string(ansi::dim)   + std::string(bar_w - filled, '.') + std::string(ansi::reset);

        std::string prog = "  [" + bar + "]  " + std::to_string(pct) + "%  "
                         + fmt_int(tested) + " / " + fmt_int(total);
        update_row(11, "|" + pad(prog, kWidth) + "|");

        // Sliding-window speed: add current sample, compute over oldest window entry.
        auto now = Clock::now();
        speed_buf_[speed_head_] = {now, tested};
        speed_head_ = (speed_head_ + 1) % kSpeedWindow;
        if (speed_size_ < kSpeedWindow) ++speed_size_;

        double speed = 0;
        if (speed_size_ >= 2) {
            int oldest_idx = (speed_head_ + kSpeedWindow - speed_size_) % kSpeedWindow;
            const auto& oldest = speed_buf_[oldest_idx];
            const auto& newest = speed_buf_[(speed_head_ + kSpeedWindow - 1) % kSpeedWindow];
            int dt_ms = static_cast<int>(
                std::chrono::duration_cast<std::chrono::milliseconds>(newest.t - oldest.t).count());
            if (dt_ms > 0) speed = (newest.tested - oldest.tested) * 1000.0 / dt_ms;
        }

        std::string sp = "  speed " + pad(fmt_int(static_cast<int>(speed)) + "/s", 12)
                       + "elapsed " + elapsed_str(elapsed_ms);
        update_row(12, "|" + std::string(ansi::dim) + pad(sp, kWidth) + std::string(ansi::reset) + "|");
    }

    void draw_stops(int count) {
        std::string label = count == 0
            ? std::string(ansi::dim) + "Stops: 0" + std::string(ansi::reset)
            : std::string(ansi::bold) + std::string(ansi::green)
              + "Stops: " + std::to_string(count) + std::string(ansi::reset);
        update_row(14, "|  " + pad(label, kWidth - 2) + "|");
    }
};

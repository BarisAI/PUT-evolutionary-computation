#pragma once
#include "../../Assignment 1/src/problem.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <numeric>
#include <random>
#include <stdexcept>
#include <vector>

enum class LocalStrategy { Steepest, Greedy };
enum class IntraMove { NodeSwap, TwoOpt };
enum class MoveType : std::uint8_t { Intra, Inter };
struct Move { MoveType type; int i, j; };
struct SearchStats { Tour tour; Cost objective; int accepted_moves; std::uint64_t delta_checks; };

// Swap two selected positions; sum each affected directed edge exactly once,
// including adjacent and wrap-around cases. Selected-node costs do not change.
inline Cost node_swap_delta(const Problem& p, const Tour& t, int i, int j) {
    const int m = static_cast<int>(t.size());
    if (i == j || m < 2) return 0;
    std::array<int, 4> affected{(i + m - 1) % m, i, (j + m - 1) % m, j};
    std::sort(affected.begin(), affected.end());
    auto after = [&](int pos) { return pos == i ? t[j] : (pos == j ? t[i] : t[pos]); };
    Cost delta = 0;
    for (int q = 0; q < 4; ++q) {
        if (q && affected[q] == affected[q - 1]) continue;
        const int edge = affected[q], next = (edge + 1) % m;
        delta += p.distance[after(edge)][after(next)] - p.distance[t[edge]][t[next]];
    }
    return delta;
}

// 2-opt reverses the segment from i+1 through j; only two boundary edges change.
inline Cost two_opt_delta(const Problem& p, const Tour& t, int i, int j) {
    const int m = static_cast<int>(t.size());
    const int a = t[i], b = t[(i + 1) % m], c = t[j], d = t[(j + 1) % m];
    return p.distance[a][c] + p.distance[b][d] - p.distance[a][b] - p.distance[c][d];
}

// Replace one selected node with one currently unselected node at the same position.
inline Cost replacement_delta(const Problem& p, const Tour& t, int i, int outside) {
    const int m = static_cast<int>(t.size());
    const int old = t[i];
    if (m == 1) return p.cost[outside] - p.cost[old];
    const int before = t[(i + m - 1) % m], after = t[(i + 1) % m];
    return p.cost[outside] - p.cost[old]
         + p.distance[before][outside] + p.distance[outside][after]
         - p.distance[before][old] - p.distance[old][after];
}

inline std::vector<int> outside_nodes(const Problem& p, const Tour& t) {
    std::vector<bool> selected(p.size(), false);
    for (int node : t) selected[node] = true;
    std::vector<int> outside;
    outside.reserve(p.size() - t.size());
    for (int v = 0; v < p.size(); ++v) if (!selected[v]) outside.push_back(v);
    return outside;
}

// Move descriptors are generated before a scan; objective deltas are NEVER cached.
inline std::vector<Move> make_moves(int m, int outside_count, IntraMove intra) {
    std::vector<Move> moves;
    moves.reserve(static_cast<std::size_t>(m) * (m - 1) / 2 +
                  static_cast<std::size_t>(m) * outside_count);
    for (int i = 0; i < m; ++i) {
        for (int j = i + 1; j < m; ++j) {
            // Exclude 2-opt no-ops (adjacent edges and reversing the entire tour).
            if (intra == IntraMove::TwoOpt && (j == i + 1 || (i == 0 && j == m - 1))) continue;
            moves.push_back({MoveType::Intra, i, j});
        }
    }
    for (int i = 0; i < m; ++i)
        for (int k = 0; k < outside_count; ++k)
            moves.push_back({MoveType::Inter, i, k});
    return moves;
}

inline Cost move_delta(const Problem& p, const Tour& t, const std::vector<int>& outside,
                       IntraMove intra, const Move& move) {
    if (move.type == MoveType::Inter) return replacement_delta(p, t, move.i, outside[move.j]);
    return intra == IntraMove::NodeSwap ? node_swap_delta(p, t, move.i, move.j)
                                        : two_opt_delta(p, t, move.i, move.j);
}

inline void apply_move(Tour& t, const std::vector<int>& outside, IntraMove intra, const Move& move) {
    if (move.type == MoveType::Inter) t[move.i] = outside[move.j];
    else if (intra == IntraMove::NodeSwap) std::swap(t[move.i], t[move.j]);
    else std::reverse(t.begin() + move.i + 1, t.begin() + move.j + 1);
}

inline SearchStats local_search(const Problem& p, Tour initial, LocalStrategy strategy,
                                 IntraMove intra, std::mt19937& rng) {
    Cost current = evaluate(p, initial);  // One-time initial value, not per neighbor.
    SearchStats stats{std::move(initial), current, 0, 0};
    const int m = static_cast<int>(stats.tour.size());
    // The full neighborhood is represented by position pairs (not candidate moves).
    // These descriptors remain valid because the number of selected nodes is fixed.
    // We never store, reuse or precompute any objective deltas.
    auto moves = make_moves(m, p.size() - m, intra);
    while (true) {
        const std::vector<int> outside = outside_nodes(p, stats.tour);
        Cost best_delta = 0;
        Move chosen{};
        if (strategy == LocalStrategy::Greedy) {
            // Basic first improvement with a uniformly randomized move order.
            // Online Fisher-Yates chooses the next move only when we need it;
            // as soon as a negative delta is found, the scan stops immediately.
            // Both inter-route and intra-route moves are in the SAME pool.
            for (std::size_t k = 0; k < moves.size(); ++k) {
                std::uniform_int_distribution<std::size_t> pick(k, moves.size() - 1);
                std::swap(moves[k], moves[pick(rng)]);
                ++stats.delta_checks;
                const Cost delta = move_delta(p, stats.tour, outside, intra, moves[k]);
                if (delta < 0) { best_delta = delta; chosen = moves[k]; break; }
            }
        } else {
            // Steepest: visit the ENTIRE joint neighborhood and select the
            // most negative delta, not the best within just one move class.
            for (const Move& move : moves) {
                ++stats.delta_checks;
                const Cost delta = move_delta(p, stats.tour, outside, intra, move);
                if (delta < best_delta) { best_delta = delta; chosen = move; }
            }
        }
        if (best_delta == 0) break;
        apply_move(stats.tour, outside, intra, chosen);
        stats.objective += best_delta;
        ++stats.accepted_moves;
    }
    return stats;
}

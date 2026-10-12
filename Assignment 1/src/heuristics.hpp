#pragma once
#include "problem.hpp"
#include <limits>
#include <numeric>
#include <random>

inline Tour random_solution(const Problem& p, std::mt19937& rng) {
    Tour tour(p.size());
    std::iota(tour.begin(), tour.end(), 0);
    // Explicit Fisher-Yates: fixed rejection sampling for reproducibility.
    for (int i = p.size() - 1; i > 0; --i) {
        const std::uint64_t range = static_cast<std::uint64_t>(i) + 1;
        const std::uint64_t limit = (std::uint64_t{1} << 32) / range * range;
        std::uint64_t draw;
        do { draw = rng(); } while (draw >= limit);
        std::swap(tour[i], tour[draw % range]);
    }
    tour.resize(p.selected_count());
    return tour;
}

enum class Method { NearestEnd, NearestAllPositions, GreedyCycle };

inline Cost insertion_delta(const Problem& p, const Tour& tour, int node, int position, bool closed) {
    const int m = static_cast<int>(tour.size());
    if (closed) {
        const int a = tour[(position + m - 1) % m], b = tour[position % m];
        return p.cost[node] + p.distance[a][node] + p.distance[node][b] - p.distance[a][b];
    }
    if (position == 0) return p.cost[node] + p.distance[node][tour.front()];
    if (position == m) return p.cost[node] + p.distance[tour.back()][node];
    const int a = tour[position - 1], b = tour[position];
    return p.cost[node] + p.distance[a][node] + p.distance[node][b] - p.distance[a][b];
}

inline Tour greedy_solution(const Problem& p, int start, Method method) {
    if (start < 0 || start >= p.size()) throw std::runtime_error("Invalid starting node");
    Tour tour{start};
    std::vector<bool> used(p.size());
    used[start] = true;
    while (static_cast<int>(tour.size()) < p.selected_count()) {
        Cost best = std::numeric_limits<Cost>::max();
        int best_node = -1, best_position = -1;
        const int m = static_cast<int>(tour.size());
        for (int node = 0; node < p.size(); ++node) {
            if (used[node]) continue;
            const int first = method == Method::NearestEnd ? m : 0;
            // A cycle has m distinct edges; position m duplicates position 0.
            const int last = method == Method::GreedyCycle ? m - 1 : m;
            for (int position = first; position <= last; ++position) {
                const Cost delta = insertion_delta(p, tour, node, position, method == Method::GreedyCycle);
                // Ascending node/position iteration breaks ties deterministically.
                if (delta < best) { best = delta; best_node = node; best_position = position; }
            }
        }
        tour.insert(tour.begin() + best_position, best_node);
        used[best_node] = true;
    }
    return tour;
}

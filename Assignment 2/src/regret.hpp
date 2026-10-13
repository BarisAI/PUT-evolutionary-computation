#pragma once
#include "../../Assignment 1/src/problem.hpp"
#include <limits>
#include <stdexcept>

// All-position nearest neighbor maintains an open path; greedy cycle a closed cycle.
enum class Basis { NearestAllPositions, GreedyCycle };
enum class Criterion { TwoRegret, WeightedTwoRegret };

struct InsertionChoice {
    Cost best, second, regret;
    int position;
};

// Return the two smallest insertion costs for this candidate node.
inline InsertionChoice insertion_choices(const Problem& p, const Tour& path,
                                         int candidate, Basis basis) {
    if (path.empty()) throw std::runtime_error("Cannot insert into empty path");
    const int m = static_cast<int>(path.size());
    const bool closed = basis == Basis::GreedyCycle;
    Cost best = std::numeric_limits<Cost>::max();
    Cost second = std::numeric_limits<Cost>::max();
    int best_position = -1;
    const int num_positions = closed ? m : m + 1;
    for (int pos = 0; pos < num_positions; ++pos) {
        Cost delta = p.cost[candidate];
        if (closed) {
            const int before = path[(pos + m - 1) % m], after = path[pos];
            delta += p.distance[before][candidate]
                   + p.distance[candidate][after] - p.distance[before][after];
        } else if (pos == 0) {
            delta += p.distance[candidate][path.front()];
        } else if (pos == m) {
            delta += p.distance[path.back()][candidate];
        } else {
            const int before = path[pos - 1], after = path[pos];
            delta += p.distance[before][candidate]
                   + p.distance[candidate][after] - p.distance[before][after];
        }
        if (delta < best) {
            second = best;
            best = delta;
            best_position = pos;
        } else if (delta < second) {
            second = delta;
        }
    }
    // Only one insertion edge is available in the one-node closed cycle.
    if (second == std::numeric_limits<Cost>::max()) second = best;
    return {best, second, second - best, best_position};
}

inline Tour regret_solution(const Problem& p, int start, Basis basis, Criterion method,
                            double regret_weight = 0.5, double cost_weight = 0.5) {
    if (start < 0 || start >= p.size()) throw std::runtime_error("Invalid starting node");
    if (regret_weight < 0 || cost_weight < 0 ||
        (regret_weight == 0 && cost_weight == 0))
        throw std::runtime_error("Weights must be nonnegative and not both zero");
    Tour tour{start};
    std::vector<bool> used(p.size());
    used[start] = true;
    while (static_cast<int>(tour.size()) < p.selected_count()) {
        double best_score = -std::numeric_limits<double>::infinity();
        Cost tie_best_cost = std::numeric_limits<Cost>::max();
        int best_node = -1, best_position = -1;
        for (int candidate = 0; candidate < p.size(); ++candidate) {
            if (used[candidate]) continue;
            const auto option = insertion_choices(p, tour, candidate, basis);
            const double score = method == Criterion::TwoRegret
                ? static_cast<double>(option.regret)
                : regret_weight * static_cast<double>(option.regret)
                    - cost_weight * static_cast<double>(option.best);
            // Maximum regret/weighted score; prefer cheaper insertion on ties.
            if (score > best_score ||
                (score == best_score && option.best < tie_best_cost)) {
                best_score = score;
                tie_best_cost = option.best;
                best_node = candidate;
                best_position = option.position;
            }
        }
        tour.insert(tour.begin() + best_position, best_node);
        used[best_node] = true;
    }
    return tour;
}

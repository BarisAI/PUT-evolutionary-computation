#include "heuristics.hpp"
#include <iostream>

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

Cost open_value(const Problem& p, const Tour& t) {
    Cost value = 0;
    for (int node : t) value += p.cost[node];
    for (std::size_t i = 1; i < t.size(); ++i) value += p.distance[t[i - 1]][t[i]];
    return value;
}

Cost closed_value(const Problem& p, const Tour& t) {
    return open_value(p, t) + p.distance[t.back()][t.front()];
}

int main() {
    try {
        const Problem p = make_problem({{0,0,20}, {3,4,2}, {1,1,9}, {10,0,0}, {2,3,4}});
        require(p.selected_count() == 3, "Odd size must round upwards");
        require(p.distance[0][1] == 5 && p.distance[0][2] == 1 && p.distance[0][4] == 4, "Distance rounding");
        require(p.distance[1][0] == 5 && p.distance[1][1] == 0, "Symmetry/diagonal");
        require(evaluate(p, {0,1,2}) == 41, "Closing edge and node costs");
        for (Tour t : {Tour{0}, Tour{0,1}, Tour{0,1,2}}) {
            for (int v = 0; v < p.size(); ++v) {
                if (std::find(t.begin(), t.end(), v) != t.end()) continue;
                for (int pos = 0; pos <= static_cast<int>(t.size()); ++pos) {
                    Tour inserted = t; inserted.insert(inserted.begin() + pos, v);
                    require(insertion_delta(p, t, v, pos, false) == open_value(p, inserted) - open_value(p,t), "Open insertion delta");
                    require(insertion_delta(p, t, v, pos, true) == closed_value(p, inserted) - closed_value(p,t), "Closed insertion delta");
                }
            }
        }
        // Matrix-only instance: no coordinates are needed by any heuristic.
        const Problem matrix{{{0,1,5,2}, {1,0,3,4}, {5,3,0,1}, {2,4,1,0}}, {0,100,0,1}};
        require(greedy_solution(matrix,0,Method::NearestEnd) == Tour({0,3}), "End NN must account for node cost");
        for (Method method : {Method::NearestEnd, Method::NearestAllPositions, Method::GreedyCycle}) {
            for (int start = 0; start < p.size(); ++start) {
                const auto t = greedy_solution(p,start,method);
                evaluate(p,t);
                require(std::find(t.begin(),t.end(),start) != t.end(), "Starting node is retained");
            }
            for (int n : {1,2,3,4}) {
                Problem small;
                small.cost.assign(n, 1); small.distance.assign(n, std::vector<Cost>(n, 0));
                evaluate(small, greedy_solution(small,0,method));
            }
        }
        std::mt19937 rng(42), again(42);
        for (int i = 0; i < 200; ++i) {
            const auto a = random_solution(p,rng), b = random_solution(p,again);
            evaluate(p,a); require(a == b, "Seed reproducibility");
        }
        bool rejected = false;
        try { evaluate(p, {0,0,1}); } catch (const std::runtime_error&) { rejected = true; }
        require(rejected, "Reject duplicate nodes");
        rejected = false;
        try { evaluate(p, {0,1,5}); } catch (const std::runtime_error&) { rejected = true; }
        require(rejected, "Reject out-of-range indices");
        rejected = false;
        try { evaluate(p, {0,1}); } catch (const std::runtime_error&) { rejected = true; }
        require(rejected, "Reject incorrect tour size");
        std::cout << "PASS: rounding, closed objective, insertion deltas, node costs, matrix-only input, odd/even sizes, seeds, validation.\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}

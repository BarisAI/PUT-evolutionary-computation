#include "regret.hpp"
#include <iostream>
#include <algorithm>

void check(bool yes, const char* msg) { if (!yes) throw std::runtime_error(msg); }

Cost open_score(const Problem& p, const Tour& t) {
    Cost z = 0;
    for (int v : t) z += p.cost[v];
    for (std::size_t i = 1; i < t.size(); ++i) z += p.distance[t[i - 1]][t[i]];
    return z;
}
Cost cycle_score(const Problem& p, const Tour& t) {
    return open_score(p, t) + p.distance[t.back()][t.front()];
}
int main() {
    try {
        const auto p = make_problem({{0,0,12},{3,4,1},{5,0,2},{1,2,8},
                                     {9,6,3},{2,9,0},{5,8,10}});
        check(p.selected_count() == 4, "Odd node counts");
        for (Basis basis : {Basis::NearestAllPositions, Basis::GreedyCycle}) {
            for (Tour partial : {Tour{0}, Tour{0,1}, Tour{0,1,2}}) {
                const int count = basis == Basis::GreedyCycle ?
                    static_cast<int>(partial.size()) : static_cast<int>(partial.size()) + 1;
                for (int v = 0; v < p.size(); ++v) {
                    if (std::find(partial.begin(), partial.end(), v) != partial.end()) continue;
                    auto option = insertion_choices(p, partial, v, basis);
                    std::vector<std::pair<Cost,int>> values;
                    for (int pos = 0; pos < count; ++pos) {
                        auto t = partial; t.insert(t.begin() + pos, v);
                        Cost delta = basis == Basis::GreedyCycle ?
                            cycle_score(p,t) - cycle_score(p,partial) :
                            open_score(p,t) - open_score(p,partial);
                        values.emplace_back(delta,pos);
                    }
                    std::stable_sort(values.begin(), values.end());
                    check(option.best == values[0].first, "Minimum insertion delta");
                    check(option.second == values[std::min(1,count-1)].first, "Second delta");
                    check(option.regret == option.second - option.best, "Regret");
                }
            }
            for (Criterion method : {Criterion::TwoRegret, Criterion::WeightedTwoRegret}) {
                for (int start = 0; start < p.size(); ++start) {
                    auto t = regret_solution(p,start,basis,method);
                    evaluate(p,t);
                    check(std::find(t.begin(),t.end(),start)!=t.end(), "Start retained");
                }
                for (int n : {1,2,3,4}) {
                    Problem q; q.cost.assign(n,1);
                    q.distance.assign(n,std::vector<Cost>(n,0));
                    evaluate(q,regret_solution(q,0,basis,method));
                }
            }
        }
        bool rejected = false;
        try { regret_solution(p,0,Basis::GreedyCycle,Criterion::WeightedTwoRegret,-1,1); }
        catch(const std::runtime_error&) { rejected = true; }
        check(rejected,"Negative weight rejection");
        check(regret_solution(p,0,Basis::GreedyCycle,Criterion::WeightedTwoRegret,0.5,0.5) ==
              regret_solution(p,0,Basis::GreedyCycle,Criterion::WeightedTwoRegret,1.0,1.0),
              "Equal-weight scaling");
        std::cout << "PASS: regret insertion deltas, unique tours, weights, odd/even sizes.\n";
        return 0;
    } catch(const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}

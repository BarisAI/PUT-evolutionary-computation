#pragma once
#include "../../Assignment 1/src/heuristics.hpp"
#include "../../Assignment 3/src/local_search.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numeric>
#include <random>
#include <stdexcept>
#include <vector>

namespace a6 {
using Clock = std::chrono::steady_clock;
constexpr int MSLS_RESTARTS = 400;
constexpr int EXPERIMENT_REPEATS = 20;
constexpr int KICK_REPLACEMENTS = 4;
constexpr double UPHILL_ACCEPT_CHANCE = 0.12;

inline double elapsed_ms(const Clock::time_point& start) {
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

struct RunResult {
    Tour best_tour;
    Cost best_objective = std::numeric_limits<Cost>::max();
    Cost initial_local_objective = -1;
    double elapsed = 0.;
    int local_search_runs = 0;
    int kicks = 0;
    int accepted_kicks = 0;
    int uphill_accepted = 0;
    std::uint64_t delta_checks = 0;
};

struct ProgressPoint { int ls_runs; double elapsed; Cost best; };

inline SearchStats steepest(const Problem& p, Tour initial, std::mt19937& rng) {
    // The exact unrestricted Assignment 3 search: 2-opt + selected/unselected replacement.
    return local_search(p, std::move(initial), LocalStrategy::Steepest, IntraMove::TwoOpt, rng);
}

// A nontrivial, reproducible perturbation: swap exactly 4 selected vertices with
// 4 distinct initially-unselected vertices, then make one double-bridge change
// of order A|B|C|D -> A|C|B|D. The costs/order are intentionally NOT optimized
// during perturbation; the following Steepest LS repairs the solution.
inline Tour perturb(const Problem& p, const Tour& base, std::mt19937& rng) {
    Tour t = base;
    const int m = static_cast<int>(t.size());
    std::vector<int> outside = outside_nodes(p,t);
    const int q = std::min({KICK_REPLACEMENTS, m, static_cast<int>(outside.size())});
    // Draw positions and outside nodes without replacement via partial Fisher-Yates.
    std::vector<int> positions(m);
    std::iota(positions.begin(), positions.end(), 0);
    for (int i=0; i<q; ++i) {
        std::uniform_int_distribution<int> d1(i,m-1), d2(i,static_cast<int>(outside.size())-1);
        std::swap(positions[i],positions[d1(rng)]);
        std::swap(outside[i],outside[d2(rng)]);
        t[positions[i]] = outside[i];
    }
    if (m >= 20) {
        std::uniform_int_distribution<int> d1(5, m-15);
        const int a = d1(rng);
        std::uniform_int_distribution<int> d2(a+5,m-10);
        const int b = d2(rng);
        std::uniform_int_distribution<int> d3(b+5,m-5);
        const int c = d3(rng);
        Tour changed; changed.reserve(m);
        changed.insert(changed.end(),t.begin(),t.begin()+a);
        changed.insert(changed.end(),t.begin()+b,t.begin()+c);
        changed.insert(changed.end(),t.begin()+a,t.begin()+b);
        changed.insert(changed.end(),t.begin()+c,t.end());
        t.swap(changed);
    } else if (m >= 4) {
        // Small-instance fallback, used by edge-case tests only.
        std::uniform_int_distribution<int> d(0,m-1);
        int a=d(rng),b=d(rng);
        if (a>b)std::swap(a,b);
        if(a!=b)std::reverse(t.begin()+a,t.begin()+b+1);
    }
    return t;
}

inline RunResult msls(const Problem& p, std::uint32_t seed,
                      std::vector<ProgressPoint>* progress=nullptr, int restarts=MSLS_RESTARTS) {
    if(restarts<=0)throw std::runtime_error("MSLS restarts must be >0");
    std::mt19937 rng(seed);
    RunResult result;
    const auto start=Clock::now();
    for (int i=0; i<restarts; ++i) {
        Tour initial = random_solution(p,rng);
        auto local = steepest(p,std::move(initial),rng);
        result.delta_checks += local.delta_checks;
        if(i==0)result.initial_local_objective=local.objective;
        ++result.local_search_runs;
        if(local.objective < result.best_objective) {
            result.best_objective=local.objective;
            result.best_tour=std::move(local.tour);
            if(progress)progress->push_back({result.local_search_runs,elapsed_ms(start),result.best_objective});
        }
    }
    result.elapsed=elapsed_ms(start);
    return result;
}

inline RunResult ils(const Problem& p,std::uint32_t seed,double budget_ms,
                     std::vector<ProgressPoint>* progress=nullptr) {
    if(budget_ms<=0)throw std::runtime_error("ILS budget must be positive");
    std::mt19937 rng(seed);
    RunResult result;
    const auto start=Clock::now();
    auto first=steepest(p,random_solution(p,rng),rng);
    result.best_tour=first.tour;
    result.best_objective=result.initial_local_objective=first.objective;
    result.delta_checks=first.delta_checks;
    result.local_search_runs=1;
    if(progress)progress->push_back({1,elapsed_ms(start),result.best_objective});
    Tour current=std::move(first.tour);
    Cost current_obj=first.objective;
    std::uniform_real_distribution<double> coin(0.,1.);
    int unsuccessful=0;
    // Check the clock *before* every next LS; last LS can overshoot budget.
    while(elapsed_ms(start) < budget_ms) {
        Tour kicked=perturb(p,current,rng);
        const auto attempt=steepest(p,std::move(kicked),rng);
        ++result.local_search_runs;
        ++result.kicks;
        result.delta_checks+=attempt.delta_checks;
        if(attempt.objective < result.best_objective) {
            result.best_objective=attempt.objective;
            result.best_tour=attempt.tour;
            unsuccessful=0;
            if(progress)progress->push_back({result.local_search_runs,elapsed_ms(start),result.best_objective});
        } else ++unsuccessful;
        // Improve-or-probabilistic-worse acceptance to leave a local-optimum basin.
        bool uphill=(attempt.objective > current_obj);
        if(attempt.objective <= current_obj || coin(rng)<UPHILL_ACCEPT_CHANCE) {
            if(uphill)++result.uphill_accepted;
            current=attempt.tour;
            current_obj=attempt.objective;
            ++result.accepted_kicks;
        }
        // Recover the best-known basin after prolonged non-improvement.
        if(unsuccessful>=80) {
            current=result.best_tour;
            current_obj=result.best_objective;
            unsuccessful=0;
        }
    }
    result.elapsed=elapsed_ms(start);
    return result;
}
}

#include "local_search.hpp"
#include <iostream>
#include <stdexcept>
#include <random>
#include <set>

void check(bool yes, const std::string& message) {
    if (!yes) throw std::runtime_error(message);
}

// Full re-evaluation is used ONLY in unit tests to independently verify delta formulas.
void verify_all_deltas(const Problem& p, const Tour& tour) {
    const Cost original = evaluate(p,tour);
    const auto outside = outside_nodes(p,tour);
    const int m = static_cast<int>(tour.size());
    for (IntraMove intra : {IntraMove::NodeSwap, IntraMove::TwoOpt}) {
        auto moves = make_moves(m, static_cast<int>(outside.size()), intra);
        int n_intra=0, n_inter=0;
        for (const auto& move : moves) {
            Tour copy=tour;
            const Cost delta=move_delta(p,tour,outside,intra,move);
            apply_move(copy,outside,intra,move);
            const Cost actual=evaluate(p,copy)-original;
            check(delta==actual,"Mismatch delta vs full objective reevaluation");
            if(move.type==MoveType::Inter)++n_inter;else ++n_intra;
        }
        check(n_inter == m * static_cast<int>(outside.size()),"Inter-route enumeration");
        check(n_intra==(intra==IntraMove::NodeSwap ? m*(m-1)/2 : m*(m-3)/2),"Intra-route enumeration");
    }
}

int main() {
 try {
    const auto p=make_problem({{0,0,12},{3,4,1},{5,0,2},{1,2,8},{9,6,3},
                               {2,9,0},{5,8,10},{10,10,8},{4,9,6},{7,2,3}});
    std::mt19937 rng(71);
    Tour test={0,1,2,3,4};
    verify_all_deltas(p,test);
    for(int k=0;k<50;++k){
       auto t=test;
       std::shuffle(t.begin(),t.end(),rng);
       verify_all_deltas(p,t);
    }
    for (LocalStrategy strategy : {LocalStrategy::Greedy,LocalStrategy::Steepest}) {
      for (IntraMove intra : {IntraMove::NodeSwap,IntraMove::TwoOpt}) {
        for(int start=0;start<10;++start){
          Tour t(10);std::iota(t.begin(),t.end(),0);
          std::shuffle(t.begin(),t.end(),rng);t.resize(5);
          const Cost oldval=evaluate(p,t);
          std::mt19937 srng(100+start);
          auto sol=local_search(p,t,strategy,intra,srng);
          check(sol.objective==evaluate(p,sol.tour),"Objective matches accumulated deltas");
          check(sol.objective<=oldval,"Search may not worsen objective");
          const auto outside=outside_nodes(p,sol.tour);
          for(const auto& move:make_moves(5,5,intra))
            check(move_delta(p,sol.tour,outside,intra,move)>=0,"Not a complete local optimum");
        }
      }
    }
    // Two random orders on an identical case should remain valid; every search
    // inspects both movement classes in one shuffled list, with no delta cache.
    std::mt19937 seed(42),same(42);
    const auto a=local_search(p,test,LocalStrategy::Greedy,IntraMove::NodeSwap,seed);
    const auto b=local_search(p,test,LocalStrategy::Greedy,IntraMove::NodeSwap,same);
    check(a.tour==b.tour&&a.delta_checks==b.delta_checks,"Reproducible random exploration");
    std::cout<<"PASS: exact move deltas, adjacent/wrap swaps, 2-opt, inter-route costs,"
             <<" all local optima, randomized first improvement, repeatability.\n";
    return 0;
 } catch (const std::exception& e){ std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}

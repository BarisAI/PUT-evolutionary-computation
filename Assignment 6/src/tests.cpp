#include "msls_ils.hpp"
#include <iostream>
#include <stdexcept>
#include <unordered_set>
void check(bool b,const char*why){if(!b)throw std::runtime_error(why);}
Problem test_problem(int n) {
    std::vector<Node> nodes;
    for(int i=0;i<n;++i)nodes.push_back({i*7%31,i*i*3%29,Cost(i*19%23)});
    return make_problem(nodes);
}
void verify_optimum(const Problem&p,const Tour&t) {
    const auto outside=outside_nodes(p,t);
    const auto moves=make_moves(static_cast<int>(t.size()),static_cast<int>(outside.size()),IntraMove::TwoOpt);
    for(auto& move:moves)check(move_delta(p,t,outside,IntraMove::TwoOpt,move)>=0,"Negative delta at local optimum");
}
int main() {
    try {
        for(int n:{4,7,19,32,50}){
            const Problem p=test_problem(n);
            std::mt19937 rng(312312+n);
            for(int run=0;run<10;++run){
                Tour start=random_solution(p,rng);
                auto local=a6::steepest(p,start,rng);
                check(local.objective==evaluate(p,local.tour),"Steepest objective mismatch");
                check(local.objective<=evaluate(p,start),"Steepest worsened solution");
                verify_optimum(p,local.tour);
                for(int i=0;i<12;++i){
                    Tour kick=a6::perturb(p,local.tour,rng);
                    check(static_cast<int>(kick.size())==p.selected_count(),"Perturb length");
                    check(evaluate(p,kick)>=0,"Perturb invalid unique indices");
                    std::unordered_set<int>a(local.tour.begin(),local.tour.end()),b(kick.begin(),kick.end());
                    int changed=0;for(int x:a)changed+=!b.count(x);
                    check(changed==std::min({a6::KICK_REPLACEMENTS,p.selected_count(),p.size()-p.selected_count()}),
                          "Perturb must replace four distinct selected vertices");
                }
                const auto original=local.tour;
                check(evaluate(p,original)==local.objective,"Tour after perturb changed original");
            }
        }
        const auto p=test_problem(40);
        std::vector<a6::ProgressPoint> pm,pi;
        auto ms=a6::msls(p,12345,&pm,8);
        check(ms.local_search_runs==8 && !pm.empty(),"MSLS 8 starts not reached");
        check(ms.best_objective==evaluate(p,ms.best_tour),"MSLS final invalid");
        auto il=a6::ils(p,54321,12.0,&pi);
        check(il.local_search_runs>=1&&il.kicks==il.local_search_runs-1,"ILS kick count");
        check(il.best_objective==evaluate(p,il.best_tour),"ILS final invalid");
        check(il.best_objective<=il.initial_local_objective,"ILS forgot starting best");
        for(const auto*progress:{&pm,&pi})for(std::size_t k=1;k<progress->size();++k){
            check((*progress)[k].best<(*progress)[k-1].best,"Best-so-far not strictly improving");
            check((*progress)[k].ls_runs>(*progress)[k-1].ls_runs,"LS count not increasing");
        }
        std::cout<<"PASS: deltas, local optima, perturbation distinctness, MSLS=8 runs, ILS budget and best-so-far.\n";
        return 0;
    } catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}

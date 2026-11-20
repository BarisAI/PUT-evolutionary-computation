#include "lns.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>

// One independent experiment per process, suitable for running sequentially
// on a laptop or using a controlled process pool. CLI:
// assignment7 <data_dir> <TSPA|TSPB> <method:0..5> <trial:0..19> <budget_ms>
int main(int argc,char** argv){
  try {
    if(argc!=6)throw std::runtime_error("Usage: assignment7 data_dir TSPA|TSPB method(0..5) trial(0..19) budget_ms");
    const std::string instance=argv[2];
    const int id=instance=="TSPA"?0:instance=="TSPB"?1:-1;
    if(id<0)throw std::runtime_error("Instance must be TSPA or TSPB");
    const int method=std::stoi(argv[3]),trial=std::stoi(argv[4]);
    const double budget=std::stod(argv[5]);
    if(method<0||method>=6||trial<0||trial>=20)throw std::runtime_error("Invalid method or trial index");
    const auto p=make_problem(read_nodes(std::string(argv[1])+"/"+instance+".csv"));
    if(p.size()!=200||p.selected_count()!=100)throw std::runtime_error("Wrong instance size");
    const auto strategy=static_cast<a7::Destroy>(method/2);
    const bool after_ls=method%2==1;
    const Basis basis=id==0?Basis::GreedyCycle:Basis::NearestAllPositions;
    // Same RNG seed for the paired with/without-LS comparison of each destroy variant.
    // Distinct trials, instances and destroy methods use non-overlapping seeds.
    const std::uint32_t seed=static_cast<std::uint32_t>(2026700+id*100000+trial*101+(method/2)*11003+17);
    const auto res=a7::search(p,strategy,after_ls,basis,seed,budget);
    if(evaluate(p,res.best_tour)!=res.best_objective)throw std::runtime_error("Incorrect final objective");
    std::cout<<instance<<','<<a7::name(strategy)<<','<<(after_ls?"with_ls":"without_ls")<<','
             <<trial<<','<<seed<<','<<std::fixed<<std::setprecision(6)<<budget<<','
             <<res.elapsed_ms<<','<<res.initial_objective<<','<<res.best_objective<<','
             <<res.iterations<<','<<res.accepted<<','<<res.rejected<<','<<res.local_search_calls<<','
             <<res.delta_evaluations<<','<<(basis==Basis::GreedyCycle?"weighted_cycle":"weighted_open_path")<<',';
    for(std::size_t i=0;i<res.best_tour.size();++i){if(i)std::cout<<' ';std::cout<<res.best_tour[i];}
    std::cout<<'\n';
    return 0;
  }catch(const std::exception&e){std::cerr<<"ERROR: "<<e.what()<<'\n';return 1;}
}

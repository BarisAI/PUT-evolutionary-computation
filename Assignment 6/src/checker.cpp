#include "../../Assignment 1/src/problem.hpp"
#include "../../Assignment 3/src/local_search.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>
namespace fs=std::filesystem;
int main(int argc,char** argv) {
  try {
    if(argc<3)throw std::runtime_error("Usage: checker DATA_DIR BEST_SOLUTIONS_TXT");
    const fs::path datadir=argv[1];
    std::ifstream best(argv[2]);
    if(!best)throw std::runtime_error("Cannot read best solutions");
    std::string line;int count=0;
    while(std::getline(best,line)) {
      if(line.empty()||line[0]=='#')continue;
      std::istringstream s(line);std::string instance,method;Cost expected;
      if(!(s>>instance>>method>>expected))throw std::runtime_error("Malformed best solution line");
      auto nodes=read_nodes((datadir/(instance+".csv")).string());
      int n=static_cast<int>(nodes.size()),k=(n+1)/2;
      Tour tour;int v;while(s>>v)tour.push_back(v);
      if(static_cast<int>(tour.size())!=k)throw std::runtime_error("Incorrect number of tour nodes");
      std::vector<bool> seen(n,false);Cost travel=0,vertex=0;
      for(int i=0;i<k;++i) {
        int a=tour[i],b=tour[(i+1)%k];
        if(a<0||a>=n||b<0||b>=n||seen[a])throw std::runtime_error("Duplicate or invalid index");
        seen[a]=true;vertex+=nodes[a].cost;
        const double dx=static_cast<double>(nodes[a].x)-nodes[b].x;
        const double dy=static_cast<double>(nodes[a].y)-nodes[b].y;
        travel+=static_cast<Cost>(std::floor(std::hypot(dx,dy)+0.5));
      }
      if(travel+vertex!=expected)throw std::runtime_error("Independent total does not match reported objective");
      // Check unrestricted neighbourhood local-optimality, too (14,850 moves for 100+100).
      Problem p=make_problem(nodes);
      const auto outside=outside_nodes(p,tour);
      const auto moves=make_moves(k,n-k,IntraMove::TwoOpt);
      for(const auto& move:moves) {
          if(move_delta(p,tour,outside,IntraMove::TwoOpt,move)<0)
              throw std::runtime_error("Reported best tour is not a full local optimum");
      }
      std::cout<<"PASS "<<instance<<' '<<method<<" nodes="<<k<<" travel="<<travel<<" vertex_cost="<<vertex<<" objective="<<expected<<" full_local_optimum=PASS"<<'\n';
      ++count;
    }
    if(count!=4)throw std::runtime_error("Expected exactly four best solutions");
    std::cout<<"PASS: all 4 independent tours checked.\n";
    return 0;
  }catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}

// Independent coordinate-based checker; does not include algorithm headers.
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

struct Point { long long x, y, cost; };
int main(int argc, char** argv) {
 try {
    if (argc != 3) throw std::runtime_error("Usage: checker DATA_DIRECTORY BEST_SOLUTIONS_FILE");
    std::ifstream solutions(argv[2]);
    if (!solutions) throw std::runtime_error("Cannot open solutions file");
    std::string line;int checked=0;
    std::set<std::string> records;
    const std::set<std::string> allowed{
     "steepest_nodes_random", "steepest_edges_random",
     "steepest_nodes_greedy", "steepest_edges_greedy",
     "greedy_nodes_random", "greedy_edges_random",
     "greedy_nodes_greedy", "greedy_edges_greedy"};
    while(std::getline(solutions,line)) {
       if(line.empty()||line[0]=='#')continue;
       std::istringstream input(line);
       std::string instance,method;long long claimed;
       if(!(input>>instance>>method>>claimed))throw std::runtime_error("Malformed solution header");
       if((instance!="TSPA"&&instance!="TSPB")||!allowed.count(method)||
          !records.insert(instance+method).second)throw std::runtime_error("Unexpected/duplicate record");
       std::ifstream file(std::filesystem::path(argv[1])/(instance+".csv"));
       if(!file)throw std::runtime_error("Cannot open instance");
       std::vector<Point> points;std::string row;
       while(std::getline(file,row)) {
          if(row.empty())continue;
          std::istringstream v(row);Point p{};char a,b;std::string extra;
          if(!(v>>p.x>>a>>p.y>>b>>p.cost)||a!=';'||b!=';'||(v>>extra))throw std::runtime_error("Bad row");
          points.push_back(p);
       }
       std::vector<int> tour;int v;
       while(input>>v)tour.push_back(v);
       if(!input.eof()||tour.size()!=(points.size()+1)/2)throw std::runtime_error("Wrong tour length");
       std::set<int> uniq;
       for(int i:tour)if(i<0||i>=static_cast<int>(points.size())||!uniq.insert(i).second)
          throw std::runtime_error("Invalid/duplicate index");
       long long distance=0,cost=0;
       for(std::size_t i=0;i<tour.size();i++) {
          const auto& a=points[tour[i]],&b=points[tour[(i+1)%tour.size()]];
          const long double dx=static_cast<long double>(a.x)-b.x,dy=static_cast<long double>(a.y)-b.y;
          distance+=static_cast<long long>(std::floor(std::sqrt(dx*dx+dy*dy)+0.5L));
          cost+=a.cost;
       }
       if(distance+cost!=claimed)throw std::runtime_error("Objective mismatch");
       std::cout<<"PASS "<<instance<<' '<<method<<" nodes="<<tour.size()<<" distance="<<distance
                <<" node_cost="<<cost<<" total="<<claimed<<'\n';++checked;
    }
    if(checked!=16)throw std::runtime_error("Expected 16 best tours");
    std::cout<<"All 16 best solutions independently verified.\n";
    return 0;
 }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}

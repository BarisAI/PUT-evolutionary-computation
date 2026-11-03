#include "lm_search.hpp"
#include "../../Assignment 4/src/candidate_search.hpp"
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <string>

namespace fs=std::filesystem;
using Clock=std::chrono::steady_clock;
inline double ms_since(Clock::time_point t){return std::chrono::duration<double,std::milli>(Clock::now()-t).count();}
struct Group {
 std::string name,method;
 int count=0, min_run=-1;
 Cost minimum=std::numeric_limits<Cost>::max(), maximum=std::numeric_limits<Cost>::min();
 long double sum=0,time=0,deltas=0,accepted=0,discarded=0,deferred=0,refreshed=0;
 Tour best;
 void add(int run,const Tour&t,Cost cost,double ms,std::uint64_t d,int a,
          std::uint64_t disc=0,std::uint64_t def=0,std::uint64_t ref=0){
   count++;sum+=cost;time+=ms;deltas+=d;accepted+=a;discarded+=disc;deferred+=def;refreshed+=ref;
   if(cost<minimum){minimum=cost;min_run=run;best=t;}
   maximum=std::max(maximum,cost);
 }
};
void save_svg(const std::vector<Node>&nodes,const Tour&tour,const fs::path&file,
              const std::string&label,Cost objective){
 std::ofstream out(file);if(!out)throw std::runtime_error("Cannot write SVG");
 int xmin=nodes.front().x,xmax=xmin,ymin=nodes.front().y,ymax=ymin;
 Cost cmin=nodes.front().cost,cmax=cmin;
 for(auto v:nodes){xmin=std::min(xmin,v.x);xmax=std::max(xmax,v.x);
  ymin=std::min(ymin,v.y);ymax=std::max(ymax,v.y);cmin=std::min(cmin,v.cost);cmax=std::max(cmax,v.cost);}
 double sc=580.0/std::max({1,xmax-xmin,ymax-ymin});
 auto x=[&](int i){return 70.0+(nodes[i].x-xmin)*sc;};
 auto y=[&](int i){return 670.0-(nodes[i].y-ymin)*sc;};
 auto color=[&](Cost c){double t=cmax==cmin?0.0:double(c-cmin)/double(cmax-cmin);
  return "rgb("+std::to_string(int(40+200*t))+","+std::to_string(int(130-80*t))+","+std::to_string(int(220-180*t))+")";};
 out<<std::fixed<<std::setprecision(2);
 out<<"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"860\" height=\"780\" viewBox=\"0 0 860 780\">\n"
    <<"<rect width=\"860\" height=\"780\" fill=\"white\"/>\n"
    <<"<g font-family=\"sans-serif\" fill=\"#17222d\"><text x=\"60\" y=\"35\" font-size=\"20\">"
    <<label<<"</text><text x=\"60\" y=\"60\" font-size=\"15\">Objective: "<<objective
    <<" | selected: "<<tour.size()<<" / "<<nodes.size()<<"</text></g>\n"
    <<"<path d=\"M 70 90 V 670 H 670\" fill=\"none\" stroke=\"#aab4be\"/>\n"
    <<"<polyline fill=\"none\" stroke=\"#334155\" stroke-width=\"1.5\" points=\"";
 for(int node:tour)out<<x(node)<<','<<y(node)<<' ';
 out<<x(tour.front())<<','<<y(tour.front())<<"\"/>\n";
 std::vector<bool> selected(nodes.size());for(int v:tour)selected[v]=true;
 for(int layer=0;layer<2;layer++)for(int i=0;i<int(nodes.size());i++){
  if(selected[i]!=bool(layer))continue;
  out<<"<circle cx=\""<<x(i)<<"\" cy=\""<<y(i)<<"\" r=\""
     <<(selected[i]?4.5:3.0)<<"\" fill=\""<<color(nodes[i].cost)
     <<"\" opacity=\""<<(selected[i]?1.0:0.45)<<"\" stroke=\""
     <<(selected[i]?"#0f172a":"none")<<"\"><title>Node "<<i<<"; cost "<<nodes[i].cost
     <<(selected[i]?"; selected":"; not selected")<<"</title></circle>\n";
 }
 out<<"<g font-family=\"sans-serif\" font-size=\"13\" fill=\"#334155\"><text x=\"702\" y=\"125\">Node cost</text>\n";
 for(int i=0;i<100;i++)out<<"<rect x=\"705\" y=\""<<150+i*3<<"\" width=\"20\" height=\"3.1\" fill=\""
   <<color(cmin+(cmax-cmin)*i/99)<<"\"/>\n";
 out<<"<text x=\"735\" y=\"160\">"<<cmin<<"</text><text x=\"735\" y=\"450\">"<<cmax
    <<"</text><text x=\"700\" y=\"500\">Outlined: selected</text><text x=\"700\" y=\"525\">Faded: unselected</text>"
    <<"<text x=\"55\" y=\"755\">Closed tour; node indices start at zero.</text></g></svg>\n";
}
int main(int argc,char**argv){
 try{
  const fs::path data=argc>1?argv[1]:"../Assignment 1/data";
  const fs::path output=argc>2?argv[2]:"results";
  const int count=argc>3?std::stoi(argv[3]):200;
  if(count<1||count>200)throw std::runtime_error("Run limit must be 1..200");
  fs::create_directories(output/"plots");
  std::ofstream detail(output/"runs.csv"),summary(output/"summary.csv"),best(output/"best_solutions.txt"),info(output/"run_info.txt");
  if(!detail||!summary||!best||!info)throw std::runtime_error("Cannot open output files");
  detail<<"instance,method,run,initial_objective,final_objective,accepted_moves,two_opt_moves,replacement_moves,delta_evaluations,discarded,deferred,refreshes,cache_hits,local_ms\n";
  summary<<"instance,method,runs,min,max,mean_objective,mean_local_ms,mean_delta_evaluations,mean_accepted_moves,mean_discarded,mean_deferred,mean_refreshes\n";
  best<<"# instance method objective then 100 zero-based node indices (without repeated closure)\n";
  const std::array<std::string,2> names{"TSPA","TSPB"};
  std::mt19937 rng(42);
  for(const auto &name:names){
   const auto nodes=read_nodes((data/(name+".csv")).string());
   const Problem p=make_problem(nodes);
   std::vector<Tour> starts; starts.reserve(count);
   for(int i=0;i<count;i++)starts.push_back(random_solution(p,rng));
   for(int m=0;m<2;m++){
    Group group;group.name=name;group.method=m==0?"full_steepest":"lm_steepest";
    for(int run=0;run<count;run++){
     const Cost initial=evaluate(p,starts[run]);
     Cost value;Tour tour;
     int accepted, twos, replaces;
     std::uint64_t d,disc=0,def=0,ref=0;
     const auto before=Clock::now();
     if(m==0){
       auto sol=steepest_search(p,starts[run],SearchMode::Full);
       value=sol.objective;tour=std::move(sol.tour);accepted=sol.accepted;
       twos=-1;replaces=-1;d=sol.evaluations;
     }else{
       auto sol=lm::local_search(p,starts[run]);
       value=sol.objective;tour=std::move(sol.tour);accepted=sol.accepted;
       twos=sol.applied_two_opt;replaces=sol.applied_replace;
       d=sol.evaluated_deltas;disc=sol.discarded;def=sol.deferred;ref=sol.refreshed;
     }
     const double elapsed=ms_since(before);
     if(value!=evaluate(p,tour)||value>initial)throw std::runtime_error("Invalid final tour or objective");
     group.add(run,tour,value,elapsed,d,accepted,disc,def,ref);
     detail<<name<<','<<group.method<<','<<run<<','<<initial<<','<<value<<','<<accepted<<','<<twos<<','<<replaces<<','<<d
           <<','<<disc<<','<<def<<','<<ref<<','<<std::fixed<<std::setprecision(6)<<elapsed<<'\n';
    }
    const long double n=group.count;
    summary<<name<<','<<group.method<<','<<group.count<<','<<group.minimum<<','<<group.maximum<<','
           <<std::fixed<<std::setprecision(3)<<double(group.sum/n)<<','<<std::setprecision(6)<<double(group.time/n)
           <<','<<double(group.deltas/n)<<','<<double(group.accepted/n)<<','<<double(group.discarded/n)
           <<','<<double(group.deferred/n)<<','<<double(group.refreshed/n)<<'\n';
    best<<name<<' '<<group.method<<' '<<group.minimum;
    for(int v:group.best)best<<' '<<v;
    best<<'\n';
    save_svg(nodes,group.best,output/"plots"/(name+"_"+group.method+".svg"),name+" / "+group.method,group.minimum);
    std::cout<<name<<' '<<group.method<<" min="<<group.minimum<<" max="<<group.maximum
             <<" mean="<<double(group.sum/n)<<" search_ms="<<double(group.time/n)
             <<" delta_evaluations="<<double(group.deltas/n)<<'\n'<<std::flush;
   }
  }
  info<<"Random initial solutions generated with the Assignment 1 Fisher-Yates method; std::mt19937 seed 42, TSPA then TSPB.\n"
      <<"Both full and LM use exactly the same 200 starts per instance and the same 2-opt plus replacement neighborhood.\n"
      <<"LM saves negative deltas using unordered removed-edge IDs + relative reconnection and old/new/neighbor IDs for replacements.\n"
      <<"Two-edge moves with one inverted relative edge are retained as deferred, removed-edge-missing entries are discarded.\n"
      <<"After a move, only newly possible deltas are evaluated (new tour edges, affected node neighbor pairs, new outside node).\n"
      <<"On exhaustion, a full move evaluation confirms a complete local optimum. Candidate moves are NOT used.\n"
      <<"Times: std::chrono::steady_clock local-search-only milliseconds; exclude common random start generation and file output.\n";
  return 0;
 }catch(const std::exception &e){std::cerr<<"ERROR: "<<e.what()<<'\n';return 1;}
}

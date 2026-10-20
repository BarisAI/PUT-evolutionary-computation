#include "local_search.hpp"
#include "../../Assignment 1/src/heuristics.hpp"
#include "../../Assignment 2/src/regret.hpp"
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <sstream>
#include <string>

namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;

double elapsed_ms(Clock::time_point before) {
    return std::chrono::duration<double, std::milli>(Clock::now() - before).count();
}
struct Constructed { Tour tour; double ms; };
struct MethodSummary { std::string instance, method, source; int count = 0;
    Cost min = std::numeric_limits<Cost>::max(), max = std::numeric_limits<Cost>::min();
    long double sum = 0; double total_ms = 0, local_ms = 0;
    int best_start = -1; Tour best_tour;
    Cost best_initial = 0; int best_moves = 0; std::uint64_t best_checks = 0;
    void add(int start, const Tour& t, Cost value, double runtime, double local = 0,
             Cost initial = 0, int moves = 0, std::uint64_t checks = 0) {
        ++count; sum += value; total_ms += runtime; local_ms += local;
        if (value < min) {
            min = value; best_start = start; best_tour = t;
            best_initial = initial; best_moves = moves; best_checks = checks;
        }
        max = std::max(max, value);
    }
    double avg() const { return static_cast<double>(sum / count); }
    double ms_per_run() const { return total_ms / count; }
    double local_ms_per_run() const { return local_ms / count; }
};

struct Variant { std::string name; LocalStrategy strategy; IntraMove intra; bool greedy_start; };
const std::array<Variant,8> variants{{
 {"steepest_nodes_random", LocalStrategy::Steepest, IntraMove::NodeSwap, false},
 {"steepest_edges_random", LocalStrategy::Steepest, IntraMove::TwoOpt, false},
 {"steepest_nodes_greedy", LocalStrategy::Steepest, IntraMove::NodeSwap, true},
 {"steepest_edges_greedy", LocalStrategy::Steepest, IntraMove::TwoOpt, true},
 {"greedy_nodes_random", LocalStrategy::Greedy, IntraMove::NodeSwap, false},
 {"greedy_edges_random", LocalStrategy::Greedy, IntraMove::TwoOpt, false},
 {"greedy_nodes_greedy", LocalStrategy::Greedy, IntraMove::NodeSwap, true},
 {"greedy_edges_greedy", LocalStrategy::Greedy, IntraMove::TwoOpt, true}
}};
const std::array<std::string,8> previous_names{{
 "random", "nearest_end", "nearest_all_positions", "greedy_cycle",
 "nn_2regret", "cycle_2regret", "nn_weighted_2regret", "cycle_weighted_2regret"
}};

// Coordinates are used for plotting only, never inside optimization algorithms.
void save_svg(const std::vector<Node>& nodes, const Tour& tour, const fs::path& destination,
              const std::string& title, Cost objective) {
    std::ofstream out(destination);
    if (!out) throw std::runtime_error("Cannot write SVG " + destination.string());
    int xmin = nodes[0].x, xmax = xmin, ymin = nodes[0].y, ymax = ymin;
    Cost cmin = nodes[0].cost, cmax = cmin;
    for (auto n : nodes) {
        xmin = std::min(xmin, n.x); xmax = std::max(xmax, n.x);
        ymin = std::min(ymin, n.y); ymax = std::max(ymax, n.y);
        cmin = std::min(cmin, n.cost); cmax = std::max(cmax, n.cost);
    }
    const double scale = 580.0 / std::max({1, xmax - xmin, ymax - ymin});
    auto x = [&](int i) { return 80.0 + (nodes[i].x - xmin) * scale; };
    auto y = [&](int i) { return 680.0 - (nodes[i].y - ymin) * scale; };
    auto color = [&](Cost cost) {
        const double t = cmax == cmin ? 0 : static_cast<double>(cost - cmin) / (cmax - cmin);
        return "rgb(" + std::to_string(static_cast<int>(40 + 200 * t)) + "," +
            std::to_string(static_cast<int>(130 - 80 * t)) + "," +
            std::to_string(static_cast<int>(220 - 180 * t)) + ")";
    };
    out << std::fixed << std::setprecision(2);
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"860\" height=\"780\" viewBox=\"0 0 860 780\">\n"
        << "<rect width=\"860\" height=\"780\" fill=\"white\"/>\n"
        << "<g font-family=\"sans-serif\" fill=\"#182335\"><text x=\"60\" y=\"35\" font-size=\"20\">"
        << title << "</text><text x=\"60\" y=\"63\" font-size=\"15\">Objective: " << objective
        << " | selected: " << tour.size() << '/' << nodes.size() << "</text></g>\n"
        << "<path d=\"M 80 95 V 680 H 675\" fill=\"none\" stroke=\"#9ca3af\"/>\n"
        << "<polyline fill=\"none\" stroke=\"#334155\" stroke-width=\"1.6\" points=\"";
    for (int node: tour) out << x(node) << ',' << y(node) << ' ';
    out << x(tour.front()) << ',' << y(tour.front()) << "\"/>\n";
    std::vector<bool> selected(nodes.size());
    for (int node : tour) selected[node] = true;
    for (int layer=0;layer<2;++layer)
        for (int i=0;i<static_cast<int>(nodes.size());++i) {
            if (selected[i] != static_cast<bool>(layer)) continue;
            out << "<circle cx=\"" << x(i) << "\" cy=\"" << y(i) << "\" r=\""
                << (selected[i] ? 4.5 : 3.0) << "\" fill=\"" << color(nodes[i].cost)
                << "\" opacity=\"" << (selected[i] ? 1.0 : 0.45) << "\" stroke=\""
                << (selected[i] ? "#0f172a" : "none") << "\"><title>Node " << i
                << "; cost " << nodes[i].cost << (selected[i] ? "; selected" : "; not selected")
                << "</title></circle>\n";
        }
    out << "<g font-family=\"sans-serif\" font-size=\"13\" fill=\"#334155\"><text x=\"705\" y=\"125\">Node cost</text>\n";
    for (int i=0;i<100;++i)
        out << "<rect x=\"710\" y=\"" << 150 + 3 * i
            << "\" width=\"22\" height=\"3.1\" fill=\""
            << color(cmin + (cmax - cmin) * i / 99) << "\"/>\n";
    out << "<text x=\"740\" y=\"160\">" << cmin << "</text><text x=\"740\" y=\"450\">" << cmax
        << "</text><text x=\"700\" y=\"500\">Outlined: selected</text>"
        << "<text x=\"700\" y=\"525\">Faded: unselected</text>"
        << "<text x=\"60\" y=\"758\">Closed tour; equal axis scale; indices start at zero.</text></g></svg>\n";
}

Tour previous_tour(const Problem& p, int method, int run, std::mt19937& random_rng) {
    switch(method) {
      case 0: return random_solution(p,random_rng);
      case 1: return greedy_solution(p,run,Method::NearestEnd);
      case 2: return greedy_solution(p,run,Method::NearestAllPositions);
      case 3: return greedy_solution(p,run,Method::GreedyCycle);
      case 4: return regret_solution(p,run,Basis::NearestAllPositions,Criterion::TwoRegret);
      case 5: return regret_solution(p,run,Basis::GreedyCycle,Criterion::TwoRegret);
      case 6: return regret_solution(p,run,Basis::NearestAllPositions,Criterion::WeightedTwoRegret);
      case 7: return regret_solution(p,run,Basis::GreedyCycle,Criterion::WeightedTwoRegret);
    }
    throw std::runtime_error("Unknown baseline method");
}

int main(int argc, char** argv) {
 try {
    const fs::path data = argc>1?argv[1]:"../Assignment 1/data";
    const fs::path output = argc>2?argv[2]:"results";
    const int run_limit = argc>3?std::stoi(argv[3]):200;
    if (run_limit<1||run_limit>200) throw std::runtime_error("Run limit should be 1..200");
    fs::create_directories(output/"plots");
    std::ofstream runs(output/"runs.csv"), objective(output/"objective_summary.csv"),
       timing(output/"runtime_summary.csv"), tours(output/"best_solutions.txt"),
       baseline(output/"previous_runs.csv"), log(output/"run_info.txt");
    if(!runs||!objective||!timing||!tours||!baseline||!log)
       throw std::runtime_error("Cannot write results");
    runs<<"instance,method,run,start,initial_objective,objective,improvement,accepted_moves,delta_checks,total_runtime_ms,search_runtime_ms\n";
    baseline<<"instance,method,run,start,objective,total_runtime_ms\n";
    objective<<"instance,assignment,method,runs,min,max,average\n";
    timing<<"instance,assignment,method,runs,mean_total_ms,mean_search_ms,total_time_ms\n";
    tours<<"# Zero-based indices, no repeated closing node; best of specified runs.\n";
    const std::array<std::string,2> instances{{"TSPA","TSPB"}};
    std::mt19937 old_rng(42), start_rng(42);
    for (int dataset=0;dataset<2;++dataset) {
       const auto& name=instances[dataset];
       const auto nodes=read_nodes((data/(name+".csv")).string());
       const Problem p=make_problem(nodes);
       std::cout<<"DATASET "<<name<<" ("<<p.size()<<" nodes)\n"<<std::flush;
       // Benchmark previous algorithms again in the same executable/hardware/build.
       // This also confirms the Assignment 1 / Assignment 2 objective values.
       for(int method=0;method<8;++method) {
          MethodSummary s; s.instance=name;s.method=previous_names[method];s.source=(method<4?"1":"2");
          for(int run=0;run<run_limit;++run) {
             auto time=Clock::now();
             const Tour t=previous_tour(p,method,run,old_rng);
             const Cost val=evaluate(p,t);
             const double ms=elapsed_ms(time);
             s.add(method==0?-1:run,t,val,ms);
             baseline<<name<<','<<s.method<<','<<run<<','<<(method==0?-1:run)<<','<<val<<','
                     <<std::setprecision(9)<<ms<<'\n';
          }
          objective<<name<<','<<s.source<<','<<s.method<<','<<s.count<<','<<s.min<<','<<s.max
                   <<','<<std::fixed<<std::setprecision(3)<<s.avg()<<'\n';
          timing<<name<<','<<s.source<<','<<s.method<<','<<s.count<<','<<std::fixed
                <<std::setprecision(6)<<s.ms_per_run()<<",,"<<s.total_ms<<'\n';
          std::cout<<"  BASELINE "<<s.method<<" min="<<s.min<<" mean="<<s.avg()<<"\n"<<std::flush;
       }
       // Same 200 starting tours are supplied to all 4 LS configurations in each
       // start class. Measured constructor time is included in total runtime.
       std::vector<Constructed> random_starts, greedy_starts;
       random_starts.reserve(run_limit);greedy_starts.reserve(run_limit);
       for(int run=0;run<run_limit;++run) {
          auto start=Clock::now();
          Tour t=random_solution(p,start_rng);
          random_starts.push_back({std::move(t),elapsed_ms(start)});
       }
       for(int run=0;run<run_limit;++run) {
          auto start=Clock::now();
          // The best minimum observed in earlier assignments for this dataset.
          Tour t = dataset==0
             ? regret_solution(p,run,Basis::GreedyCycle,Criterion::WeightedTwoRegret)
             : regret_solution(p,run,Basis::NearestAllPositions,Criterion::WeightedTwoRegret);
          greedy_starts.push_back({std::move(t),elapsed_ms(start)});
       }
       for (int method=0;method<8;++method) {
          const Variant& v=variants[method];
          MethodSummary s;s.instance=name;s.method=v.name;s.source="3";
          for(int run=0;run<run_limit;++run) {
             const Constructed& initial=(v.greedy_start?greedy_starts:random_starts)[run];
             const Cost initial_objective=evaluate(p,initial.tour);
             // Independent seeds for repeated deterministic randomized-order runs.
             std::mt19937 move_rng(static_cast<std::uint32_t>(
                     1000003 + dataset*100003 + method*1009 + run));
             const auto start=Clock::now();
             SearchStats result=local_search(p,initial.tour,v.strategy,v.intra,move_rng);
             const double search_ms=elapsed_ms(start);
             const double total_ms=initial.ms+search_ms;
             if (result.objective>initial_objective)
                 throw std::runtime_error("Local search worsened an initial solution");
             // Full evaluation used only after search finishes (NOT for neighbor deltas).
             if (result.objective!=evaluate(p,result.tour))
                 throw std::runtime_error("Final objective does not match accumulated deltas");
             s.add(run,result.tour,result.objective,total_ms,search_ms,initial_objective,
                   result.accepted_moves,result.delta_checks);
             runs<<name<<','<<v.name<<','<<run<<','<<run<<','<<initial_objective<<','
                 <<result.objective<<','<<(initial_objective-result.objective)<<','
                 <<result.accepted_moves<<','<<result.delta_checks<<','
                 <<std::fixed<<std::setprecision(6)<<total_ms<<','<<search_ms<<'\n';
          }
          objective<<name<<",3,"<<v.name<<','<<s.count<<','<<s.min<<','<<s.max
                   <<','<<std::fixed<<std::setprecision(3)<<s.avg()<<'\n';
          timing<<name<<",3,"<<v.name<<','<<s.count<<','<<std::fixed
                <<std::setprecision(6)<<s.ms_per_run()<<','<<s.local_ms_per_run()
                <<','<<s.total_ms<<'\n';
          tours<<name<<' '<<v.name<<' '<<s.min;
          for(int x:s.best_tour)tours<<' '<<x;
          tours<<'\n';
          save_svg(nodes,s.best_tour,output/"plots"/(name+"_"+v.name+".svg"),
                   name+" / "+v.name,s.min);
          std::cout<<"  LOCAL "<<v.name<<" min="<<s.min<<" max="<<s.max
                   <<" avg="<<s.avg()<<" mean-total-ms="<<s.ms_per_run()<<'\n'<<std::flush;
       }
    }
    log<<"Assignment 3 local search experiment, seed 42 for starting tours.\n"
       <<"Runs per instance and method: "<<run_limit<<"\n"
       <<"Each greedy LS scan uses std::shuffle on the MIXED list of move descriptors. "
       <<"Deltas are computed only as visited and scanning ends on first improvement.\n"
       <<"Timer: std::chrono::steady_clock, milliseconds, each run's total includes initial generation "
       <<"and local search. Baseline uses the same executable and optimized build.\n"
       <<"Greedy initialization TSPA: cycle_weighted_2regret; TSPB: nn_weighted_2regret.\n";
    std::cout<<"SUCCESS\n";
    return 0;
 } catch(const std::exception& e) {std::cerr<<"ERROR: "<<e.what()<<'\n';return 1;}
}

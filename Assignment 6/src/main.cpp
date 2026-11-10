#include "msls_ils.hpp"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>
namespace fs=std::filesystem;

struct Summary {
    std::string instance,method;
    int trials=0;
    Cost minimum=std::numeric_limits<Cost>::max();
    Cost maximum=std::numeric_limits<Cost>::min();
    long double sum_objective=0.,sum_time=0.,sum_ls=0.;
    int min_ls=std::numeric_limits<int>::max(),max_ls=0;
    Tour best_tour;
    int best_run=-1;
    void add(int r,const a6::RunResult& result) {
        ++trials;
        sum_objective+=result.best_objective;sum_time+=result.elapsed;
        sum_ls+=result.local_search_runs;
        min_ls=std::min(min_ls,result.local_search_runs);
        max_ls=std::max(max_ls,result.local_search_runs);
        if(result.best_objective<minimum) {
            minimum=result.best_objective;best_run=r;best_tour=result.best_tour;
        }
        maximum=std::max(maximum,result.best_objective);
    }
    void write(std::ofstream& f) const {
        f<<instance<<','<<method<<','<<trials<<','<<minimum<<','<<maximum<<','
         <<std::fixed<<std::setprecision(3)<<double(sum_objective/trials)<<','
         <<std::setprecision(6)<<double(sum_time/trials)<<','
         <<double(sum_ls/trials)<<','<<min_ls<<','<<max_ls<<','<<best_run<<'\n';
    }
};

void save_run(std::ofstream& f,const std::string& instance,const std::string& method,
              int trial,std::uint32_t seed,double budget,const a6::RunResult& res) {
    f<<instance<<','<<method<<','<<trial<<','<<seed<<','<<res.best_objective<<','
     <<std::fixed<<std::setprecision(6)<<res.elapsed<<','<<budget<<','
     <<res.local_search_runs<<','<<res.kicks<<','<<res.accepted_kicks<<','
     <<res.uphill_accepted<<','<<res.initial_local_objective<<','<<res.delta_checks<<'\n';
}
void save_progress(std::ofstream& f,const std::string& instance,const std::string& method,
                   int trial,const std::vector<a6::ProgressPoint>& points) {
    for(auto& p:points)
        f<<instance<<','<<method<<','<<trial<<','<<p.ls_runs<<','
         <<std::fixed<<std::setprecision(6)<<p.elapsed<<','<<p.best<<'\n';
}
int main(int argc,char** argv) {
    try {
        const fs::path data=argc>1?argv[1]:"../Assignment 1/data";
        const fs::path output=argc>2?argv[2]:"results";
        const int repeats=argc>3?std::stoi(argv[3]):a6::EXPERIMENT_REPEATS;
        const int restarts=argc>4?std::stoi(argv[4]):a6::MSLS_RESTARTS;
        if(repeats<=0||restarts<=0)throw std::runtime_error("Invalid experiment configuration");
        fs::create_directories(output);
        std::ofstream runs(output/"runs.csv"),summary(output/"summary.csv"),progress(output/"progress.csv"),
                      best(output/"best_solutions.txt"),budgets(output/"budgets.csv"),info(output/"run_info.txt");
        if(!runs||!summary||!progress||!best||!budgets||!info)throw std::runtime_error("Output files not writable");
        runs<<"instance,method,trial,seed,best_objective,elapsed_ms,target_budget_ms,ls_runs,kicks,accepted_kicks,uphill_accepted,initial_local_objective,delta_checks\n";
        summary<<"instance,method,trials,min,max,average,mean_elapsed_ms,mean_ls_runs,min_ls_runs,max_ls_runs,best_trial\n";
        progress<<"instance,method,trial,ls_runs,elapsed_ms,best_objective\n";
        best<<"# instance method objective then 100 zero-based indices (no duplicate closing index)\n";
        budgets<<"instance,msls_mean_ms,ils_budget_ms,msls_restarts,experiments\n";
        info<<"Assignment 6; C++17; random_solution from Assignment 1, exact unrestricted Steepest 2-opt + inter-route replacement from Assignment 3.\n"
            <<"MSLS: "<<restarts<<" independently randomized starts per run; retain best local optimum.\n"
            <<"ILS: initial random start, Steepest, then 4 distinct selected/outside replacements and double bridge A|B|C|D -> A|C|B|D; after kick, full Steepest.\n"
            <<"ILS acceptance: if candidate <= current, accept; else probability 0.12. After 80 non-best-improving kicks, restart current from global best.\n"
            <<"The ILS stop time is the observed mean wall-clock duration of "<<repeats<<" MSLS runs ON THAT INSTANCE. Last LS may exceed budget.\n"
            <<"The MSLS and ILS algorithms are both tested "<<repeats<<" times per instance.\n"
            <<"MSLS seeds 2026600 + instanceIndex*100000 + trial*97 + 11; ILS seeds 2026800 + instanceIndex*100000 + trial*97 + 29.\n";
        for(int id=0;id<2;++id) {
            const std::string name=id==0?"TSPA":"TSPB";
            const auto nodes=read_nodes((data/(name+".csv")).string());
            const Problem p=make_problem(nodes);
            if(p.size()!=200||p.selected_count()!=100)throw std::runtime_error("Wrong instance size");
            Summary ms;ms.instance=name;ms.method="MSLS";
            for(int trial=0;trial<repeats;++trial) {
                std::vector<a6::ProgressPoint> points;
                const auto seed=static_cast<std::uint32_t>(2026600+id*100000+trial*97+11);
                const auto result=a6::msls(p,seed,&points,restarts);
                if(evaluate(p,result.best_tour)!=result.best_objective)throw std::runtime_error("MSLS objective error");
                ms.add(trial,result);
                save_run(runs,name,"MSLS",trial,seed,0.,result);
                save_progress(progress,name,"MSLS",trial,points);
                std::cout<<name<<" MSLS "<<(trial+1)<<"/"<<repeats<<" score="<<result.best_objective
                         <<" ms="<<std::fixed<<std::setprecision(2)<<result.elapsed<<"\n"<<std::flush;
            }
            const double budget=static_cast<double>(ms.sum_time/ms.trials);
            std::cout<<name<<" ILS BUDGET (mean MSLS) "<<budget<<" ms\n"<<std::flush;
            Summary il;il.instance=name;il.method="ILS";
            for(int trial=0;trial<repeats;++trial) {
                std::vector<a6::ProgressPoint> points;
                const auto seed=static_cast<std::uint32_t>(2026800+id*100000+trial*97+29);
                const auto result=a6::ils(p,seed,budget,&points);
                if(evaluate(p,result.best_tour)!=result.best_objective)throw std::runtime_error("ILS objective error");
                il.add(trial,result);
                save_run(runs,name,"ILS",trial,seed,budget,result);
                save_progress(progress,name,"ILS",trial,points);
                std::cout<<name<<" ILS "<<(trial+1)<<"/"<<repeats<<" score="<<result.best_objective
                         <<" ms="<<std::fixed<<std::setprecision(2)<<result.elapsed<<" LS="<<result.local_search_runs<<"\n"<<std::flush;
            }
            ms.write(summary);il.write(summary);
            budgets<<name<<','<<std::fixed<<std::setprecision(6)<<budget<<','<<budget<<','<<restarts<<','<<repeats<<'\n';
            for(const Summary* s:{&ms,&il}) {
                best<<name<<' '<<s->method<<' '<<s->minimum;
                for(int v:s->best_tour)best<<' '<<v;
                best<<'\n';
            }
        }
        return 0;
    } catch(const std::exception& ex) {
        std::cerr<<"ERROR: "<<ex.what()<<'\n';return 1;
    }
}

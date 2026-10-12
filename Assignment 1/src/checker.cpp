// Independent coordinate-based checker; does not reuse algorithm/evaluation code.
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
        std::string line;
        int checked = 0;
        std::set<std::string> records;
        const std::set<std::string> allowed_methods{"random", "nearest_end", "nearest_all_positions", "greedy_cycle"};
        while (std::getline(solutions, line)) {
            if (line.empty() || line[0] == '#') continue;
            std::istringstream input(line);
            std::string instance, method;
            long long claimed;
            if (!(input >> instance >> method >> claimed)) throw std::runtime_error("Malformed solution header");
            if ((instance != "TSPA" && instance != "TSPB") || !allowed_methods.count(method) || !records.insert(instance + method).second)
                throw std::runtime_error("Unexpected or duplicate solution record");
            std::ifstream data(std::filesystem::path(argv[1]) / (instance + ".csv"));
            if (!data) throw std::runtime_error("Cannot open instance");
            std::vector<Point> points;
            std::string row;
            while (std::getline(data, row)) {
                std::istringstream values(row);
                Point p{}; char first, second; std::string extra;
                if (!(values >> p.x >> first >> p.y >> second >> p.cost) || first != ';' || second != ';' || (values >> extra))
                    throw std::runtime_error("Malformed input data");
                points.push_back(p);
            }
            std::vector<int> tour;
            int node;
            while (input >> node) tour.push_back(node);
            if (!input.eof()) throw std::runtime_error("Malformed node index");
            if (tour.empty() || tour.size() != (points.size() + 1) / 2) throw std::runtime_error("Wrong tour size");
            std::set<int> seen;
            for (int i : tour)
                if (i < 0 || i >= static_cast<int>(points.size()) || !seen.insert(i).second) throw std::runtime_error("Invalid or duplicate index");
            long long distance = 0, cost = 0;
            for (std::size_t i = 0; i < tour.size(); ++i) {
                const auto& a = points[tour[i]]; const auto& b = points[tour[(i + 1) % tour.size()]];
                const long double dx = static_cast<long double>(a.x) - b.x, dy = static_cast<long double>(a.y) - b.y;
                distance += static_cast<long long>(std::floor(std::sqrt(dx * dx + dy * dy) + 0.5L));
                cost += a.cost;
            }
            if (distance + cost != claimed) throw std::runtime_error("Objective mismatch");
            std::cout << "PASS " << instance << ' ' << method << " nodes=" << tour.size() << " distance=" << distance << " node_cost=" << cost << " total=" << claimed << '\n';
            ++checked;
        }
        if (checked != 8) throw std::runtime_error("Expected all eight best solutions");
        std::cout << "All 8 best solutions independently verified.\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}

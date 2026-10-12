#include "heuristics.hpp"
#include <array>
#include <filesystem>
#include <iomanip>
#include <iostream>

namespace fs = std::filesystem;
const std::array<std::string, 4> names{"random", "nearest_end", "nearest_all_positions", "greedy_cycle"};

struct Result { Cost minimum, maximum; double average; Tour best; };

// Coordinates are used only for output, never passed to a heuristic.
void plot_svg(const std::vector<Node>& nodes, const Tour& tour, const std::string& path,
              const std::string& title, Cost objective) {
    std::ofstream out(path);
    if (!out) throw std::runtime_error("Cannot write plot: " + path);
    int xmin = nodes[0].x, xmax = xmin, ymin = nodes[0].y, ymax = ymin;
    Cost cmin = nodes[0].cost, cmax = cmin;
    for (const auto& n : nodes) {
        xmin = std::min(xmin, n.x); xmax = std::max(xmax, n.x);
        ymin = std::min(ymin, n.y); ymax = std::max(ymax, n.y);
        cmin = std::min(cmin, n.cost); cmax = std::max(cmax, n.cost);
    }
    const double scale = 580.0 / std::max({1, xmax - xmin, ymax - ymin});
    auto x = [&](int i) { return 80.0 + (nodes[i].x - xmin) * scale; };
    auto y = [&](int i) { return 680.0 - (nodes[i].y - ymin) * scale; };
    out << std::fixed << std::setprecision(2);
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"860\" height=\"780\" viewBox=\"0 0 860 780\">\n"
        << "<rect width=\"860\" height=\"780\" fill=\"white\"/>\n"
        << "<g font-family=\"sans-serif\" fill=\"#182335\"><text x=\"60\" y=\"35\" font-size=\"21\">" << title << "</text>"
        << "<text x=\"60\" y=\"63\" font-size=\"15\">Objective: " << objective << " | selected: " << tour.size() << '/' << nodes.size() << "</text></g>\n"
        << "<path d=\"M 80 95 V 680 H 675\" fill=\"none\" stroke=\"#9ca3af\"/>\n"
        << "<g font-family=\"sans-serif\" font-size=\"13\" fill=\"#475569\">"
        << "<text x=\"78\" y=\"701\">" << xmin << "</text><text x=\"" << 80 + (xmax - xmin) * scale << "\" y=\"701\">" << xmax << "</text>"
        << "<text x=\"20\" y=\"682\">" << ymin << "</text><text x=\"20\" y=\"" << 680 - (ymax - ymin) * scale << "\">" << ymax << "</text>"
        << "<text x=\"360\" y=\"728\">x coordinate</text><text x=\"10\" y=\"390\" transform=\"rotate(-90 10 390)\">y coordinate</text></g>\n";
    out << "<polyline fill=\"none\" stroke=\"#334155\" stroke-width=\"1.6\" points=\"";
    for (int node : tour) out << x(node) << ',' << y(node) << ' ';
    out << x(tour.front()) << ',' << y(tour.front()) << "\"/>\n";
    std::vector<bool> selected(nodes.size());
    for (int node : tour) selected[node] = true;
    auto color = [&](Cost cost) {
        const double t = cmax == cmin ? 0 : static_cast<double>(cost - cmin) / (cmax - cmin);
        return "rgb(" + std::to_string(static_cast<int>(40 + 200 * t)) + "," +
               std::to_string(static_cast<int>(130 - 80 * t)) + "," + std::to_string(static_cast<int>(220 - 180 * t)) + ")";
    };
    for (int layer = 0; layer < 2; ++layer)
        for (int i = 0; i < static_cast<int>(nodes.size()); ++i) {
            if (selected[i] != static_cast<bool>(layer)) continue;
            out << "<circle cx=\"" << x(i) << "\" cy=\"" << y(i) << "\" r=\"" << (selected[i] ? 4.5 : 3.0)
                << "\" fill=\"" << color(nodes[i].cost) << "\" opacity=\"" << (selected[i] ? 1.0 : 0.45)
                << "\" stroke=\"" << (selected[i] ? "#0f172a" : "none") << "\"><title>Node " << i
                << "; cost " << nodes[i].cost << (selected[i] ? "; selected" : "; not selected") << "</title></circle>\n";
        }
    out << "<g font-family=\"sans-serif\" font-size=\"13\" fill=\"#334155\"><text x=\"705\" y=\"125\">Node cost</text>";
    for (int i = 0; i < 100; ++i)
        out << "<rect x=\"710\" y=\"" << 150 + i * 3 << "\" width=\"22\" height=\"3.1\" fill=\"" << color(cmin + (cmax - cmin) * i / 99) << "\"/>";
    out << "<text x=\"740\" y=\"160\">" << cmin << "</text><text x=\"740\" y=\"450\">" << cmax << "</text>"
        << "<text x=\"700\" y=\"500\">Outlined: selected</text><text x=\"700\" y=\"525\">Faded: unselected</text>"
        << "<text x=\"60\" y=\"758\">Closed tour; equal scaling on both axes. Node indices are zero-based.</text></g></svg>\n";
}

int main(int argc, char** argv) {
    try {
        const fs::path data = argc > 1 ? argv[1] : "data";
        const fs::path output = argc > 2 ? argv[2] : "results";
        const unsigned seed = argc > 3 ? static_cast<unsigned>(std::stoul(argv[3])) : 42;
        fs::create_directories(output / "plots");
        std::ofstream summary(output / "summary.csv"), runs(output / "runs.csv"), best(output / "best_solutions.txt");
        if (!summary || !runs || !best) throw std::runtime_error("Cannot write result files");
        summary << "instance,method,runs,min,max,average\n";
        runs << "instance,method,run,start,objective\n";
        best << "# Zero-based node indices, no repeated closing node. Random seed: " << seed << "\n";
        std::mt19937 rng(seed);
        for (const auto& instance : {"TSPA", "TSPB"}) {
            const auto nodes = read_nodes((data / (std::string(instance) + ".csv")).string());
            const Problem p = make_problem(nodes); // immediately after reading
            for (int method = 0; method < 4; ++method) {
                const int count = method == 0 ? 200 : p.size();
                Result r{std::numeric_limits<Cost>::max(), std::numeric_limits<Cost>::min(), 0, {}};
                Cost sum = 0;
                for (int run = 0; run < count; ++run) {
                    Tour tour = method == 0 ? random_solution(p, rng) : greedy_solution(p, run, static_cast<Method>(method - 1));
                    const Cost value = evaluate(p, tour);
                    runs << instance << ',' << names[method] << ',' << run << ',' << (method == 0 ? -1 : run) << ',' << value << '\n';
                    if (value < r.minimum) { r.minimum = value; r.best = tour; }
                    r.maximum = std::max(r.maximum, value); sum += value;
                }
                r.average = static_cast<double>(sum) / count;
                summary << instance << ',' << names[method] << ',' << count << ',' << r.minimum << ',' << r.maximum << ',' << std::fixed << std::setprecision(3) << r.average << '\n';
                best << instance << ' ' << names[method] << ' ' << r.minimum;
                for (int node : r.best) best << ' ' << node;
                best << '\n';
                plot_svg(nodes, r.best, (output / "plots" / (std::string(instance) + "_" + names[method] + ".svg")).string(), std::string(instance) + " / " + names[method], r.minimum);
                std::cout << instance << ' ' << names[method] << " min=" << r.minimum << " max=" << r.maximum << " average=" << r.average << '\n';
            }
        }
        return 0;
    } catch (const std::exception& e) { std::cerr << "Error: " << e.what() << '\n'; return 1; }
}

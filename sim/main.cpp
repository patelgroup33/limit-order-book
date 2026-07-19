#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#include "json_export.hpp"
#include "order_flow_simulator.hpp"

/// @file main.cpp
/// @brief `lob_sim`: runs the order-flow simulation and writes a JSON replay.
///
/// Usage: lob_sim [steps] [output.json]
///   steps   number of commands to simulate (default 1200)
///   output  path to write the JSON replay (default simulation.json)

int main(int argc, char** argv) {
    std::size_t steps = 1200;
    std::string output = "simulation.json";

    if (argc > 1) {
        steps = static_cast<std::size_t>(std::strtoull(argv[1], nullptr, 10));
    }
    if (argc > 2) {
        output = argv[2];
    }

    lobsim::OrderFlowSimulator sim{lobsim::OrderFlowSimulator::Config{}};
    const lobsim::SimResult result = sim.run(steps);

    std::ofstream out(output);
    if (!out) {
        std::cerr << "error: could not open '" << output << "' for writing\n";
        return 1;
    }
    lobsim::write_simulation_json(out, result);

    // A quick summary for the console.
    std::size_t total_trades = 0;
    for (const lobsim::Frame& f : result.frames) {
        total_trades += f.trades.size();
    }
    std::cout << "wrote " << result.frames.size() << " frames, " << total_trades
              << " trades to " << output << '\n';
    return 0;
}

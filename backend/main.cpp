#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include <fstream>
#include <condition_variable>

#include "src/server/server.hpp"

int main(int argc, char ** argv) {
    const int MAX_PROCESSES = 1;
    printf("\033c");

    std::ifstream f("config.json");
    nlohmann::json config = nlohmann::json::parse(f);

    Database db = Database();
    WorkerProcessHandler wph = WorkerProcessHandler(1, &db);
    Server server = Server(&config, &wph, &db);
    server.run();

    return 0;
}

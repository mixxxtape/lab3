//Mishyna Daryna K-24 variant 10

#include <iostream>
#include <syncstream>
#include <sstream>
#include <vector>
#include <thread>
#include <chrono>
#include <string>
#include <map>
#include <latch>
#include <atomic>

using namespace std::chrono_literals;

void f(const std::string& setname, int idx) {
    std::this_thread::sleep_for(100ms);
    std::osyncstream os(std::cout);
    os << "From set " << setname << " action " << idx << " completed." << std::endl;
}

struct SetInfo {
    std::string name;
    int count;
    std::vector<int> preds;
    std::vector<int> succs;
};

int main() {
    const int nt = 4;

    std::vector<SetInfo> sets = {
        {"a", 7, {}},
        {"b", 9, {0}},
        {"c", 4, {0}},
        {"d", 8, {1}},
        {"e", 7, {1}},
        {"f", 5, {2}},
        {"g", 7, {3,4}},
        {"h", 9, {4}},
        {"i", 8, {5}},
        {"j", 5, {6,7,8}}
    };

    int S = sets.size();

    for (int i = 0; i < S; ++i)
        for (int p : sets[i].preds)
            sets[p].succs.push_back(i);

    std::vector<std::unique_ptr<std::latch>> latches;
    latches.reserve(S);

    for (int i = 0; i < S; ++i) {
        latches.emplace_back(std::make_unique<std::latch>(sets[i].preds.size()));
    }

    std::vector<std::atomic<int>> remaining(S);
    for (int i = 0; i < S; ++i)
        remaining[i].store(sets[i].count);

    std::cout << "Computation started." << std::endl;

    std::vector<std::thread> workers;
    workers.reserve(nt);

    for (int tid = 0; tid < nt; ++tid) {
        workers.emplace_back([tid, nt, &sets, &latches, &remaining]() {
            for (int s = 0; s < (int)sets.size(); ++s) {
                latches[s]->wait();

                std::vector<int> my_actions;
                int cnt = sets[s].count;
                for (int k = 1; k <= cnt; ++k)
                    if ((k - 1) % nt == tid)
                        my_actions.push_back(k);

                for (int action : my_actions)
                    f(sets[s].name, action);

                int done = my_actions.size();
                if (done > 0) {
                    int prev = remaining[s].fetch_sub(done);
                    if (prev == done) {
                        for (int succ : sets[s].succs)
                            latches[succ]->count_down();
                    }
                }
            }
            });
    }

    for (auto& t : workers)
        if (t.joinable()) t.join();

    std::cout << "Computation finished." << std::endl;
    return 0;

}

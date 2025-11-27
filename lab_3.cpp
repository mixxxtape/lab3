//Mishyna Daryna K-24 variant 10

#include <iostream>
#include <syncstream>
#include <sstream>
#include <thread>
#include <vector>
#include <string>
#include <map>
#include <latch>
#include <atomic>
#include <chrono>

using namespace std;
using namespace chrono_literals;

void f(const string& setname, int idx) {
    this_thread::sleep_for(100ms);
    osyncstream os(cout);
    os << "From set " << setname << " action " << idx << " executed." << endl;
}

struct SetInfo {
    string name;
    int count;
    vector<int> preds;
    vector<int> succs;
};

int main() {
    const int nt = 4;

    vector<SetInfo> sets = {
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

    vector<unique_ptr<latch>> latches;
    latches.reserve(S);

    for (int i = 0; i < S; ++i) {
        latches.emplace_back(make_unique<latch>(sets[i].preds.size()));
    }

    vector<atomic<int>> remaining(S);
    for (int i = 0; i < S; ++i)
        remaining[i].store(sets[i].count);

    cout << "Computation started." << endl;

    vector<thread> workers;
    workers.reserve(nt);

    for (int tid = 0; tid < nt; ++tid) {
        workers.emplace_back([tid, nt, &sets, &latches, &remaining]() {

            for (int s = 0; s < (int)sets.size(); ++s) {
                latches[s]->wait();

                vector<int> my_actions;
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

    cout << "Computation finished." << endl;
    return 0;
}

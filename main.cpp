#include <iostream>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <climits>
using namespace std;

struct GanttEntry {
    int pid;
    int start;
    int end;
    string type;
};

struct Process {
    int id = 0;
    int arrival = 0;
    int readyTime = 0;
    vector<int> cpu;
    vector<int> io;
    int idx = 0;
    int rem = 0;
    bool inIO = false;
    int ioDone = 0;
    bool finished = false;
    int terminate = 0;
    int totalCPU = 0;
    int totalIO = 0;
    int waiting_time = 0;
    int response_time = -1;
    vector<int> enqueue_times;
};

void addGantt(vector<GanttEntry>& g, int pid, int s, int e, const string& t) {
    if (s < e) g.push_back({pid, s, e, t});
}

int main() {
    ifstream fin("input.txt");
    ofstream fout("output.txt");

    if (!fin.is_open()) {
        cerr << "Error: Could not open input.txt\n";
        return 1;
    }
    if (!fout.is_open()) {
        cerr << "Error: Could not open output.txt\n";
        return 1;
    }

    string line;
    getline(fin, line); int tq = stoi(line.substr(2));
    getline(fin, line); int dl = stoi(line.substr(3));
    getline(fin, line); int n = stoi(line.substr(2));

    vector<Process> P(n);
    for (int i = 0; i < n; ++i) {
        getline(fin, line);
        size_t colon = line.find(':');
        string pidStr = line.substr(1, colon - 1);
        P[i].id = stoi(pidStr);
        string data = line.substr(colon + 1);
        stringstream ss(data);
        string token;
        vector<int> nums;
        while (getline(ss, token, ',')) {
            nums.push_back(stoi(token));
        }

        P[i].arrival = nums[0];
        P[i].readyTime = nums[1];
        int nb = (int(nums.size()) - 2 + 1) / 2;
        P[i].cpu.resize(nb);
        if (nb > 1) P[i].io.resize(nb - 1);

        int idx = 2;
        for (int j = 0; j < nb; ++j) {
            P[i].cpu[j] = nums[idx++];
            P[i].totalCPU += P[i].cpu[j];
            if (j < nb - 1) {
                P[i].io[j] = nums[idx++];
                P[i].totalIO += P[i].io[j];
            }
        }
        P[i].rem = 0;
    }
    fin.close();

    vector<GanttEntry> gantt;
    vector<int> ready;
    vector<bool> arrived(n, false);
    int completed = 0;
    int cur = INT_MAX;
    for (auto& pr : P) cur = min({cur, pr.arrival, pr.readyTime});

    auto collectReadyAt = [&](int t, vector<int>& pendingReady) {
        for (int i = 0; i < n; ++i) {
            if (!arrived[i] && P[i].readyTime <= t) {
                arrived[i] = true;
                P[i].idx = 0;
                P[i].rem = P[i].cpu[0];
                P[i].enqueue_times.push_back(t);
                pendingReady.push_back(i);
            }
        }
    };

    auto collectIOAt = [&](int t, vector<int>& pendingIO) {
        for (int i = 0; i < n; ++i) {
            if (P[i].inIO && P[i].ioDone <= t) {
                P[i].inIO = false;
                if (P[i].idx < (int)P[i].cpu.size()) {
                    P[i].rem = P[i].cpu[P[i].idx];
                    P[i].enqueue_times.push_back(t);
                    pendingIO.push_back(i);
                } else if (!P[i].finished) {
                    P[i].finished = true;
                    P[i].terminate = t;
                    completed++;
                }
            }
        }
    };

    vector<int> pend;
    collectReadyAt(cur, pend);
    for (int id : pend) ready.push_back(id);

    bool firstDispatch = true;
    long long cpuBusy = 0;

    while (completed < n) {
        vector<int> pendingNew, pendingIO, pendingPreempt;
        collectIOAt(cur, pendingIO);
        collectReadyAt(cur, pendingNew);
        for (int id : pendingNew) ready.push_back(id);
        for (int id : pendingIO) ready.push_back(id);
        pendingNew.clear(); pendingIO.clear();

        if (ready.empty()) {
            int nextEv = INT_MAX;
            for (int i = 0; i < n; ++i) {
                if (!arrived[i]) nextEv = min(nextEv, P[i].readyTime);
                if (P[i].inIO) nextEv = min(nextEv, P[i].ioDone);
            }
            if (nextEv == INT_MAX) break;
            if (nextEv > cur) {
                addGantt(gantt, -1, cur, nextEv, "IDLE");
                cur = nextEv;
            } else {
                cur++;
            }
            continue;
        }

        int pid = ready.front();
        ready.erase(ready.begin());

        if (!firstDispatch && dl > 0) {
            int s = cur, e = cur + dl;
            addGantt(gantt, -1, s, e, "DL");
            for (int t = s + 1; t <= e; ++t) {
                collectIOAt(t, pendingIO);
                collectReadyAt(t, pendingNew);
            }
            cur = e;
            for (int id : pendingNew) ready.push_back(id);
            for (int id : pendingIO) ready.push_back(id);
            pendingNew.clear(); pendingIO.clear();
        }
        firstDispatch = false;

        if (P[pid].enqueue_times.empty()) {
            P[pid].enqueue_times.push_back(cur);
        }
        int enq_time = P[pid].enqueue_times.front();
        P[pid].enqueue_times.erase(P[pid].enqueue_times.begin());
        int startTime = cur;

        int lastDL = 0;
        if (!gantt.empty() && gantt.back().type == "DL") {
            lastDL = gantt.back().end - gantt.back().start;
        }

        int wait_inc = startTime - enq_time - lastDL;
        if (wait_inc < 0) wait_inc = 0;
        P[pid].waiting_time += wait_inc;

        if (P[pid].rem <= 0 && P[pid].idx < (int)P[pid].cpu.size()) {
            P[pid].rem = P[pid].cpu[P[pid].idx];
        }

        int run = min(tq, P[pid].rem);
        int s = cur, e = cur + run;
        P[pid].rem -= run;
        addGantt(gantt, P[pid].id, s, e, "CPU");
        cpuBusy += run;

        for (int t = s + 1; t <= e; ++t) {
            collectIOAt(t, pendingIO);
            collectReadyAt(t, pendingNew);
        }
        cur = e;
        for (int id : pendingNew) ready.push_back(id);
        for (int id : pendingIO) ready.push_back(id);

        if (P[pid].rem == 0) {
            int bidx = P[pid].idx;
            if (bidx < (int)P[pid].cpu.size() - 1) {
                if (P[pid].response_time == -1) {
                    P[pid].response_time = cur - P[pid].arrival;
                }
                P[pid].inIO = true;
                P[pid].ioDone = cur + P[pid].io[bidx];
                addGantt(gantt, P[pid].id, cur, P[pid].ioDone, "I/O");
                P[pid].idx++;
            } else {
                if (!P[pid].finished) {
                    P[pid].finished = true;
                    P[pid].terminate = cur;
                    if (P[pid].response_time == -1) {
                        P[pid].response_time = cur - P[pid].arrival;
                    }
                    completed++;
                }
            }
        } else {
            P[pid].enqueue_times.push_back(cur);
            ready.push_back(pid);
        }
    }


    int totalTime = 0;
    for (int i = 0; i < n; ++i) totalTime = max(totalTime, P[i].terminate);

    double cpuUtil = (totalTime > 0) ? (100.0 * cpuBusy / totalTime) : 0.0;
    double throughput = (totalTime > 0) ? (double)n / totalTime : 0.0;

    double avgTurn = 0.0, avgWait = 0.0, avgResponse = 0.0;
    for (int i = 0; i < n; ++i) {
        int ta = P[i].terminate - P[i].arrival;
        avgTurn += ta;
        avgWait += P[i].waiting_time;
        avgResponse += P[i].response_time;
    }
    avgTurn /= n;
    avgWait /= n;
    avgResponse /= n;

    fout << "\n\nPROCESS SCHEDULING TABLE:\n";
    fout << left
         << setw(8) << "Process"
         << setw(8) << "Arrival"
         << setw(8) << "Ready"
         << setw(10) << "Response"
         << setw(10) << "Terminate"
         << setw(12) << "Turnaround"
         << setw(10) << "Waiting"
         << "\n";
    fout << string(80, '-') << "\n";

    for (int i = 0; i < n; ++i) {
        int ta = P[i].terminate - P[i].arrival;
        fout << left
             << setw(8) << ("P" + to_string(P[i].id))
             << setw(8) << P[i].arrival
             << setw(8) << P[i].readyTime
             << setw(10) << P[i].response_time
             << setw(10) << P[i].terminate
             << setw(12) << ta
             << setw(10) << P[i].waiting_time
             << "\n";
    }
    fout << string(80, '-') << "\n";

    fout << "Total Time: " << totalTime << " units\n";
    fout << "CPU Busy Time: " << cpuBusy << " units\n";
    fout << fixed << setprecision(2);
    fout << "CPU Utilization: " << cpuUtil << " %\n";
    fout << "Throughput: " << throughput << " processes/unit-time\n";
    fout << "Average Turnaround Time: " << avgTurn << " units\n";
    fout << "Average Waiting Time: " << avgWait << " units\n";
    fout << "Average Response Time: " << avgResponse << " units\n";

    fout << "\nGantt-Chart:\n";
    fout << string(80, '-') << "\n";

    for (auto& g : gantt) {
        if (g.type == "I/O") continue;
        if (g.type == "CPU")      fout << "| P" << g.pid << " |";
        else if (g.type == "IDLE") fout << "| IDLE |";
        else if (g.type == "DL")   fout << "| DL |";
    }
    fout << "\n";

    for (auto& g : gantt) {
        if (g.type == "I/O") continue;
        fout << setw(6) << g.start;
    }
    fout << setw(6) << totalTime;
    fout << "\n";
    fout << string(80, '-') << "\n";

    ofstream trace("trace.txt");
    if (trace.is_open()) {
        trace << "Detail:\n";
        for (auto& g : gantt) {
            if (g.type == "I/O") continue;
            trace << "Time " << setw(3) << g.start << "-" << setw(3) << g.end << " : ";
            if (g.type == "CPU")      trace << "Process P" << g.pid << " (CPU)";
            else if (g.type == "IDLE") trace << "CPU IDLE";
            else if (g.type == "DL")   trace << "Dispatcher Latency (Context Switch)";
            trace << "\n";
        }
        trace.close();
        fout << "\nTrace written to trace.txt\n";
    } else {
        fout << "\nCould not create trace.txt\n";
    }

    fout.close();
    cout << "Results in output.txt and trace.txt\n";
    return 0;
}
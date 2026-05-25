#include <cmath>
#include <condition_variable>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <queue>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>

using namespace std;

template <typename T>
T fun_sin(T arg) {
    return sin(arg);
}

template <typename T>
T fun_sqrt(T arg) {
    return sqrt(arg);
}

template <typename T>
T fun_pow(T arg, int power) {
    return pow(arg, power);
}

template <typename T>
class Server {
public:
    using task_type = function<T()>;

    void start() {
        stopped = false;
        worker = thread(&Server::work, this);
    }

    void stop() {
        {
            lock_guard<mutex> lock(mut);
            stopped = true;
        }
        cond.notify_all();
        result_cond.notify_all();

        if (worker.joinable())
            worker.join();
    }

    size_t add_task(task_type task) {
        lock_guard<mutex> lock(mut);
        size_t id = next_id++;
        tasks.push({id, move(task)});
        cond.notify_one();
        return id;
    }

    T request_result(size_t id) {
        unique_lock<mutex> lock(mut);
        result_cond.wait(lock, [&]() { 
            return results.find(id) != results.end() || stopped; 
        });

        auto it = results.find(id);
        if (it == results.end()) {
            throw runtime_error("Result not available for id: " + to_string(id));
        }

        T result = it->second;
        results.erase(it);
        return result;
    }

private:
    void work() {
        while (true) {
            pair<size_t, task_type> task;

            {
                unique_lock<mutex> lock(mut);
                cond.wait(lock, [&]() { 
                    return !tasks.empty() || stopped; 
                });

                if (stopped && tasks.empty())
                    break;

                task = move(tasks.front());
                tasks.pop();
            }

            T result = task.second();

            {
                lock_guard<mutex> lock(mut);
                results[task.first] = result;
            }
            result_cond.notify_all();
        }
    }

    mutex mut;
    condition_variable cond;
    condition_variable result_cond;
    queue<pair<size_t, task_type>> tasks;
    unordered_map<size_t, T> results;  
    thread worker;
    size_t next_id = 1;
    bool stopped = false;
};

void sin_client(Server<double>& server, int N) {
    ofstream file("sin_results.txt");
    
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<double> dist(-1000.0, 1000.0);

    file << setprecision(17);
    for (int i = 0; i < N; i++) {
        double x = dist(gen);
        size_t id = server.add_task([x]() { return fun_sin(x); });
        double result = server.request_result(id);
        file << "sin " << x << " = " << result << '\n';
    }
    file.close();
}

void sqrt_client(Server<double>& server, int N) {
    ofstream file("sqrt_results.txt");
    
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<double> dist(0.0, 100000.0);

    file << setprecision(17);
    for (int i = 0; i < N; i++) {
        double x = dist(gen);
        size_t id = server.add_task([x]() { return fun_sqrt(x); });
        double result = server.request_result(id);
        file << "sqrt " << x << " = " << result << '\n';
    }
    file.close();
}

void pow_client(Server<double>& server, int N) {
    ofstream file("pow_results.txt");
    
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<double> base_dist(1.0, 10.0);
    uniform_int_distribution<int> power_dist(1, 5);

    file << setprecision(17);
    for (int i = 0; i < N; i++) {
        double x = base_dist(gen);
        int p = power_dist(gen);
        size_t id = server.add_task([x, p]() { return fun_pow(x, p); });
        double result = server.request_result(id);
        file << "pow " << x << " ^ " << p << " = " << result << '\n';
    }
    file.close();
}

bool equal_double(double a, double b) {
    return abs(a - b) <= 1e-10 * max(1.0, abs(a));
}

bool check_file(const string& file_name) {
    ifstream file(file_name);
    if (!file.is_open()) {
        cerr << "Cannot open " << file_name << " for testing\n";
        return false;
    }
    
    string op;
    bool ok = true;

    while (file >> op) {
        if (op == "sin") {
            double x, result;
            char eq;
            file >> x >> eq >> result;
            if (!equal_double(fun_sin(x), result)) {
                cout << "Error: sin(" << x << ") = " << fun_sin(x) 
                     << ", but got " << result << '\n';
                ok = false;
            }
        } 
        else if (op == "sqrt") {
            double x, result;
            char eq;
            file >> x >> eq >> result;
            if (!equal_double(fun_sqrt(x), result)) {
                cout << "Error: sqrt(" << x << ") = " << fun_sqrt(x) 
                     << ", but got " << result << '\n';
                ok = false;
            }
        } 
        else if (op == "pow") {
            double x, result;
            int p;
            char caret, eq;
            file >> x >> caret >> p >> eq >> result;
            if (!equal_double(fun_pow(x, p), result)) {
                cout << "Error: pow(" << x << ", " << p << ") = " 
                     << fun_pow(x, p) << ", but got " << result << '\n';
                ok = false;
            }
        }
    }
    
    file.close();
    return ok;
}

int main(int argc, char* argv[]) {
    if (argc >= 2 && string(argv[1]) == "test") {
        bool ok = check_file("sin_results.txt") &&
                  check_file("sqrt_results.txt") &&
                  check_file("pow_results.txt");
        cout << (ok ? "Test passed!\n" : "Test failed!\n");
        return ok ? 0 : 1;
    }

    int N = 10;
    if (argc >= 2)
        N = stoi(argv[1]);

    if (N <= 5 || N >= 10000) {
        cerr << "Error: N must be: 5 < N < 10000\n";
        return 1;
    }

    Server<double> server;
    server.start();

    thread client1(sin_client, ref(server), N);
    thread client2(sqrt_client, ref(server), N);
    thread client3(pow_client, ref(server), N);

    client1.join();
    client2.join();
    client3.join();

    server.stop();

    return 0;
}


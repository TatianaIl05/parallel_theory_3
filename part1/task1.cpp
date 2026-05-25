#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <iomanip>

using namespace std;
using namespace chrono;


void parallel_init(vector<double>& A, vector<double>& b, int N, int nthreads) {
    vector<thread> threads;
    
    for (int tid = 0; tid < nthreads; ++tid) {
        threads.emplace_back([&, tid]() {
            int rows_per_thread = N / nthreads;
            int start_row = tid * rows_per_thread;
            int end_row = (tid == nthreads - 1) ? N : start_row + rows_per_thread;
            
            for (int i = start_row; i < end_row; ++i) {
                for (int j = 0; j < N; ++j) {
                    A[i * N + j] = (double)(i + j);
                }

                b[i] = (double)i;
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
}


double parallel_matvec(vector<double>& A, vector<double>& b, vector<double>& c, 
                       int N, int nthreads) {
    auto start = steady_clock::now();
    
    vector<thread> threads;
    
    for (int tid = 0; tid < nthreads; ++tid) {
        threads.emplace_back([&, tid]() {
            int rows_per_thread = N / nthreads;
            int start_row = tid * rows_per_thread;
            int end_row = (tid == nthreads - 1) ? N : start_row + rows_per_thread;
            
            for (int i = start_row; i < end_row; ++i) {
                double sum = 0.0;
                for (int j = 0; j < N; ++j) {
                    sum += A[i * N + j] * b[j];
                }
                c[i] = sum;
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto end = steady_clock::now();
    return duration<double>(end - start).count();
}


double run_test(int N, int nthreads) {
    vector<double> A((size_t)N * N);
    vector<double> b(N);
    vector<double> c(N);
    
    parallel_init(A, b, N, nthreads);
    
    double time = parallel_matvec(A, b, c, N, nthreads);
    
    return time;
}


int main() {
    vector<int> sizes = {20000, 40000};
    vector<int> threads_list = {1, 2, 4, 7, 8, 16, 20, 40};
    int repeats = 3;  
    
    cout << fixed << setprecision(4);
    
    for (int N : sizes) {
        cout << "Размер матрицы: " << N << " x " << N << "\n";
        
        cout << "| Потоки | Tp | Sp |\n";
        
        double T1 = 0.0;
        
        for (int p : threads_list) {
            double total_time = 0.0;
            for (int r = 0; r < repeats; ++r) {
                total_time += run_test(N, p);
            }
            double avg_time = total_time / repeats;
            
            if (p == 1) {
                T1 = avg_time;
                cout << "|" << setw(6) << p << "|" 
                     << setw(14) << avg_time << "|" 
                     << setw(12) << "1.00 |\n";
            } else {
                double speedup = T1 / avg_time;
                cout << "|" << setw(6) << p << "|" 
                     << setw(14) << avg_time << "|" 
                     << setw(12) << setprecision(2) << speedup << "|\n";
            }
        }
    }
    
    return 0;
}

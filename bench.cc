#include <iostream>
#include <chrono>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <numaif.h>
#include <thread>
#include <cstdlib>
#include <unistd.h>

unsigned long num_pages = 256 * 1024;
int read_ratio = 100; 
int write_ratio = 0;
int time_lim = 5;
int access_pattern = 0; 
int thread_pin = 0;

struct Page {
    char data[4096]; 
};

void check_autonuma(){
    FILE* fptr = fopen("/proc/sys/kernel/numa_balancing", "r");
    char buf[2];
    char *ret = fgets(buf, sizeof(buf), fptr);
    printf("autonuma: %c\n", buf[0]);
    fclose(fptr);
}

int drop_caches() {
    return system("sudo sync; echo 1 | sudo tee /proc/sys/vm/drop_caches");
}

void benchmark(std::vector<Page> & pages){
    int cpu = sched_getcpu();
    printf("Benchmark on thread: %d\n", cpu);

    auto start = std::chrono::high_resolution_clock::now();
    auto end = start + std::chrono::seconds(time_lim);
    srand(time(0)); 

    unsigned long work_counter = 0;

    while(std::chrono::high_resolution_clock::now() < end) {
        if(access_pattern == 0) 
        {
            for (unsigned long i = 0; i < num_pages; i++) {
                
                if(i % 100 < read_ratio) {
                    volatile char readValue = pages[i].data[0];
                } else {
                    pages[i].data[0] = rand() % 127;
                }

                work_counter ++;
            }
        } 
        else 
        { 
            for (long i = 0; i < num_pages; i++) {
                long idx = rand() % num_pages;

                if(i % 100 < read_ratio) {
                    volatile char readValue = pages[idx].data[0];
                } else {
                    pages[idx].data[0] = rand() % 127;
                }
   
                work_counter ++;
            }
        }
    }

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start);
    std::cout << "Elapsed time: " << duration.count() << " milliseconds" << std::endl;
    std::cout << "Work counter: " << work_counter << " work/ms " << work_counter / duration.count() << std::endl;
}

int main(int argc, char* argv[]) {

    if(argc >= 2) { num_pages = atoi(argv[1]) * 256; }
    if(argc >= 3) { read_ratio = atoi(argv[2]); write_ratio = 100 - read_ratio; }
    if(argc >= 4) { time_lim = atoi(argv[3]); }
    if(argc >= 5) { access_pattern = atoi(argv[4]); }
    if(argc >= 6) { thread_pin = atoi(argv[5]); }

    check_autonuma();
    drop_caches();

    cpu_set_t  mask;
    CPU_ZERO(&mask);
    CPU_SET(0, &mask);
    int result = sched_setaffinity(0, sizeof(mask), &mask);

    std::vector<Page> pages(num_pages);
    pthread_t bench_thread;
    cpu_set_t cpuset;
    pthread_attr_t attr;


    int cpu = sched_getcpu();
    printf("Running on thread: %d\n", cpu);
    for (auto page: pages){
        volatile char q = page.data[0];
		asm volatile("" : : : "memory");
    }




    pthread_attr_init(&attr);
    CPU_ZERO(&cpuset);
    CPU_SET(thread_pin, &cpuset);
    pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset);

    pthread_create(&bench_thread, &attr, [](void* arg) -> void* {
        std::vector<Page>& pages = *reinterpret_cast<std::vector<Page>*>(arg);
        benchmark(pages);
        return nullptr;
    }, &pages);

    pthread_join(bench_thread, nullptr);

    return 0;
}

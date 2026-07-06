#ifndef PROCESS_MANAGEMENT_HPP
#define PROCESS_MANAGEMENT_HPP

#include "Task.hpp"
#include<queue>
#include<memory>
#include<semaphore.h>
#include <atomic>              // add this
#include <condition_variable>
class ProcessManagement{
    sem_t* itemsSemaphore;
    sem_t* emptySlotsSemaphore;

    public:
        ProcessManagement();
        ~ProcessManagement();
        bool submitToQueue(std::unique_ptr<Task> task);
        void executeTasks();
        void waitForCompletion(int totalTasks);

    private:
        struct SharedMemory{
            pthread_mutex_t queueLock;
            int size;
            char tasks[1000][256];
            int front;
            int rear;

            void printSharedMemory(){
                std::cout<<"Size: "<<size<<std::endl;
                std::cout<<"Front: "<<front<<std::endl;
                std::cout<<"Rear: "<<rear<<std::endl;
            }
        };

        SharedMemory* sharedMem;
        int shmFd;
        const char* SHM_NAME = "/my_queue";
        std::mutex queueLock;


};


#endif


//multithreading

// #ifndef PROCESS_MANAGEMENT_HPP
// #define PROCESS_MANAGEMENT_HPP

// #include "Task.hpp"
// #include <memory>
// #include <iostream>
// #include <mutex>
// #include <condition_variable>
// #include <atomic>

// class ProcessManagement {
// public:
//     ProcessManagement();
//     ~ProcessManagement();
//     bool submitToQueue(std::unique_ptr<Task> task);
//     void executeTasks();
//     void waitForCompletion(int totalTasks);

// private:
//     struct SharedMemory {
//         int size;
//         char tasks[1000][256];
//         int front;
//         int rear;

//         void printSharedMemory() {
//             std::cout << "Size: " << size << std::endl;
//             std::cout << "Front: " << front << std::endl;
//             std::cout << "Rear: " << rear << std::endl;
//         }
//     };

//     SharedMemory* sharedMem;
//     std::mutex queueLock; // Standard mutex handles thread synchronization perfectly

//     // Thread completion tracking tools
//     std::atomic<int> completedTasks{0};
//     std::condition_variable completionCV;
//     std::mutex completionMutex;
// };

// #endif
#include "ProcessManagement.hpp"
#include <iostream>
#include <cstring>
#include <sys/wait.h>
#include <unistd.h>
#include "../encryptDecrypt/Cryption.hpp"
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <semaphore.h>

ProcessManagement::ProcessManagement(){
    
    sem_unlink("/items_semaphore");
    sem_unlink("/empty_slots_semaphore");
    shm_unlink(SHM_NAME);

    itemsSemaphore = sem_open("/items_semaphore", O_CREAT | O_EXCL, 0666, 0);
    emptySlotsSemaphore = sem_open("/empty_slots_semaphore", O_CREAT | O_EXCL, 0666, 1000);

    shmFd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, 0666);
    ftruncate(shmFd, sizeof(SharedMemory));
    sharedMem = static_cast<SharedMemory *>(mmap(nullptr, sizeof(SharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED, shmFd, 0));
    
    sharedMem->front = 0;
    sharedMem->rear = 0;
    sharedMem->size = 0;

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&(sharedMem->queueLock), &attr);
    pthread_mutexattr_destroy(&attr);
}

bool ProcessManagement::submitToQueue(std::unique_ptr<Task> task){
    sem_wait(emptySlotsSemaphore);
    
    pthread_mutex_lock(&(sharedMem->queueLock));
    if(sharedMem->size >= 1000){
        pthread_mutex_unlock(&(sharedMem->queueLock));
        sem_post(emptySlotsSemaphore);
        return false;
    }

    strcpy(sharedMem->tasks[sharedMem->rear], task->toString().c_str());
    sharedMem->rear = (sharedMem->rear + 1) % 1000;
    sharedMem->size++;
    pthread_mutex_unlock(&(sharedMem->queueLock));
    
    sem_post(itemsSemaphore);
    
    int pid = fork();
    if(pid < 0) {
        
        sem_wait(itemsSemaphore); 
        pthread_mutex_lock(&(sharedMem->queueLock));
        sharedMem->rear = (sharedMem->rear - 1 + 1000) % 1000;
        sharedMem->size--;
        pthread_mutex_unlock(&(sharedMem->queueLock));
        sem_post(emptySlotsSemaphore);
        
        std::cerr << "OS limit hit: Fork failed!" << std::endl;
        return false;
    }
    else if(pid > 0) {
        std::cout << "Entering the parent process" << std::endl;
        return true; 
    } else {
        std::cout << "Entering the child process" << std::endl;
        executeTasks();
        std::cout << "Exiting the child process" << std::endl;
        exit(0);
    }
}

void ProcessManagement::executeTasks(){
    sem_wait(itemsSemaphore);
    
    pthread_mutex_lock(&(sharedMem->queueLock));
    char taskStr[256];
    strcpy(taskStr, sharedMem->tasks[sharedMem->front]);
    sharedMem->front = (sharedMem->front + 1) % 1000;
    sharedMem->size--;
    pthread_mutex_unlock(&(sharedMem->queueLock));
    
    sem_post(emptySlotsSemaphore);
    
    std::cout << "Executing child process" << std::endl;
    executeCryption(taskStr);
}

void ProcessManagement::waitForCompletion(int totalTasks) {
    
    int remaining = totalTasks;
    while (remaining > 0) {
        int status;
        pid_t pid = wait(&status);
        if (pid > 0) {
            remaining--;
        } else if (pid < 0) {
            
            break; 
        }
    }
}

ProcessManagement::~ProcessManagement(){
    pthread_mutex_destroy(&(sharedMem->queueLock)); 
    munmap(sharedMem, sizeof(SharedMemory));
    shm_unlink(SHM_NAME);
    sem_close(itemsSemaphore);
    sem_close(emptySlotsSemaphore);
    sem_unlink("/items_semaphore");
    sem_unlink("/empty_slots_semaphore");
}



//MULTITHREADING
// #include "ProcessManagement.hpp"
// #include <iostream>
// #include <cstring>
// #include <thread>
// #include "../encryptDecrypt/Cryption.hpp"

// ProcessManagement::ProcessManagement() {
    
//     sharedMem = new SharedMemory();
//     sharedMem->front = 0;
//     sharedMem->rear = 0;
//     sharedMem->size = 0;
// }

// bool ProcessManagement::submitToQueue(std::unique_ptr<Task> task) {
//     std::unique_lock<std::mutex> lock(queueLock);
    
   
//     if (sharedMem->size >= 1000) {
//         return false;
//     }

//     strcpy(sharedMem->tasks[sharedMem->rear], task->toString().c_str());
//     sharedMem->rear = (sharedMem->rear + 1) % 1000;
//     sharedMem->size++;
//     lock.unlock(); 

//     try {
//         std::thread thread_1(&ProcessManagement::executeTasks, this);
//         thread_1.detach();
//     } 
//     catch (const std::system_error& e) {
//         lock.lock();
//         sharedMem->rear = (sharedMem->rear - 1 + 1000) % 1000;
//         sharedMem->size--;
//         lock.unlock();
        
//         std::cerr << "OS limit hit: Thread creation failed!" << std::endl;
//         return false;
//     }

//     return true;
// }

// void ProcessManagement::executeTasks() {
//     std::unique_lock<std::mutex> lock(queueLock);
    
//     char taskStr[256];
//     strcpy(taskStr, sharedMem->tasks[sharedMem->front]);
//     sharedMem->front = (sharedMem->front + 1) % 1000;
//     sharedMem->size--;
//     lock.unlock();
    
//     std::cout << "Executing child thread" << std::endl;
//     executeCryption(taskStr);

//     std::unique_lock<std::mutex> compLock(completionMutex);
//     completedTasks.fetch_add(1);   
//     completionCV.notify_one();
// }

// void ProcessManagement::waitForCompletion(int totalTasks) {
//     std::unique_lock<std::mutex> lock(completionMutex);
//     completionCV.wait(lock, [this, totalTasks] {
//         return completedTasks.load() >= totalTasks;
//     });
// }

// ProcessManagement::~ProcessManagement() {
    
//     delete sharedMem;
// }
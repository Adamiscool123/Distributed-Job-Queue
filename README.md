💡 OverviewThe Distributed Job Queue System (DJQS) is a robust, low-latency, and horizontally scalable backend system designed to handle asynchronous task distribution across a network of Worker processes. Built from the ground up in pure C++, this project focuses on solving critical concurrency challenges inherent in distributed systems.The core challenge addressed is ensuring thread safety of the central job queue when multiple Clients and Workers attempt simultaneous read/write operations.⚙️ Architecture & Data FlowThe system adheres to the classic Client-Server-Worker pattern. The Server acts as a thread-safe central broker, decoupling the request from the time-consuming execution.ComponentRoleC++ Technologies DemonstratedServer (The Queue Boss)Manages the thread-safe std::queue<std::string>. Dispatches jobs and handles all connections.Sockets, Threads (std::thread), Mutexes (std::mutex)Client (The Request Maker)Sends task commands to the Server. Immediately receives an ACK and disconnects.SocketsWorker (The Processor)Continuously pulls work from the Server. Simulates task execution using sleep().Sockets, sleep()🔒 Concurrency: The Core C++ ChallengeThe most critical component is the Server's ability to manage concurrent access to the shared job queue. This is solved using a Mutex.The Problem (Race Condition)If Worker 1 and Worker 2 try to grab the first job from the queue at the exact same time, the underlying data structure can become corrupted, leading to a system crash or data loss.The Solution (Thread Safety)Every operation that modifies or accesses the shared std::queue is wrapped in a Mutex lock. This ensures that only one thread (Client or Worker) can interact with the queue at any given moment, guaranteeing the system's integrity and stability under heavy load.C++// Example C++ snippet showing Mutex usage
void Server::dispatchJob() {
    std::lock_guard<std::mutex> lock(queue_mutex_); // LOCK the queue
    if (!job_queue_.empty()) {
        std::string job = job_queue_.front();
        job_queue_.pop();
        // Send job to Worker
    }
    // Mutex is automatically UNLOCKED when 'lock' goes out of scope
}
🛠️ Installation & ExecutionPrerequisitesC++ compiler (GCC/G++) supporting C++11 or later.make utility (optional, but recommended).CompilationBash# Compile the three separate executables
g++ server.cpp -o server -std=c++17 -pthread
g++ client.cpp -o client -std=c++17 -pthread
g++ worker.cpp -o worker -std=c++17 -pthread
Run SequenceTo properly test the system, open three or more separate terminal windows.Terminal WindowCommandPurpose1. Server./serverStart the main listener.2. Worker A./workerStart the first processor.3. Worker B./workerStart the second processor (Demonstrates distribution!).4. Client./clientSend your instructions.Sample CommandsUse these commands in the Client terminal. The Workers will print a status message and then sleep() to simulate the work time.TASK: CALCULATE_FINANCE: 5s_delay
TASK: CLEANUP_LOGS: 2s_delay
TASK: SEND_NOTIFICATION: 1s_delay

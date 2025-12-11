#include <arpa/inet.h>
#include <iostream> // For I/O streams objects
#include <mutex> // To ensure that only one thread can access a shared resource at a time
#include <netinet/in.h>
#include <queue> // For push , pop , empty , and front
#include <string>
#include <sys/socket.h> // For sockets
#include <thread>       // For multitasking/multithreading
#include <unistd.h>     // Sleep function

int counter_id = 0;

struct Job {
  int id;

  // Job type
  std::string type;

  // Input data
  std::string payload;

  // What to do first
  int priority;

  // If crashes how many times to redo
  int retries;

  // How long it can max run for
  time_t deadline;
};

std::queue<Job> job_queue;
std::mutex queue_mutex;

/// Send data to worker
void send_to_worker(int worker_socket, Job job) {
  // Message to send to worker
  std::string message = "JOB" + std::to_string(job.id) + " " + job.type + " " +
                        job.payload + " " + std::to_string(job.priority) + " " +
                        std::to_string(job.retries) + " " +
                        std::to_string(job.deadline);

  // Send message to worker
  send(worker_socket, message.c_str(), message.length(), 0);
}

void handle_worker(int worker_socket) {
  // Make while loop to keep checking for jobs
  while (true) {
    // Lock the queue so no other thread can access it
    queue_mutex.lock();

    // Check if there are any jobs in the queue
    if (!job_queue.empty()) {
      // Get the job
      Job job = job_queue.front();

      // Remove the job from the queue
      job_queue.pop();

      // Unlock the queue
      queue_mutex.unlock();

      // Send the job to the worker
      send_to_worker(worker_socket, job);
    } else {
      // Unlock the queue
      queue_mutex.unlock();

      // Wait for a job
      sleep(1);
    }
  }
}

// Handle client connection
void handle_client(int clientSocket) {
  // What socket they are connected on
  std::cout << "Client connected on socket: " << clientSocket << std::endl;

  // While loop for receiving data
  while (true) {

    // Recieving data

    // Create buffer for data
    char buffer[10000] = {0};

    // Receive data from clientSocket

    // recv(socket, buffer, length of buffer, flag)
    // Flag: 0 - for standard blocking read
    // Make it a signed size tpye (ssize) so can see if there any any errors
    ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);

    // Check for errors
    if (bytesRead <= 0) {
      std::cout << "Client disconnected" << std::endl;
      break;
    }

    std::string message(buffer, bytesRead);

    // Print out message/data received from client
    std::cout << "Message from client: " << message << std::endl;

    if (message.substr(0, 6) == "SUBMIT") {
      if (message.substr(0, 23) == "SUBMIT TRANSCODE_VIDEO") {

        std::string keyword = "--payload=";

        std::string keyword1 = "--priority=";

        std::string keyword2 = "--retries=";

        std::string keyword3 = "--deadline=";

        size_t payload = message.find(keyword);

        size_t priority = message.find(keyword1);

        size_t retries = message.find(keyword2);

        size_t deadline = message.find(keyword3);

        size_t start_of_next_text1;

        size_t start_of_next_text2;

        size_t start_of_next_text3;

        if (retries != std::string::npos) {
          start_of_next_text2 = retries + keyword2.length();
        }

        if (deadline != std::string::npos) {
          start_of_next_text3 = deadline + keyword3.length();
        }

        if (priority != std::string::npos) {
          // 2. Calculate the starting position of the text *after* the keyword
          // pos is the start of "Age:", so we add the length of "Age:" to get
          // past it
          start_of_next_text1 = priority + keyword1.length();
        }

        if (payload != std::string::npos) {
          start_of_next_text1 = payload + keyword.length();
        }

        // Create job object
        Job job;

        // Assign values to job

        job.id = counter_id;
        job.type = "TRANSCODE_VIDEO";
        job.payload = message.substr(start_of_next_text1);

        // Stoi: String to Integer
        job.priority = std::stoi(message.substr(start_of_next_text1));
        job.retries = std::stoi(message.substr(start_of_next_text2));
        job.deadline = std::stoi(message.substr(start_of_next_text3));

        job_queue.push(job);

        std::string response =
            "Server: Job " + std::to_string(job.id) +
            " submitted successfully (TRANSCODE_VIDEO, priority: 5)";

        std::cout << response << std::endl;

        counter_id++;
      }
    } else if (message.substr(0, 10) == "JOB_STATUS") {
      std::cout << "Job status" << std::endl;
    } else if (message.substr(0, 10) == "JOB_CANCEL") {
      std::cout << "Job cancel" << std::endl;
    } else if (message.substr(0, 12) == "WORKER_DRAIN") {
      std::cout << "Worker drain" << std::endl;
    } else if (message.substr(0, 12) == "WORKER_SCALE") {
      std::cout << "Worker scale" << std::endl;
    } else if (message.substr(0, 15) == "QUEUE_REBALANCE") {
      std::cout << "Queue rebalance" << std::endl;
    } else if (message.substr(0, 11) == "QUEUE_PAUSE") {
      std::cout << "Queue pause" << std::endl;
    } else if (message.substr(0, 12) == "QUEUE_RESUME") {
      std::cout << "Queue resume" << std::endl;
    } else if (message.substr(0, 11) == "METRICS_GET") {
      std::cout << "Metrics get" << std::endl;
    } else if (message.substr(0, 9) == "TRACE_JOB") {
      std::cout << "Trace job" << std::endl;
    }
  }
  // Close passed client socket
  close(clientSocket);
}

void server(int port) {
  // Creating Socket:
  // AF_INET: Internet Protocol version 4 (IPv4)
  // SOCK_STREAM: Stream socket - Transmission Control Protocol (TCP)
  // 0: Telling OS you want stream socket from IPv4 and give me standard
  // protocol (TCP)
  int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

  // Specify the address
  sockaddr_in serverAddress;

  // Assign protocol to IPv4
  serverAddress.sin_family = AF_INET;

  // htons = Host to Netzork Short: Takes in port number and converts it to
  // standardized network format.
  serverAddress.sin_port = htons(port);

  // Final step for configuring serverAddress: Tells OS which IP address the
  // server should listen to

  // INADDR_ANY: Uses port 0.0.0.0 or in other words "Listen on all available
  // network interfaces"
  serverAddress.sin_addr.s_addr = INADDR_ANY;

  // System call that assigns server address with server socket
  // bind(serverSocket, serverAddress, serverAddress length)
  int value = bind(serverSocket, (struct sockaddr *)&serverAddress,
                   sizeof(serverAddress));

  if (value < 0) {
    std::cout << "Binding failed" << std::endl;
    return;
  }

  // Listen to the socket: 5 is the queue for amount of connections waiting to
  // be accepted
  listen(serverSocket, 5);

  std::cout << "Server Listening on port " << port << std::endl;

  while (true) {
    // Accept connection request:

    // accept(serverSocket, clientAddressPointer, clientAddressLengthPointer)
    // The nullptr are placeholders
    int newSocket = accept(serverSocket, nullptr, nullptr);

    if (newSocket < 0) {
      // Handle error
      continue;
    }

    // Check if client or worker
    char peek_buffer[100];

    // MSG_PEEK: Peeks at the data on the socket without removing it from the
    // buffer
    recv(newSocket, peek_buffer, sizeof(peek_buffer), MSG_PEEK);

    std::string msg(peek_buffer);

    if (msg.find("WORKER") == 0) {
      // Worker
      std::thread(handle_worker, newSocket).detach();
    } else {
      // Client
      std::thread(handle_client, newSocket).detach();
    }
  }

  // Close socket
  close(serverSocket);
}

int main(void) {
  int port = 8080;

  server(port);

  return 0;
}
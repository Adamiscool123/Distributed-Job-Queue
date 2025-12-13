#include <arpa/inet.h>
#include <ctime>
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
bool send_to_worker(int worker_socket, const Job &job) {
  // Message to send to worker
  std::string message = "JOB " + std::to_string(job.id) + " " + job.type + " " +
                        std::to_string(job.priority) + " " +
                        std::to_string(job.retries) + " " +
                        std::to_string(job.deadline) + " " + job.payload;

  // Send message to worker
  ssize_t sent = send(worker_socket, message.c_str(), message.length(), 0);
  if (sent < 0) {
    std::cout << "Failed to send job " << job.id << " to worker" << std::endl;
    return false;
  }
  if (static_cast<size_t>(sent) != message.length()) {
    std::cout << "Partial job send to worker for job " << job.id << std::endl;
    return false;
  }

  return true;
}

void handle_worker(int worker_socket) {
  // Make while loop to keep checking for jobs
  while (true) {
    std::unique_lock<std::mutex> lock(queue_mutex);

    // Check if there are any jobs in the queue
    if (job_queue.empty()) {
      lock.unlock();
      // Wait for a job
      sleep(1);
      continue;
    }

    // Get the job
    Job job = job_queue.front();

    // Remove the job from the queue
    job_queue.pop();

    lock.unlock();

    // Send the job to the worker
    if (!send_to_worker(worker_socket, job)) {
      break;
    }
  }

  close(worker_socket);
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

    // Convert buffer to string
    std::string message(buffer, bytesRead);

    // Print out message/data received from client
    std::cout << "Message from client: " << message << std::endl;

    // Handle different client commands

    if (message.substr(0, 6) == "SUBMIT") {
      // Check to see if message was found from index 0 - 6
      if (message.rfind("SUBMIT TRANSCODE_VIDEO", 0) == 0) {

        std::string keyword = "--payload=";

        std::string keyword1 = "--priority=";

        std::string keyword2 = "--retries=";

        std::string keyword3 = "--deadline=";

        // Find positions of keywords
        size_t payload_pos = message.find(keyword);
        size_t priority_pos = message.find(keyword1);
        size_t retries_pos = message.find(keyword2);
        size_t deadline_pos = message.find(keyword3);


        // Validate positions
        // Npos: not position
        if (payload_pos == std::string::npos ||
            priority_pos == std::string::npos ||
            retries_pos == std::string::npos ||
            deadline_pos == std::string::npos) {
          std::cout << "Invalid SUBMIT format: missing fields" << std::endl;
          continue;
        }


        // Lambda function to find where the whole instruction field e.g --payload=video is so in that example its o the ending
        // Use & to capture local variables by reference
        auto next_field_pos = [&](size_t current) {
          // Npos is infinity but it is a placeholder
          size_t next = std::string::npos;
          for (size_t pos : {payload_pos, priority_pos, retries_pos, deadline_pos}) {
            // Checks to see if position of the 4 instructions is bigger than currect position and position is smaller than next
            // Position
            // So if current = payload_pos then it checks the other 3 positions to see which is the smallest one after payload_pos 
            // Then returns it.
            // If there is no next field it returns npos
            if (pos > current && pos < next) {
              next = pos;
            }
          }
          return next;
        };

        // Lambda function to extract value between positions
        auto extract_value = [&](size_t start_pos, size_t key_len) {
          
          // Get start and end positions
          size_t start = start_pos + key_len;
          size_t end = next_field_pos(start_pos);
          
          // To see if there is no next field
          if (end == std::string::npos) {
            end = message.size();
          }
          if (end < start) {
            return std::string();
          }
          
          // If end (the end position of the instruction) is bigger than start (the start of the instruction position)
          // AND
          // Message at end - 1 (the last character of the instruction) is a space
          // Then decrement end by 1
          // This is to remove any trailing spaces from the value
          while (end > start && message[end - 1] == ' ') {
            --end;
          }
          // After finishing the while loop return the substring from start to end
          return message.substr(start, end - start);
        };

        std::string payload_value = extract_value(payload_pos, keyword.length());
        std::string priority_value =
            extract_value(priority_pos, keyword1.length());
        std::string retries_value = extract_value(retries_pos, keyword2.length());
        std::string deadline_value =
            extract_value(deadline_pos, keyword3.length());

        if (payload_value.empty() || priority_value.empty() ||
            retries_value.empty() || deadline_value.empty()) {
          std::cout << "Invalid SUBMIT format: empty fields" << std::endl;
          continue;
        }

        Job job;
        job.type = "TRANSCODE_VIDEO";
        job.payload = payload_value;

        try {
          job.priority = std::stoi(priority_value);
          job.retries = std::stoi(retries_value);
          job.deadline = std::stoi(deadline_value);
        } catch (const std::exception &e) {
          std::cout << "Invalid numeric field in SUBMIT: " << e.what()
                    << std::endl;
          continue;
        }

        {
          std::lock_guard<std::mutex> lock(queue_mutex);
          job.id = counter_id++;
          job_queue.push(job);
        }

        std::string response =
            "Server: Job " + std::to_string(job.id) +
            " submitted successfully (TRANSCODE_VIDEO, priority: 5)";

        std::cout << response << std::endl;
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
    char peek_buffer[100] = {0};

    // Ssize_t can hold negative and positive values whereas size_t only holds positive values
    // MSG_PEEK flag allows us to look at the incoming data without removing it from the queue
    ssize_t peeked =
        recv(newSocket, peek_buffer, sizeof(peek_buffer) - 1, MSG_PEEK);

    // If error while peeking then close socket and continue
    if (peeked <= 0) {
      close(newSocket);
      continue;
    }

    // Convert peeked data to string for easier handling
    std::string msg(peek_buffer, static_cast<size_t>(peeked));

    // Check if it's a worker or client based on the banner
    bool is_worker = (msg.find("Worker") == 0 || msg.find("WORKER") == 0);

    // Consume the peeked banner so it doesn't remain in the socket buffer
    ssize_t consumed = recv(newSocket, peek_buffer, static_cast<size_t>(peeked), 0);

    // If error while consuming then close socket and continue
    if (consumed <= 0) {
      close(newSocket);
      continue;
    }

    // Check if it's a worker or client based on the banner
    if (is_worker) {
      // Worker
      // Use threading to handle multiple workers
      // Detach thread to allow it to run independently in the background
      std::thread(handle_worker, newSocket).detach();
    } else {
      // Client
      // Use threading to handle multiple clients
      // Detach thread to allow it to run independently in the background
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

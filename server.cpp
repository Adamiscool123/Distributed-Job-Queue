#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream> // For I/O streams objects
#include <mutex> // To ensure that only one thread can access a shared resource at a time
#include <netinet/in.h>
#include <queue> // For push , pop , empty , and front
#include <sstream>
#include <string>
#include <sys/socket.h> // For sockets
#include <thread>       // For multitasking/multithreading
#include <unistd.h>     // Sleep function

// Need to remove
std::string checker;

int job_id;

// Split string into vector of strings
std::vector<std::string> split(const std::string &s, char delimiter) {
  // Make vector of strings
  std::vector<std::string> tokens;

  // Make string to store each token
  std::string token;

  // Allows you to take out parts of the string
  // Wraps the message in a input string stream
  std::istringstream tokenStream(s);

  // Get each token

  // Keeps reading tokenStream putting each of them into the token until it hits
  // the delimiter (a space)
  while (std::getline(tokenStream, token, delimiter)) {
    // Put in token to tokens vector
    tokens.push_back(token);
  }
  return tokens;
}

struct Job {
  int id;

  std::string status;

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

  // So can access from send_to_worker function
  int client_socket;
};

int counter_id = 0;

// Queue of Job objects
std::queue<Job> job_queue;

// Mutex for protecting access to the job queue
std::mutex queue_mutex;

/// Send data to worker
bool send_to_worker_submit(int worker_socket, const Job &job) {
  // Message to send to worker
  std::string message = "JOB " + std::to_string(job.id) + " " + job.type + " " +
                        std::to_string(job.priority) + " " +
                        std::to_string(job.retries) + " " +
                        std::to_string(job.deadline) + " " + job.payload;

  // Send message to worker
  // Ssize_t: Can be negative or positive
  // Message.c_str(): Convert string to const char*
  ssize_t sent = send(worker_socket, message.c_str(), message.length(), 0);

  // Check for errors
  if (sent < 0) {
    std::cout << "Failed to send job " << job.id << " to worker" << std::endl;
    return false;
  }

  // Check if entire message was sent
  if (static_cast<size_t>(sent) != message.length()) {
    std::cout << "Partial job send to worker for job " << job.id << std::endl;
    return false;
  }

  return true;
}

bool send_to_worker_status(int worker_socket, int id) {
  // Message to send to worker
  std::string message = "JOB_STATUS " + std::to_string(id);

  // Send message to worker
  // Ssize_t: Can be negative or positive
  ssize_t sent = send(worker_socket, message.c_str(), message.length(), 0);

  // Check for errors
  if (sent < 0) {
    std::cout << "Failed to send job status request for job " << id
              << " to worker" << std::endl;
    return false;
  }

  // Check if entire message was sent
  if (static_cast<size_t>(sent) != message.length()) {
    std::cout << "Partial job status request send to worker for job " << id
              << std::endl;
    return false;
  }

  return true;
}

bool send_to_worker_metrics(int worker_socket) {
  std::string message = "METRICS_GET";

  ssize_t sent = send(worker_socket, message.c_str(), message.length(), 0);

  if (sent < 0) {
    std::cout << "Failed to send METRICS_GET to worker" << std::endl;
    return false;
  }
  if (static_cast<size_t>(sent) != message.length()) {
    std::cout << "Partial METRICS_GET send to worker" << std::endl;
    return false;
  }

  return true;
}

void handle_worker(int worker_socket) {
  // Make while loop to keep checking for jobs
  while (true) {
    Job job;

    if (checker == "SUBMIT") {

      // Lock the queue while accessing it
      std::unique_lock<std::mutex> lock(queue_mutex);

      // Check if there are any jobs in the queue and if empty unlock and wait
      if (job_queue.empty()) {
        lock.unlock();
        // Wait for a job
        sleep(1);
        continue;
      }

      // Get the job - the one at the front of the queue

      job = job_queue.front();

      // Remove the job from the queue
      job_queue.pop();

      // Unlock the queue
      lock.unlock();

      // Send the job to the worker and check for errors
      if (!send_to_worker_submit(worker_socket, job)) {
        break;
      }

      char buffer[1000] = {0};

      ssize_t bytes = recv(worker_socket, buffer, sizeof(buffer) - 1, 0);

      if (bytes <= 0) {
        if (bytes == 0) {
          std::cout << "Worker closed connection" << std::endl;
        } else {
          std::cout << "Error receiving completion from worker" << std::endl;
        }
        break;
      }

      // Static cast to convert ssize_t to size_t so can only hold positive
      // values
      std::string message(buffer, static_cast<size_t>(bytes));

      if (message.find("Job " + std::to_string(job.id) + " completed:") == 0) {
        std::cout << "Message from worker: " << message << std::endl;

        int value =
            send(job.client_socket, message.c_str(), message.length(), 0);

        if (value < 0) {
          std::cout << "Failed to send job completion to client" << std::endl;
        } else {
          std::cout << "Server: Sent job completion to client " << job.id
                    << std::endl;
        }
      }
    } else if (checker == "JOB_STATUS") {

      int id = job_id;

      if (!send_to_worker_status(worker_socket, id)) {
        break;
      }

      checker = "";

      char buffer[1000] = {0};

      ssize_t bytes = recv(worker_socket, buffer, sizeof(buffer) - 1, 0);

      if (bytes <= 0) {
        if (bytes == 0) {
          std::cout << "Worker closed connection" << std::endl;
        } else {
          std::cout << "Error receiving completion from worker" << std::endl;
        }
        break;
      }

      // Capture worker message with job id and status
      std::string msg(buffer, static_cast<size_t>(bytes));

      std::cout << "Message from worker: " << msg << std::endl;

      int target_socket = job.client_socket;

      if (target_socket < 0) {
        std::cout << "Invalid client socket for job status response"
                  << std::endl;
        continue;
      }

      std::vector<std::string> token = split(msg, '\n');

      int value = send(target_socket, token[0].c_str(), token[0].length(), 0);

      if (value <= 0) {
        if (bytes == 0) {
          std::cout << "Client closed connection" << std::endl;
        } else {
          std::cout << "Error sending message to Client" << std::endl;
        }
        continue;
      }

      if (msg.rfind("Error", 0) == 0) {
        continue;
      }

      // Get completion message

      char buffer2[1000] = {0};

      ssize_t bytes2 = recv(worker_socket, buffer2, sizeof(buffer2) - 1, 0);

      if (bytes2 <= 0) {
        if (bytes == 0) {
          std::cout << "Worker closed connection" << std::endl;
        } else {
          std::cout << "Error receiving completion from worker" << std::endl;
        }
        break;
      }

      std::string msg2(buffer2, static_cast<size_t>(bytes2));

      std::cout << "Server: " << msg2 << std::endl;
    } else if (checker == "METRICS_GET") {

      if (!send_to_worker_metrics(worker_socket)) {
        break;
      }

      checker = "";

      char buffer[2000] = {0};

      ssize_t bytes = recv(worker_socket, buffer, sizeof(buffer) - 1, 0);

      if (bytes <= 0) {
        if (bytes == 0) {
          std::cout << "Worker closed connection" << std::endl;
        } else {
          std::cout << "Error receiving metrics from worker" << std::endl;
        }
        break;
      }

      std::string msg(buffer, static_cast<size_t>(bytes));

      std::cout << "Message from worker: " << msg << std::endl;

      int value = send(job.client_socket, msg.c_str(), msg.length(), 0);

      if (value <= 0) {
        if (bytes == 0) {
          std::cout << "Client closed connection" << std::endl;
        } else {
          std::cout << "Error sending metrics to Client" << std::endl;
        }
        continue;
      }
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

    std::vector<std::string> parts = split(message, ' ');

    Job job;

    job.client_socket = clientSocket;

    if (message.substr(0, 6) == "SUBMIT") {
      // Check to see if message was found from index 0 - 6
      if (parts[0] == "SUBMIT" && parts[1] == "TRANSCODE_VIDEO") {

        checker = "SUBMIT";

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

        // Lambda function to find where the whole instruction field e.g
        // --payload=video is so in that example its o the ending Use & to
        // capture local variables by reference
        auto next_field_pos = [&](size_t current) {
          // Npos is infinity but it is a placeholder
          size_t next = std::string::npos;
          for (size_t pos :
               {payload_pos, priority_pos, retries_pos, deadline_pos}) {
            // Checks to see if position of the 4 instructions is bigger than
            // currect position and position is smaller than next Position So if
            // current = payload_pos then it checks the other 3 positions to see
            // which is the smallest one after payload_pos Then returns it. If
            // there is no next field it returns npos
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

          // If end (the end position of the instruction) is bigger than start
          // (the start of the instruction position) AND Message at end - 1 (the
          // last character of the instruction) is a space Then decrement end by
          // 1 This is to remove any trailing spaces from the value
          while (end > start && message[end - 1] == ' ') {
            --end;
          }
          // After finishing the while loop return the substring from start to
          // end
          return message.substr(start, end - start);
        };

        // Extract values
        std::string payload_value =
            extract_value(payload_pos, keyword.length());
        std::string priority_value =
            extract_value(priority_pos, keyword1.length());
        std::string retries_value =
            extract_value(retries_pos, keyword2.length());
        std::string deadline_value =
            extract_value(deadline_pos, keyword3.length());

        // Validate extracted values
        if (payload_value.empty() || priority_value.empty() ||
            retries_value.empty() || deadline_value.empty()) {
          std::cout << "Invalid SUBMIT format: empty fields" << std::endl;
          continue;
        }

        // Create job object and assign values

        job.client_socket = clientSocket;

        job.type = "TRANSCODE_VIDEO";

        job.payload = payload_value;

        try {
          // Stoi = string to integer
          job.priority = std::stoi(priority_value);
          job.retries = std::stoi(retries_value);
          job.deadline = std::stoi(deadline_value);
        } catch (const std::exception &e) {
          std::cout << "Invalid numeric field in SUBMIT: " << e.what()
                    << std::endl;
          continue;
        }

        {
          // Lock the queue while adding job so no other thread can access it
          std::lock_guard<std::mutex> lock(queue_mutex);

          // Assign job ID
          job.id = counter_id++;

          // Add job to queue
          job_queue.push(job);
        }

        // Show confirmation message
        std::string response =
            "Server: Job " + std::to_string(job.id) +
            " submitted successfully (TRANSCODE_VIDEO, priority: 5)";

        std::cout << response << std::endl;
      } else {

        std::cout << "Unknown job type from client: " << parts[1] << std::endl;

        int value = send(clientSocket, "Unknown command", 15, 0);

        if (value < 0) {
          std::cout << "Failed to send unknown command message to client"
                    << std::endl;
        }
      }
    } else if (message.substr(0, 10) == "JOB_STATUS") {

      checker = "JOB_STATUS";

      job_id = std::stoi(parts[1]);
    } else if (message.substr(0, 11) == "METRICS_GET") {

      checker = "METRICS_GET";

    } else if (message.substr(0, 8) == "SHUTDOWN") {

      std::cout << "Shutdown command received from client. Closing connection."
                << std::endl;
      break;
    } else {

      std::cout << "Unknown command from client: " << message << std::endl;

      int value = send(job.client_socket, "Unknown command", 15, 0);

      if (value < 0) {
        std::cout << "Failed to send unknown command message to client"
                  << std::endl;
      }
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

  // htons = Host to Network Short: Takes in port number and converts it to
  // standardized network format so router can understand
  serverAddress.sin_port = htons(port);

  // Final step for configuring serverAddress: Tells OS which IP address the
  // server should listen to

  // INADDR_ANY: Uses port 0.0.0.0 or in other words "Listen on all available
  // network interfaces" so no need to specify actual IP address Basically can
  // take in connections from any IP address
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

    // Ssize_t can hold negative and positive values whereas size_t only holds
    // positive values MSG_PEEK flag allows us to look at the incoming data
    // without removing it from the queue
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
    ssize_t consumed =
        recv(newSocket, peek_buffer, static_cast<size_t>(peeked), 0);

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

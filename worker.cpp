#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream> // For I/O streams objects
#include <mutex> // To ensure that only one thread can access a shared resource at a time
#include <netinet/in.h>
#include <queue> // For push , pop , empty , and front
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/socket.h> // For sockets
#include <thread>       // For multitasking/multithreading
#include <unistd.h>     // Sleep function
#include <vector>

void clearScreen() {
// Check for Windows operating systems (32-bit or 64-bit)
#if defined(_WIN32) || defined(_WIN64)
  // If it is Windows, compile this line:
  std::system("cls");
#else
  // Otherwise (for Linux, macOS, etc.), compile this line:
  std::system("clear");
#endif
}

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

  std::string status = "PENDING";

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

std::vector<Job> jobs_list;

std::mutex jobs_mutex;

// Receive job from server and assign values to Job object then return it
bool receive_from_server(int socket, Job &job, std::vector<std::string> &parts,
                         std::string &type) {
  // Buffer to store the message
  char buffer[1000] = {0};
  // Get the message from the server
  ssize_t bytes = recv(socket, buffer, sizeof(buffer) - 1, 0);

  if (bytes <= 0) {
    if (bytes == 0) {
      std::cout << "Server closed connection" << std::endl;
    } else {
      std::cout << "Error receiving job from server" << std::endl;
    }
    return false;
  }

  // Convert the message to a string
  // Static_cast to convert ssize_t to size_t so can only hold positive values
  std::string message(buffer, static_cast<size_t>(bytes));

  parts = split(message, ' ');

  // Use != std::string::npos because if the result is 0 for message.find then
  // the if statement would think it's false
  if (message.find("JOB_STATUS") != std::string::npos) {
    type = "JOB_STATUS";

    return true;
  } else if (message.find("JOB") != std::string::npos) {
    if (message.find("TRANSCODE_VIDEO") != std::string::npos) {
      // Create input string stream to parse the message
      std::istringstream iss(message);
      // Temporary variable to hold the tag
      std::string tag;

      // Parse the message header and check for errors
      if (!(iss >> tag >> job.id >> job.type >> job.priority >> job.retries >>
            job.deadline)) {
        std::cout << "Malformed job header: " << message << std::endl;
        return false;
      }

      // Check if tag is correct
      if (tag != "JOB") {
        std::cout << "Unexpected message tag: " << tag << std::endl;
        return false;
      }

      std::string payload_rest;
      std::getline(iss, payload_rest);
      if (!payload_rest.empty() && payload_rest[0] == ' ') {
        payload_rest.erase(0, 1);
      }
      job.payload = payload_rest;

      type = "SUBMIT";

      return true;
    }
  } else if (message.find("METRICS_GET") != std::string::npos) {
    type = "METRICS_GET";

    return true;
  }

  return false;
}

void connection(int port) {
  // Create worker socket
  // AF_INET: IPv4
  // SOCK_STREAM: TCP
  int workerSocket = socket(AF_INET, SOCK_STREAM, 0);

  // Specify address to link to worker socket
  sockaddr_in serverAddress;

  // Specify Internet Protocol - IPv4
  serverAddress.sin_family = AF_INET;

  // Specify port
  // Use htons to make port from host byte order to network byte order
  serverAddress.sin_port = htons(port);

  // Convert string IP to binary form so that it can be used -
  // inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr) AND Check for
  // errors
  if (inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr) <= 0) {
    std::cout << "Invalid server address" << std::endl;
    return;
  }

  // Connect worker socket to server using its address
  int value = connect(workerSocket, (struct sockaddr *)&serverAddress,
                      sizeof(serverAddress));

  if (value < 0) {
    std::cout << "Connection failed" << std::endl;
    return;
  } else {
    std::cout << "Connection successful at localhost: 8080" << std::endl;
  }

  // Send identity to server
  const char *identity = "Worker";

  send(workerSocket, identity, strlen(identity), 0);

  std::cout << "Registered as worker, waiting for jobs " << std::endl;

  std::vector<std::string> parts;

  std::string type;

  while (true) {
    Job job;

    // Wait to receive job from server
    if (!receive_from_server(workerSocket, job, parts, type)) {
      std::cout << "Exiting worker loop due to receive error" << std::endl;
      job.status = "FAILED";
      break;
    }
    if (type == "SUBMIT") {

      // Get job received from server and check for errors

      job.status = "PENDING";

      // Start processing job

      std::cout << "Received job " << job.id << ": " << job.type << " "
                << job.payload << std::endl;

      if (job.type == "TRANSCODE_VIDEO") {

        std::cout << "Processing video: " << job.payload << std::endl;

        job.status = "IN_PROGRESS";

        std::cout << "Step 1/5: Demuxing video stream..." << std::endl;

        sleep(2);

        std::cout << "Step 2/5: Decoding 4K frames..." << std::endl;

        sleep(1);

        std::cout << "Step 3/5: Scaling to 1080p..." << std::endl;

        sleep(1);

        std::cout << "Step 4/5: Encoding with H.265..." << std::endl;

        sleep(3);

        std::cout << "Step 5/5: Muxing final file..." << std::endl;

        sleep(2);

        job.status = "COMPLETED";

        std::cout << "Job " << job.id << " completed: " << job.payload
                  << std::endl;

        // Send completion message back to server
        std::string message =
            "Job " + std::to_string(job.id) + " completed: " + job.payload;

        // Send message to server
        ssize_t sent = send(workerSocket, message.c_str(), message.length(), 0);

        // Check for errors like if full length of message was sent
        if (sent < 0 || static_cast<size_t>(sent) != message.length()) {
          std::cout << "Failed to send completion to server" << std::endl;
          break;
        }
      }
      std::lock_guard<std::mutex> lock(jobs_mutex);
      jobs_list.push_back(job);
    } else if (type == "JOB_STATUS") {

      // Handle JOB_STATUS request
      std::cout << "Received JOB_STATUS request from server" << std::endl;

      sleep(1); // Simulate processing time

      std::cout << "Handling JOB_STATUS request" << std::endl;

      sleep(2); // Simulate processing time

      bool found = false;

      for (int i = 0; i < jobs_list.size(); i++) {
        if (jobs_list[i].id == std::stoi(parts[1])) {
          found = true;

          std::cout << "Found job ID: " << jobs_list[i].id
                    << " with status: " << jobs_list[i].status << std::endl;

          std::string status_message = "Job " +
                                       std::to_string(jobs_list[i].id) +
                                       " status: " + jobs_list[i].status;

          // Send status message back to server
          ssize_t sent = send(workerSocket, status_message.c_str(),
                              status_message.length(), 0);

          // Check for errors
          if (sent < 0 ||
              static_cast<size_t>(sent) != status_message.length()) {
            std::cout << "Failed to send job status to server" << std::endl;
            break;
          }
          break;
        }
      }

      if (!found) {
        std::cout << "Job ID: " << parts[1] << " not found" << std::endl;

        std::string message = "Error Job " + parts[1] + " not found";

        // Send not found message back to server
        ssize_t sent = send(workerSocket, message.c_str(), message.length(), 0);

        if (sent < 0 || static_cast<size_t>(sent) != message.length()) {
          std::cout << "Failed to send job not found message to server"
                    << std::endl;
          break;
        }
        continue;
      }

      std::string message = "Job " + std::to_string(job.id) + " completed\n";

      // Send message to server
      ssize_t final_message =
          send(workerSocket, message.c_str(), message.length(), 0);

      if (final_message < 0 ||
          static_cast<size_t>(final_message) != message.length()) {
        std::cout << "Failed to send completion to server" << std::endl;
        break;
      }
    } else if (type == "METRICS_GET") {

      // Handle METRICS_GET request
      std::cout << "Received METRICS_GET request from server" << std::endl;

      sleep(1); // Simulate processing time

      std::cout << "Handling METRICS_GET request" << std::endl;

      sleep(2); // Simulate processing time

      int total_jobs = jobs_list.size();
      int completed_jobs = 0;
      int pending_jobs = 0;
      int in_progress_jobs = 0;
      int failed_jobs = 0;

      for (int i = 0; i < jobs_list.size(); i++) {
        if (jobs_list[i].status == "COMPLETED") {
          completed_jobs++;
        } else if (jobs_list[i].status == "PENDING") {
          pending_jobs++;
        } else if (jobs_list[i].status == "IN_PROGRESS") {
          in_progress_jobs++;
        } else if (jobs_list[i].status == "FAILED") {
          failed_jobs++;
        }
      }

      std::string metrics_message =
          std::string("\nMetrics") + std::string("\nTotal Jobs: ") +
          std::to_string(total_jobs) + std::string("\nCompleted: ") +
          std::to_string(completed_jobs) + std::string("\nIn Progress: ") +
          std::to_string(in_progress_jobs) + std::string("\nPending: ") +
          std::to_string(pending_jobs) + std::string("\nFailed: ") +
          std::to_string(failed_jobs);

      // Send metrics message back to server
      ssize_t sent = send(workerSocket, metrics_message.c_str(),
                          metrics_message.length(), 0);

      // Check for errors
      if (sent < 0 || static_cast<size_t>(sent) != metrics_message.length()) {

        std::cout << "Failed to send metrics to server" << std::endl;
        break;
      }

      std::cout << "Sent metrics to server" << std::endl;
    }
  }
  // End of worker loop - close socket
  close(workerSocket);
}

int main(void) {
  int port = 8080;

  connection(port);

  return 0;
}

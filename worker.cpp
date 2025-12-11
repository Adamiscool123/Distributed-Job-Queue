#include <arpa/inet.h>
#include <cstring>
#include <iostream> // For I/O streams objects
#include <mutex> // To ensure that only one thread can access a shared resource at a time
#include <netinet/in.h>
#include <queue> // For push , pop , empty , and front
#include <sstream>
#include <string>
#include <sys/socket.h> // For sockets
#include <thread>       // For multitasking/multithreading
#include <unistd.h>     // Sleep function
#include <vector>

// Split string into vector of strings
std::vector<std::string> split(const std::string &s, char delimiter) {
  // Make vector of strings
  std::vector<std::string> tokens;

  // Make string to store each token
  std::string token;

  // Allows you to take out parts of the string
  std::istringstream tokenStream(s);

  // Get each token

  // Keeps reading tokenStream putting each of them into the token until it hits
  // the delimiter (a space)
  while (std::getline(tokenStream, token, delimiter)) {
    tokens.push_back(token);
  }
  return tokens;
}

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

// Receive job from server and assign values to Job object then return it
Job receive_from_server(int socket) {
  // Buffer to store the message
  char buffer[1000] = {0};
  // Get the message from the server
  int bytes = recv(socket, buffer, sizeof(buffer) - 1, 0);

  // Convert the message to a string
  std::string message(buffer);

  std::vector<std::string> parts = split(message, ' ');

  // Create Job objects
  Job job;

  // Assign values to job

  // Stoi: String to Integer
  job.id = std::stoi(parts[1]);

  job.type = parts[2];

  job.payload = parts[3];

  job.priority = std::stoi(parts[4]);

  job.retries = std::stoi(parts[5]);

  job.deadline = std::stoi(parts[6]);

  return job;
}

void connection(int port) {
  // Create worker socket
  int workerSocket = socket(AF_INET, SOCK_STREAM, 0);

  // Specify address
  sockaddr_in serverAddress;

  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(port);
  serverAddress.sin_addr.s_addr = INADDR_ANY;

  int value = connect(workerSocket, (struct sockaddr *)&serverAddress,
                      sizeof(serverAddress));

  if (value < 0) {
    std::cout << "Connection failed" << std::endl;
    return;
  } else {
    std::cout << "Connection successful at localhost: 8080" << std::endl;
  }

  const char *identity = "Worker";

  send(workerSocket, identity, strlen(identity), 0);

  while (true) {
    // Get job received from server
    Job job = receive_from_server(workerSocket);

    std::cout << "Registered as worker, waiting for jobs " << std::endl;

    std::cout << "Received job " << job.id << ": " << job.type << " "
              << job.payload << std::endl;

    if (job.type == "TRANSCODE_VIDEO") {

      std::cout << "Processing video: " << job.payload << std::endl;

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

      std::cout << "Job " << job.id << " completed: " << job.payload
                << std::endl;

      std::string message =
          "Job " + std::to_string(job.id) + " completed: " + job.payload;
      send(workerSocket, message.c_str(), message.length(), 0);
    }
  }
}

int main(void) {
  int port = 8080;

  connection(port);

  return 0;
}
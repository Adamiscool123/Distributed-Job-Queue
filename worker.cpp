#include <arpa/inet.h>
#include <ctime>
#include <cstring>
#include <stdexcept>
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
bool receive_from_server(int socket, Job &job) {
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
  std::string message(buffer, static_cast<size_t>(bytes));

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

  //Check if tag is correct
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

  return true;
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
  serverAddress.sin_port = htons(port);

  // Convert string IP to binary form so that it can be used - inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr)
  // AND
  // Check for errors
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

  while (true) {
    // Get job received from server and check for errors
    Job job;
    
    // Wait to receive job from server
    if (!receive_from_server(workerSocket, job)) {
      std::cout << "Exiting worker loop due to receive error" << std::endl;
      break;
    }

    // Start processing job
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
  }
  // End of worker loop - close socket
  close(workerSocket);
}

int main(void) {
  int port = 8080;

  connection(port);

  return 0;
}

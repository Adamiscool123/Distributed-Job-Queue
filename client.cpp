#include <arpa/inet.h>
#include <cstring>
#include <iostream> // For I/O streams objects
#include <mutex> // To ensure that only one thread can access a shared resource at a time
#include <netinet/in.h>
#include <queue> // For push , pop , empty , and front
#include <string>
#include <sys/socket.h> // For sockets
#include <thread>       // For multitasking/multithreading
#include <unistd.h>     // Sleep function

void client(int port) {
  // Create client socket
  int clientSocket = socket(AF_INET, SOCK_STREAM, 0);

  // Specifie address:

  // Make sock address object
  sockaddr_in serverAddress;

  // Specifiy Internet Protocol - IPv4
  serverAddress.sin_family = AF_INET;

  // Specifiy port - 8080
  // Htons: Host to Network Short - Takes in port number then converts from host byte order to network byte order
  serverAddress.sin_port = htons(port);

  // Connect to localhost
  // Inet_pton: Convert the IP address from text to binary form so that it can be used by the router
  if (inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr) <= 0) {
    std::cout << "Invalid address" << std::endl;
    return;
  }

  // Connect client socket to server using its address
  int value = connect(clientSocket, (struct sockaddr *)&serverAddress,
                      sizeof(serverAddress));

  if (value < 0) {
    std::cout << "Connection failed" << std::endl;
    return;
  } else {
    std::cout << "Connection successful" << std::endl;
  }

  // Send data
  const char *identity = "Client";

  send(clientSocket, identity, strlen(identity), 0);

  while (true) {

    // Message:

    std::string message;

    std::cout << "Instruction: ";

    std::getline(std::cin, message);

    // Convert message back to char in order to send message to server
    const char *result_message = message.c_str();

    // Send message: clientsocket, message, length of message.
    int value = send(clientSocket, result_message, strlen(result_message), 0);

    if (value < 0) {
      std::cout << "Send failed" << std::endl;
      break;
    }

    char buffer[1000] = {0};

    // ssize_t can only hold positive or negative values whereas size_t only holds positive values
    // Receive data from server
    ssize_t bytes = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

    // Check for errors
    if(bytes <=0){
      if(bytes == 0){
        std::cout << "Server closed connection" << std::endl;
      }
      else{
        std::cout << "Error receiving from server" << std::endl;
      }
      break;
    }

    // Turn to string
    std::string completion_message(buffer, static_cast<size_t>(bytes));

    // Send message to client
    std::cout << "Message from server: " << completion_message << std::endl;
  }
  close(clientSocket);
}

int main(void) {
  int port = 8080;

  client(port);

  return 0;
}
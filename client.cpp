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
  serverAddress.sin_port = htons(port);

  // Listen on all available network interfaces
  serverAddress.sin_addr.s_addr = INADDR_ANY;

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
    send(clientSocket, result_message, strlen(result_message), 0);
  }
}

int main(void) {
  int port = 8080;

  client(port);

  return 0;
}
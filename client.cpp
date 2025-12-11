#include <iostream> // For I/O streams objects
#include <thread> // For multitasking/multithreading
#include <mutex> // To ensure that only one thread can access a shared resource at a time
#include <queue> // For push , pop , empty , and front
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h> // For sockets
#include <unistd.h> // Sleep function
#include <cstring>

void client(int port){
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

    // Send data

    while(true){
        // Connect client socket to server using its address
        connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
        
        // Message:

        char* message = new char[500];

        std::cout << "Instruction: ";

        std::cin >> message;

        // Send message: clientsocket, message, length of message.
        send(clientSocket, message, strlen(message), 0);
    }
}


int main(void){
    int port = 8080;

    client(port);

    return 0;
}
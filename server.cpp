#include <iostream> // For I/O streams objects
#include <thread> // For multitasking/multithreading
#include <mutex> // To ensure that only one thread can access a shared resource at a time
#include <queue> // For push , pop , empty , and front
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h> // For sockets
#include <unistd.h> // Sleep function

// Handle client connection
void handle_client(int clientSocket){
    // What socket they are connected on
    std::cout << "Client connected on socket: " << clientSocket << std::endl;

    // While loop for receiving data
    while(true){

        // Recieving data

        // Create buffer for data
        char buffer[1000] = { 0 };

        // Receive data from clientSocket
        
        // recv(socket, buffer, length of buffer, flag)
        // Flag: 0 - for standard blocking read
        // Make it a signed size tpye (ssize) so can see if there any any errors
        ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);

        // Check for errors
        if(bytesRead <= 0){
            std::cout << "Client disconnected" << std::endl;
            break;
        }

        // Print out message/data received from client
        std::cout << "Message from client: " << buffer << std::endl;

    }
    // Close passed client socket

    close(clientSocket);
}

void server(int port){
    // Creating Socket:
    // AF_INET: Internet Protocol version 4 (IPv4)
    // SOCK_STREAM: Stream socket - Transmission Control Protocol (TCP)
    // 0: Telling OS you want stream socket from IPv4 and give me standard protocol (TCP)
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    // Specify the address
    sockaddr_in serverAddress;
    
    // Assign protocol to IPv4
    serverAddress.sin_family = AF_INET;
    
    // htons = Host to Netzork Short: Takes in port number and converts it to standardized network format.
    serverAddress.sin_port = htons(port);
    
    // Final step for configuring serverAddress: Tells OS which IP address the server should listen to
    
    // INADDR_ANY: Uses port 0.0.0.0 or in other words "Listen on all available network interfaces"
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    
    // System call that assigns server address with server socket
    // bind(serverSocket, serverAddress, serverAddress length)
    bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

    // Listen to the socket: 5 is the queue for amount of connections waiting to be accepted
    listen(serverSocket, 5);

    std::cout << "Server Listening on port " << port << std::endl;

    while(true){
        // Accept connection request: 
        
        // accept(serverSocket, clientAddressPointer, clientAddressLengthPointer)
        // The nullptr are placeholders
        int clientSocket = accept(serverSocket, nullptr, nullptr);

        if(clientSocket < 0){
            // Handle error
            continue;
        }

        std::thread client_thread(handle_client, clientSocket);

        client_thread.detach();
    }

    // Close socket
    close(serverSocket);
}


int main(void){
    int port = 8080;

    server(port);

    return 0;
}
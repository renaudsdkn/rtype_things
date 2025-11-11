/*
 *
 *
 */


#include "../include/client/chatClient.hpp"

 ChatClient::ChatClient(const std::string& ipAddress, int port, const std::string& nickname)
     : _nickname(nickname), _connected(false), _inputLength(0) {

     memset(_inputBuffer, 0, sizeof(_inputBuffer));

     _clientFD = socket(AF_INET, SOCK_STREAM, 0);
     if (_clientFD < 0) {
         throw ChatErrors("Failed to create socket");
     }

     struct sockaddr_in server_addr;
     memset(&server_addr, 0, sizeof(server_addr));
     server_addr.sin_family = AF_INET;
     server_addr.sin_port = htons(port);

     // Convert IP address from string to binary
     if (inet_pton(AF_INET, ipAddress.c_str(), &server_addr.sin_addr) <= 0) {
         close(_clientFD);
         throw ChatErrors("Invalid IP address: " + ipAddress);
     }

     if (connect(_clientFD, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
         close(_clientFD);
         throw ChatErrors("Failed to connect to server at " + ipAddress + ":" + std::to_string(port));
     }

     // Set socket to non-blocking
     int flags = fcntl(_clientFD, F_GETFL, 0);
     fcntl(_clientFD, F_SETFL, flags | O_NONBLOCK);

     // Send nickname
     write(_clientFD, nickname.c_str(), nickname.size());
     _connected = true;

     std::cout << "Connected to server as " << nickname << std::endl;
 }

 void ChatClient::receiveMessages() {
     if (!_connected) return;

     char buffer[2048];
     int size = read(_clientFD, buffer, sizeof(buffer) - 1);

     if (size > 0) {
         buffer[size] = '\0';
         std::string msg(buffer);

         // Parse message format "username: message"
         size_t colonPos = msg.find(':');
         if (colonPos != std::string::npos) {
             std::string user = msg.substr(0, colonPos);
             std::string text = msg.substr(colonPos + 2); // Skip ": "
             _messages.push_back({user, text});
         } else {
             // System message
             _messages.push_back({"System", msg});
         }
     } else if (size == 0) {
         _connected = false;
         _messages.push_back({"System", "Disconnected from server\n"});
     }
 }

 void ChatClient::sendMessage() {
     if (!_connected || !_inputLength) return;
     write(_clientFD, _inputBuffer, _inputLength);
 }

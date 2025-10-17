#include <iostream>
#include "../include/server/server.hpp"

int main(int argc, char* argv[]) {
    try {
        unsigned short port = 1234; // valeur par défaut

        if (argc > 1) {
            port = static_cast<unsigned short>(std::stoi(argv[1]));
        }

        asio::io_context io_context;
        UdpServer server(io_context, port);
        io_context.run();
    } catch (const std::exception& e) {
        std::cerr << "[FATAL] Exception: " << e.what() << std::endl;
        return 84;
    }
    return 0;
}

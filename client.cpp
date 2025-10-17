#include <asio.hpp>
#include <iostream>
#include <cstring>

#include "../include/protocol/serializer.hpp"


#include <iostream>
#include <thread>

using asio::ip::udp;
using namespace Protocol;
using namespace ProtocolData;

int main() {
    try {
        asio::io_context io;
        udp::socket socket(io);
        socket.open(udp::v4());

        udp::endpoint server_endpoint(asio::ip::make_address("127.0.0.1"), 12345);

        std::cout << "[CLIENT] Test client started — sending messages every second...\n";

        int counter = 0;
        while (true) {
            switch (counter % 5) {
                case 0: {
                    std::cout << "[CLIENT] Sending CONNECT\n";
                    ConnectMessage msg;
                    auto data = msg.serialize();
                    socket.send_to(asio::buffer(data), server_endpoint);
                    break;
                }

                case 1: {
                    std::cout << "[CLIENT] Sending INPUT\n";
                    PlayerInput input{ 1, 1, 0, 1, 0, 0 }; // up + right
                    PlayerInputMessage msg(input);
                    auto data = msg.serialize();
                    socket.send_to(asio::buffer(data), server_endpoint);
                    break;
                }

                case 2: {
                    std::cout << "[CLIENT] Sending SPAWN_ENTITY\n";
                    entity_state e{ 101, 1 };
                    SpawnEntity spawn{ e };
                    PacketHeader header{
                        htons(sizeof(PacketHeader) + sizeof(SpawnEntity)),
                        static_cast<uint8_t>(MessageType::SPAWN_ENTITY)
                    };
                    std::vector<uint8_t> buffer(sizeof(header) + sizeof(spawn));
                    std::memcpy(buffer.data(), &header, sizeof(header));
                    std::memcpy(buffer.data() + sizeof(header), &spawn, sizeof(spawn));
                    socket.send_to(asio::buffer(buffer), server_endpoint);
                    break;
                }

                case 3: {
                    std::cout << "[CLIENT] Sending MOVE_ENTITY\n";
                    MoveEntity move{ 101, 50.0f, 75.0f };
                    PacketHeader header{
                        htons(sizeof(PacketHeader) + sizeof(MoveEntity)),
                        static_cast<uint8_t>(MessageType::MOVE_ENTITY)
                    };
                    std::vector<uint8_t> buffer(sizeof(header) + sizeof(move));
                    std::memcpy(buffer.data(), &header, sizeof(header));
                    std::memcpy(buffer.data() + sizeof(header), &move, sizeof(move));
                    socket.send_to(asio::buffer(buffer), server_endpoint);
                    break;
                }

                case 4: {
                    std::cout << "[CLIENT] Sending DESTROY_ENTITY\n";
                    DestroyEntity destroy{ 101 };
                    PacketHeader header{
                        htons(sizeof(PacketHeader) + sizeof(DestroyEntity)),
                        static_cast<uint8_t>(MessageType::DESTROY_ENTITY)
                    };
                    std::vector<uint8_t> buffer(sizeof(header) + sizeof(destroy));
                    std::memcpy(buffer.data(), &header, sizeof(header));
                    std::memcpy(buffer.data() + sizeof(header), &destroy, sizeof(destroy));
                    socket.send_to(asio::buffer(buffer), server_endpoint);
                    break;
                }
            }

            counter++;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

    } catch (const std::exception& e) {
        std::cerr << "[CLIENT ERROR] " << e.what() << std::endl;
    }

    return 0;
}


/*int main() {
    asio::io_context io;
    asio::ip::udp::socket socket(io);
    socket.open(asio::ip::udp::v4());

    asio::ip::udp::endpoint server(asio::ip::address::from_string("127.0.0.1"), 1234);
    ProtocolData::PacketHeader header;
header.size = htons(3);  // Convertit 3 → [0x00][0x03] Big Endian
header.type = 0x01;
 

    std::vector<uint8_t> data(sizeof(header));
    std::memcpy(data.data(), &header, sizeof(header));
    
    socket.send_to(asio::buffer(data), server);
    std::cout << "CONNECT packet sent!" << std::endl;

    return 0;
}*/

void send_input(asio::ip::udp::socket& socket, 
                const asio::ip::udp::endpoint& server,
                uint32_t player_id,
                bool up, bool down, bool right, bool left, bool shoot)
{
    // Créer PlayerInput
    ProtocolData::PlayerInput input;
    input.playerId = htonl(player_id);
    input.up = up ? 1 : 0;
    input.down = down ? 1 : 0;
    input.right = right ? 1 : 0;
    input.left = left ? 1 : 0;
    input.shoot = shoot ? 1 : 0;
    
    // Créer header
    ProtocolData::PacketHeader header;
    header.size = htons(sizeof(ProtocolData::PacketHeader) + sizeof(ProtocolData::PlayerInput));
    header.type = static_cast<uint8_t>(ProtocolData::MessageType::INPUT);
    
    // Assembler
    std::vector<uint8_t> data(sizeof(header) + sizeof(input));
    std::memcpy(data.data(), &header, sizeof(header));
    std::memcpy(data.data() + sizeof(header), &input, sizeof(input));
    
    // Envoyer
    socket.send_to(asio::buffer(data), server);
}

// Usage
/*int main() {
    asio::io_context io;
    asio::ip::udp::socket socket(io);
    socket.open(asio::ip::udp::v4());
    asio::ip::udp::endpoint server(asio::ip::address::from_string("127.0.0.1"), 1234);
    
    // Envoyer différents inputs
    send_input(socket, server, 1, true, false, true, false, true);   // Haut + Droite + Tir
    send_input(socket, server, 1, false, false, false, true, false); // Gauche
    send_input(socket, server, 1, false, true, false, false, true);  // Bas + Tir
    
    std::cout << "Multiple INPUT packets sent!" << std::endl;
    return 0;
}
*/
/*int main() {
    asio::io_context io;
    asio::ip::udp::socket socket(io);
    socket.open(asio::ip::udp::v4());
    asio::ip::udp::endpoint server(asio::ip::address::from_string("127.0.0.1"), 1234);
    
    std::cout << "Controls: Z=up, S=down, D=right, Q=left, SPACE=shoot, X=quit" << std::endl;
    
    while (true) {
        std::cout << "Enter command: ";
        char cmd;
        std::cin >> cmd;
        
        if (cmd == 'x' || cmd == 'X') break;
        
        bool up = (cmd == 'z' || cmd == 'Z');
        bool down = (cmd == 's' || cmd == 'S');
        bool right = (cmd == 'd' || cmd == 'D');
        bool left = (cmd == 'q' || cmd == 'Q');
        bool shoot = (cmd == ' ');
        
        send_input(socket, server, 1, up, down, right, left, shoot);
        std::cout << "Input sent!" << std::endl;
    }
    
    return 0;
}*/

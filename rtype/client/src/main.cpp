#include <iostream>
#include <stdexcept>
#include "../include/client/GameClient.hpp"

int main(void)
{
    try {
        GameClient client(800, 450, "R-Type Client ECS Intégration");
        client.run();
        
    } catch (const std::exception& e) {
        std::cerr << "Couldn't load the game : " << e.what() << std::endl;
        return 84;
    }
    return 0;
}

/*#include "../include/client/client.hpp"
#include <iostream>

int main() {
    RTypeClient client("127.0.0.1", 1234);
    client.start();

    std::cout << "Commande (w/a/s/d = input, p=ping, x=disconnect, q=quit): ";
    char cmd;
    ProtocolData::PlayerInput input{0,0,0,0,0,0};

    while (std::cin >> cmd) {
        if (cmd == 'q') break;
        if (cmd == 'p') client.send_ping();
        if (cmd == 'x') client.send_disconnect();
        if (cmd == 'w') { input.up = 1; client.send_input(input); input.up = 0; }
        if (cmd == 's') { input.down = 1; client.send_input(input); input.down = 0; }
        if (cmd == 'a') { input.left = 1; client.send_input(input); input.left = 0; }
        if (cmd == 'd') { input.right = 1; client.send_input(input); input.right = 0; }
    }

    client.stop();
    return 0;
}*/
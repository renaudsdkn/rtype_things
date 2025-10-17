/*
** EPITECH PROJECT, 2025
** R-Type
** File description:
** UdpClient.cpp
*/

#include "../include/client/UdpClient.hpp" // Ajustez le chemin d'accès au header
#include <cstring>

namespace Client
{

    UdpClient::UdpClient(asio::io_context& io_context, const std::string& server_host, unsigned short server_port)
        : m_socket(io_context)
    {
        // 1. Résoudre le nom d'hôte ou l'IP
        asio::ip::udp::resolver resolver(io_context);
        asio::ip::udp::resolver::results_type endpoints =
            resolver.resolve(asio::ip::udp::v4(), server_host, std::to_string(server_port));

        if (endpoints.empty()) {
            throw std::runtime_error("Impossible de résoudre l'adresse du serveur.");
        }
        
        // 2. Stocker le point de terminaison du serveur (le premier dans la liste)
        m_server_endpoint = *endpoints.begin();

        // 3. Ouvrir le socket
        m_socket.open(asio::ip::udp::v4());
        
        std::cout << "[NET] Client started, server endpoint set to "
                  << m_server_endpoint.address().to_string() << ":"
                  << m_server_endpoint.port() << std::endl;
    }

    void UdpClient::start_receive()
    {
        // 1. Démarrer une opération de réception asynchrone
        m_socket.async_receive_from(
            asio::buffer(m_recv_buffer),
            m_remote_endpoint,
            [this](const asio::error_code& error, std::size_t bytes_recvd)
            {
                if (!error) {
                    process_datagram(bytes_recvd);
                } else if (error != asio::error::operation_aborted) {
                    std::cerr << "[NET ERROR] Receive failed: " << error.message() << std::endl;
                }
                
                // 2. Redémarrer l'opération pour la prochaine réception
                if (m_socket.is_open()) {
                    start_receive();
                }
            });
    }

    void UdpClient::process_datagram(std::size_t bytes_recvd)
    {
        // 1. Vérifier si le message provient bien du serveur attendu (sécurité de base)
        if (m_remote_endpoint != m_server_endpoint) {
            std::cerr << "[NET WARN] Message from unknown sender: " 
                      << m_remote_endpoint.address().to_string() << std::endl;
            return;
        }

        // 2. Copier les données reçues dans un vecteur pour la désérialisation
        std::vector<uint8_t> buffer(bytes_recvd);
        std::memcpy(buffer.data(), m_recv_buffer.data(), bytes_recvd);

        try {
            // 3. Désérialiser le message
            auto message = Protocol::MessageFactory::deserialize(buffer);
            
            // 4. Mettre en file d'attente pour le thread principal du jeu
            enqueue_message({std::move(message), m_remote_endpoint});

        } catch (const std::runtime_error& e) {
            std::cerr << "[PROTO ERROR] Failed to deserialize message: " << e.what() << std::endl;
        }
    }

    void UdpClient::send_message(const std::vector<uint8_t>& message_to_send)
    {
        // Envoi asynchrone du message au serveur
        m_socket.async_send_to(
            asio::buffer(message_to_send),
            m_server_endpoint,
            [](auto ec, auto)
            {
                if (ec)
                    std::cerr << "[NET ERROR] Send failed: " << ec.message() << std::endl;
            });
    }

    void UdpClient::enqueue_message(NetworkMessage&& msg)
    {
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        m_message_queue.push(std::move(msg));
    }

    std::optional<NetworkMessage> UdpClient::poll_message()
    {
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        if (m_message_queue.empty()) {
            return std::nullopt;
        }
        
        NetworkMessage msg = std::move(m_message_queue.front());
        m_message_queue.pop();
        return msg;
    }
}

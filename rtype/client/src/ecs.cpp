//#pragma once

#include <asio.hpp>
#include <iostream>
#include <vector>
#include <memory>
#include <functional>
#include <queue>
#include <mutex>

// Définitions de protocole pour les en-têtes et la désérialisation
#include "../include/protocol/protocol_data.hpp"
#include "../include/protocol/serializer.hpp"
#include "../include/protocol/Factory.hpp"


class UdpClient {
public:
    using MessageHandler = std::function<void(std::unique_ptr<Protocol::IMessage>)>;

    UdpClient(asio::io_context& io_context, const std::string& server_ip, unsigned short server_port, MessageHandler handler)
        : m_socket(io_context, asio::ip::udp::endpoint(asio::ip::udp::v4(), 0)), // Bind sur un port aléatoire
          m_server_endpoint(asio::ip::address::from_string(server_ip), server_port),
          m_io_context(io_context),
          m_message_handler(handler)
    {
        std::cout << "[NET] UDP Client started, connecting to " << server_ip << ":" << server_port << std::endl;
        start_receive();
    }

    ~UdpClient() {
        if (m_socket.is_open()) {
            m_socket.close();
        }
    }

    void send(const std::vector<uint8_t>& data) {
        m_socket.async_send_to(asio::buffer(data), m_server_endpoint,
            [](auto ec, std::size_t /*bytes_sent*/) {
                if (ec) {
                    std::cerr << "[NET ERROR] Send failed: " << ec.message() << std::endl;
                }
            });
    }

    // Traitement des messages reçus dans le thread principal
    void poll_messages() {
        std::unique_ptr<Protocol::IMessage> message;
        while (pop_message(message)) {
            m_message_handler(std::move(message));
        }
    }

private:
    void start_receive() {
        m_socket.async_receive_from(asio::buffer(m_recv_buffer), m_remote_endpoint,
            [this](const asio::error_code& error, std::size_t bytes_recvd) {
                if (!error && bytes_recvd > 0) {
                    process_datagram(bytes_recvd);
                } else if (error) {
                    std::cerr << "[NET ERROR] Receive failed: " << error.message() << std::endl;
                }
                // Continue la réception, même en cas d'erreur non critique
                start_receive();
            });
    }

    void process_datagram(std::size_t bytes_recvd) {
        if (m_remote_endpoint != m_server_endpoint) {
            std::cerr << "[NET WARN] Received data from an unknown source. Ignoring." << std::endl;
            return;
        }

        std::vector<uint8_t> data(m_recv_buffer.begin(), m_recv_buffer.begin() + bytes_recvd);

        try {
            auto message = Protocol::MessageFactory::deserialize(data);
            push_message(std::move(message));
        } catch (const std::exception& e) {
            std::cerr << "[NET ERROR] Failed to deserialize message: " << e.what() << std::endl;
        }
    }

    void push_message(std::unique_ptr<Protocol::IMessage> message) {
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        m_received_messages.push(std::move(message));
    }

    bool pop_message(std::unique_ptr<Protocol::IMessage>& message) {
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        if (m_received_messages.empty()) {
            return false;
        }
        message = std::move(m_received_messages.front());
        m_received_messages.pop();
        return true;
    }

    asio::ip::udp::socket m_socket;
    asio::ip::udp::endpoint m_server_endpoint;
    asio::io_context& m_io_context;
    MessageHandler m_message_handler;

    // Buffer de réception et endpoint distant (pour async_receive_from)
    std::array<char, 1024> m_recv_buffer;
    asio::ip::udp::endpoint m_remote_endpoint;

    // File d'attente thread-safe pour les messages reçus
    std::queue<std::unique_ptr<Protocol::IMessage>> m_received_messages;
    std::mutex m_queue_mutex;
};

/*
** EPITECH PROJECT, 2025
** R-Type
** File description:
** UdpClient.hpp
*/

#pragma once

#include <asio.hpp>
#include <iostream>
#include <functional>
#include <array>
#include <memory>
#include <queue>
#include <mutex>
#include <optional>
#include "../include/protocol/serializer.hpp"
#include "../include/protocol/serializer.hpp"
// Assurez-vous que le chemin d'accès à vos headers de protocole est correct



namespace Client
{
    /**
     * @brief Structure pour stocker un message réseau reçu, prêt à être traité.
     */
    struct NetworkMessage {
        std::unique_ptr<Protocol::IMessage> message;
        asio::ip::udp::endpoint senderEndpoint;
    };

    /**
     * @brief Client UDP asynchrone pour la communication avec le serveur R-Type.
     * Utilise ASIO pour gérer l'E/S non bloquante.
     */
    class UdpClient {
    public:
        using MessageCallback = std::function<void(const NetworkMessage&)>;
        static constexpr size_t MAX_PACKET_SIZE = 1024; // Taille max d'un datagramme UDP

        /**
         * @brief Constructeur. Initialise le socket et le point de terminaison du serveur.
         * @param io_context Le contexte I/O d'ASIO.
         * @param server_host L'adresse IP ou le nom d'hôte du serveur.
         * @param server_port Le port du serveur.
         */
        UdpClient(asio::io_context& io_context, const std::string& server_host, unsigned short server_port);

        /**
         * @brief Démarre la boucle de réception asynchrone de paquets.
         */
        void start_receive();

        /**
         * @brief Envoie un message sérialisé au serveur.
         * @param message_to_send Le message à envoyer (déjà sérialisé).
         */
        void send_message(const std::vector<uint8_t>& message_to_send);

        /**
         * @brief Tente de défiler et retourner le premier message de la file.
         * @return Un optional contenant le message si la file n'est pas vide.
         */
        std::optional<NetworkMessage> poll_message();

        /**
         * @brief Définit la fonction de rappel (callback) appelée lors de la réception d'un message (sur le thread ASIO).
         * @param callback La fonction à appeler.
         */
        // void set_message_callback(MessageCallback callback);

    private:
        asio::ip::udp::socket m_socket;
        asio::ip::udp::endpoint m_server_endpoint;
        asio::ip::udp::endpoint m_remote_endpoint; // Pour stocker l'expéditeur de la réponse
        std::array<char, MAX_PACKET_SIZE> m_recv_buffer;

        // File d'attente pour transférer les messages reçus du thread ASIO au thread principal
        std::queue<NetworkMessage> m_message_queue;
        std::mutex m_queue_mutex;

        /**
         * @brief Traite les données brutes reçues.
         * @param bytes_recvd Le nombre d'octets reçus.
         */
        void process_datagram(std::size_t bytes_recvd);

        /**
         * @brief Ajoute un message décodé à la file d'attente.
         * @param msg Le message réseau à stocker.
         */
        void enqueue_message(NetworkMessage&& msg);
    };
}

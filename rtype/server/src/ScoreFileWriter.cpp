#include "../include/server/ScoreFileWriter.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>

ScoreFileWriter::ScoreFileWriter(const std::string& filePath)
    : m_filePath(filePath)
{
    // Ensure directory exists
    try {
        auto p = std::filesystem::path(m_filePath).parent_path();
        if (!p.empty() && !std::filesystem::exists(p)) {
            std::filesystem::create_directories(p);
        }
    } catch (const std::exception& e) {
        std::cerr << "[ScoreFileWriter] unable to ensure directory: " << e.what() << std::endl;
    }
}

ScoreFileWriter::~ScoreFileWriter() = default;

bool ScoreFileWriter::writeMatchScores(uint32_t matchId, int roomId, const std::vector<ScoreEntry>& entries)
{
    try {
        nlohmann::json root;
        root["match_id"] = matchId;
        root["room_id"] = roomId;
        root["generated_at"] = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

        root["players"] = nlohmann::json::array();
        for (const auto &e : entries) {
            nlohmann::json j;
            j["player_id"] = e.player_id;
            j["nickname"] = e.nickname;
            j["score"] = e.score;
            j["kills"] = e.kills;
            j["deaths"] = e.deaths;
            j["extras"] = e.extras.is_null() ? nlohmann::json::object() : e.extras;
            root["players"].push_back(j);
        }

        // Write to a temporary file then rename (atomic on POSIX)
        std::string tmpPath = m_filePath + ".tmp";
        {
            std::ofstream ofs(tmpPath, std::ios::out | std::ios::trunc);
            if (!ofs) {
                std::cerr << "[ScoreFileWriter] Cannot open temp file for writing: " << tmpPath << std::endl;
                return false;
            }
            ofs << root.dump(2) << std::endl;
            ofs.close();
        }
        std::filesystem::rename(tmpPath, m_filePath);
        std::cout << "[ScoreFileWriter] Wrote " << entries.size() << " entries to " << m_filePath << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[ScoreFileWriter] Exception: " << e.what() << std::endl;
        return false;
    }
}
#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>

// Minimal ScoreEntry (compatible avec ScoreStorage)
struct ScoreEntry {
    uint32_t player_id;
    std::string nickname;
    int score;
    int kills;
    int deaths;
    nlohmann::json extras; // optional JSON object
};

class ScoreFileWriter {
public:
    // filePath : chemin du fichier JSON à écrire (ex: "data/last_match_scores.json")
    explicit ScoreFileWriter(const std::string& filePath);
    ~ScoreFileWriter();

    // Écrit la liste d'entrées dans le fichier (atomiquement si possible)
    bool writeMatchScores(uint32_t matchId, int roomId, const std::vector<ScoreEntry>& entries);

private:
    std::string m_filePath;
};
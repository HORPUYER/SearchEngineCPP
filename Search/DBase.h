#pragma once
#include <string>
#include <vector>
#include <utility>

struct SearchResult 
{
    std::string url;
    std::string title;
    int rank;
};

class Database
{
public:
    Database(const std::string& connectionString);
    ~Database();

    // Create tables (idempotent)
    void createTables();

    // insert / get helpers
    int insertDocument(const std::string& url, const std::string& title, const std::string& content);
    int insertWord(const std::string& word);
    void insertDocumentWord(int document_id, int word_id, int frequency);

    int GetDocumentId(const std::string& url);
    int GetWordId(const std::string& word);

    std::vector<std::pair<std::string, int>> GetDocumentsByWord(const std::string& word);
    std::vector<std::pair<std::string, int>> GetWordsByDocument(int document_id);

    std::vector<SearchResult> SearchDocumentsByWords(const std::vector<std::string>& words);

    void clearAll();

private:
    std::string m_connStr; // store connection string, create pqxx::connection per method
};

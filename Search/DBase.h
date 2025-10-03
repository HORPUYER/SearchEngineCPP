#pragma once
#include <pqxx/pqxx>
#include <memory>
#include <string>
#include <vector>

struct SearchResult {
    std::string url;
    std::string title;
    int rank;
};

class Database {
public:
    explicit Database(const std::string& connectionString);
    ~Database();

    void createTables();

    int insertDocument(const std::string& url, const std::string& title, const std::string& content);

    // стандартные версии
    int insertWord(const std::string& word);
    void insertDocumentWord(int document_id, int word_id, int frequency);

    // версии для пакетной вставки через уже открытую транзакцию
    int insertWordTxn(pqxx::work& txn, const std::string& word);
    void insertDocumentWordTxn(pqxx::work& txn, int document_id, int word_id, int frequency);

    int GetDocumentId(const std::string& url);
    int GetWordId(const std::string& word);

    std::vector<std::pair<std::string, int>> GetDocumentsByWord(const std::string& word);
    std::vector<std::pair<std::string, int>> GetWordsByDocument(int document_id);
    std::vector<SearchResult> SearchDocumentsByWords(const std::vector<std::string>& words);

    void clearAll();

    // доступ к connection (для Spider)
    pqxx::connection& connection() { return *m_conn; }

private:
    std::string m_connStr;
    std::unique_ptr<pqxx::connection> m_conn;
};

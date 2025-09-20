#pragma once
#ifndef DATABASE_H
#define DATABASE_H

#include <pqxx/pqxx>
#include <string>
#include <vector>
#include <utility>

struct SearchResult {
    std::string url;
    std::string title;
    int rank;
};

class Database {
public:
    explicit Database(const std::string& connectionString);

    void createTables();

    int insertDocument(const std::string& url, const std::string& title, const std::string& content);
    int insertWord(const std::string& word);
    void insertDocumentWord(int document_id, int word_id, int frequency);

    int GetDocumentId(const std::string& url);
    int GetWordId(const std::string& word);

    std::vector<std::pair<std::string, int>> GetDocumentsByWord(const std::string& word);
    std::vector<std::pair<std::string, int>> GetWordsByDocument(int document_id);

    // Поиск документов по множеству слов
    std::vector<SearchResult> SearchDocumentsByWords(const std::vector<std::string>& words);

    // Очистка БД (для тестов/отладки)
    void clearAll();

private:
    pqxx::connection m_conn;
};

#endif // DATABASE_H

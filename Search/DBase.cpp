#include "DBase.h"
#include <iostream>
#include <sstream>
#include <stdexcept>

Database::Database(const std::string& connectionString)
    : m_connStr(connectionString),
    m_conn(std::make_unique<pqxx::connection>(connectionString))
{
    if (!m_conn->is_open()) {
        throw std::runtime_error("Failed to open DB connection");
    }
    createTables();
}

Database::~Database() {}

void Database::createTables()
{
    try {
        pqxx::work txn(*m_conn);

        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS Documents (
                id SERIAL PRIMARY KEY,
                url TEXT NOT NULL UNIQUE,
                title TEXT,
                content TEXT
            )
        )");

        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS Words (
                id SERIAL PRIMARY KEY,
                word TEXT NOT NULL UNIQUE
            )
        )");

        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS DocumentWords (
                document_id INT NOT NULL REFERENCES Documents(id) ON DELETE CASCADE,
                word_id INT NOT NULL REFERENCES Words(id) ON DELETE CASCADE,
                frequency INT NOT NULL,
                PRIMARY KEY(document_id, word_id)
            )
        )");

        txn.commit();
        std::cout << "[DB] Tables created or already exist." << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "[DB] createTables error: " << e.what() << std::endl;
        throw;
    }
}

int Database::insertDocument(const std::string& url, const std::string& title, const std::string& content)
{
    pqxx::work txn(*m_conn);
    pqxx::result r = txn.exec_params(
        "INSERT INTO Documents (url, title, content) VALUES ($1, $2, $3) "
        "ON CONFLICT (url) DO UPDATE SET title=EXCLUDED.title, content=EXCLUDED.content "
        "RETURNING id",
        url, title, content
    );
    int id = r[0][0].as<int>();
    txn.commit();
    return id;
}

int Database::insertWord(const std::string& word)
{
    pqxx::work txn(*m_conn);
    int id = insertWordTxn(txn, word);
    txn.commit();
    return id;
}

void Database::insertDocumentWord(int document_id, int word_id, int frequency)
{
    pqxx::work txn(*m_conn);
    insertDocumentWordTxn(txn, document_id, word_id, frequency);
    txn.commit();
}

int Database::insertWordTxn(pqxx::work& txn, const std::string& word)
{
    pqxx::result r = txn.exec_params(
        "INSERT INTO Words (word) VALUES ($1) "
        "ON CONFLICT (word) DO UPDATE SET word=EXCLUDED.word "
        "RETURNING id",
        word
    );
    return r[0][0].as<int>();
}

void Database::insertDocumentWordTxn(pqxx::work& txn, int document_id, int word_id, int frequency)
{
    txn.exec_params(
        "INSERT INTO DocumentWords (document_id, word_id, frequency) VALUES ($1, $2, $3) "
        "ON CONFLICT (document_id, word_id) DO UPDATE SET frequency = EXCLUDED.frequency",
        document_id, word_id, frequency
    );
}

int Database::GetDocumentId(const std::string& url)
{
    pqxx::work txn(*m_conn);
    pqxx::result r = txn.exec_params("SELECT id FROM Documents WHERE url = $1", url);
    if (r.empty()) return -1;
    return r[0][0].as<int>();
}

int Database::GetWordId(const std::string& word)
{
    pqxx::work txn(*m_conn);
    pqxx::result r = txn.exec_params("SELECT id FROM Words WHERE word = $1", word);
    if (r.empty()) return -1;
    return r[0][0].as<int>();
}

std::vector<std::pair<std::string, int>> Database::GetDocumentsByWord(const std::string& word)
{
    std::vector<std::pair<std::string, int>> results;
    pqxx::work txn(*m_conn);

    pqxx::result r = txn.exec_params(R"(
        SELECT d.url, dw.frequency
        FROM Documents d
        JOIN DocumentWords dw ON d.id = dw.document_id
        JOIN Words w ON dw.word_id = w.id
        WHERE w.word = $1
        ORDER BY dw.frequency DESC
    )", word);

    for (auto row : r) {
        results.emplace_back(row["url"].as<std::string>(), row["frequency"].as<int>());
    }
    return results;
}

std::vector<std::pair<std::string, int>> Database::GetWordsByDocument(int document_id)
{
    std::vector<std::pair<std::string, int>> results;
    pqxx::work txn(*m_conn);

    pqxx::result r = txn.exec_params(R"(
        SELECT w.word, dw.frequency
        FROM Words w
        JOIN DocumentWords dw ON w.id = dw.word_id
        WHERE dw.document_id = $1
        ORDER BY dw.frequency DESC
    )", document_id);

    for (auto row : r) {
        results.emplace_back(row["word"].as<std::string>(), row["frequency"].as<int>());
    }
    return results;
}

std::vector<SearchResult> Database::SearchDocumentsByWords(const std::vector<std::string>& words)
{
    std::vector<SearchResult> results;
    if (words.empty()) return results;

    std::ostringstream arr;
    arr << "{";
    for (size_t i = 0; i < words.size(); ++i) {
        std::string w = words[i];
        size_t pos = 0;
        while ((pos = w.find('"', pos)) != std::string::npos) { w.replace(pos, 1, "\\\""); pos += 2; }
        arr << "\"" << w << "\"";
        if (i + 1 < words.size()) arr << ",";
    }
    arr << "}";

    std::string sql = R"(
        SELECT d.url, d.title, SUM(dw.frequency) AS relevance
        FROM Documents d
        JOIN DocumentWords dw ON d.id = dw.document_id
        JOIN Words w ON dw.word_id = w.id
        WHERE w.word = ANY($1)
        GROUP BY d.url, d.title
        HAVING COUNT(DISTINCT w.word) = array_length($1,1)
        ORDER BY relevance DESC
        LIMIT 10
    )";

    pqxx::work txn(*m_conn);
    pqxx::result r = txn.exec_params(sql, arr.str());
    txn.commit();

    for (auto row : r) {
        SearchResult sr;
        sr.url = row["url"].as<std::string>();
        sr.title = row["title"].is_null() ? std::string() : row["title"].as<std::string>();
        sr.rank = row["relevance"].as<int>();
        results.push_back(sr);
    }

    return results;
}

void Database::clearAll()
{
    pqxx::work txn(*m_conn);
    txn.exec("TRUNCATE DocumentWords, Words, Documents RESTART IDENTITY CASCADE");
    txn.commit();
}

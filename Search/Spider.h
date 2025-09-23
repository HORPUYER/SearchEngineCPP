#pragma once
#ifndef SPIDER_H
#define SPIDER_H

#include "Config.h"
#include "DBase.h"

#include <boost/asio.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp> 

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <mutex>
#include <regex>

class Spider
{
public:
    Spider(Config& config, Database& db, std::size_t threads = 4);
    ~Spider();

    void run();

private:
    Config& m_config;
    Database& m_db;

    boost::asio::thread_pool m_pool;
    std::size_t m_threads;

    std::mutex m_visitedMutex;
    std::unordered_set<std::string> m_visited;

    std::mutex m_dbMutex;

    void crawl(const std::string& url, int depth);

    // HTTP/HTTPS
    std::string fetchPage(const std::string& url);

    // Парсинг/индексация
    std::string extractTitle(const std::string& html);
    std::string cleanText(const std::string& html);
    std::vector<std::string> extractLinks(const std::string& html, const std::string& baseUrl);
    std::string normalizeUrl(const std::string& link, const std::string& baseUrl);

    void splitAndCountWords(const std::string& text, std::unordered_map<std::string, int>& outFreq);
    std::string toLower(const std::string& s);
};

#endif // SPIDER_H

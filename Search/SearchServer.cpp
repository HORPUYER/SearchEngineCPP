#include "Config.h"
#include "DBase.h"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>

#include <iostream>
#include <sstream>
#include <thread>
#include <vector>
#include <cctype>

namespace beast = boost::beast;     // из boost
namespace http = beast::http;       // из boost
namespace net = boost::asio;        // из boost
using tcp = net::ip::tcp;

// ------------------ HTML шаблоны -------------------
static std::string make_search_form() 
{
    return R"(
<!doctype html>
<html>
<head><meta charset="utf-8"><title>Search</title></head>
<body>
  <h1>Search</h1>
  <form method="POST" action="/search">
    <input name="q" type="text" size="60" />
    <input type="submit" value="Search" />
  </form>
</body>
</html>
)";
}

static std::string make_results_page(const std::string& query, const std::vector<SearchResult>& results, const std::string& message = {}) 
{
    std::ostringstream oss;
    oss << "<!doctype html><html><head><meta charset='utf-8'><title>Results</title></head><body>";
    oss << "<h1>Results for: " << query << "</h1>";
    if (!message.empty()) {
        oss << "<p>" << message << "</p>";
    }
    if (results.empty()) {
        oss << "<p>No results found.</p>";
    }
    else {
        oss << "<ol>";
        for (auto& r : results) {
            oss << "<li><a href=\"" << r.url << "\">"
                << (r.title.empty() ? r.url : r.title)
                << "</a> (score: " << r.rank << ")</li>";
        }
        oss << "</ol>";
    }
    oss << "<p><a href='/'>Back</a></p>";
    oss << "</body></html>";
    return oss.str();
}

// ------------------ Вспомогательные функции -------------------
static std::string url_decode(const std::string& s) 
{
    std::string ret;
    char ch;
    int i, ii;
    for (i = 0; i < s.length(); i++) {
        if (int(s[i]) == 37) { // '%'
            sscanf(s.substr(i + 1, 2).c_str(), "%x", &ii);
            ch = static_cast<char>(ii);
            ret += ch;
            i = i + 2;
        }
        else if (s[i] == '+') {
            ret += ' ';
        }
        else {
            ret += s[i];
        }
    }
    return ret;
}

static std::vector<std::string> splitQueryWords(const std::string& q) 
{
    std::vector<std::string> res;
    std::istringstream iss(q);
    std::string w;
    while (iss >> w) {
        std::string clean;
        for (char c : w) {
            if (std::isalnum(static_cast<unsigned char>(c))) {
                clean.push_back(std::tolower(static_cast<unsigned char>(c)));
            }
        }
        if (clean.size() >= 3 && clean.size() <= 32) {
            res.push_back(clean);
        }
        if (res.size() >= 4) break; // максимум 4 слова
    }
    return res;
}

// ------------------ Обработка запроса -------------------
void handle_session(tcp::socket socket, Database& db)
{
    beast::error_code ec;
    beast::flat_buffer buffer;

    http::request<http::string_body> req;
    http::read(socket, buffer, req, ec);
    if (ec) return;

    http::response<http::string_body> res;

    try
    {
        if (req.method() == http::verb::get && req.target() == "/") 
        {
            // отдаём форму поиска
            res = http::response<http::string_body>(http::status::ok, req.version());
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, "text/html; charset=utf-8");
            res.body() = make_search_form();
            res.prepare_payload();
        }
        else if (req.method() == http::verb::post && (req.target() == "/search" || req.target() == "/"))
        {
            // разбираем тело запроса (q=...)
            std::string body = req.body();
            std::string q;
            size_t pos = body.find("q=");
            if (pos != std::string::npos)
            {
                q = url_decode(body.substr(pos + 2));
            }

            auto words = splitQueryWords(q);
            if (words.empty())
            {
                res = http::response<http::string_body>(http::status::ok, req.version());
                res.set(http::field::content_type, "text/html; charset=utf-8");
                res.body() = make_results_page(q, {}, "Empty or invalid query (words 3..32 chars, up to 4).");
                res.prepare_payload();
            }
            else 
            {
                std::vector<SearchResult> results = db.SearchDocumentsByWords(words);
                res = http::response<http::string_body>(http::status::ok, req.version());
                res.set(http::field::content_type, "text/html; charset=utf-8");
                res.body() = make_results_page(q, results);
                res.prepare_payload();
            }
        }
        else
        {
            res = http::response<http::string_body>(http::status::not_found, req.version());
            res.set(http::field::content_type, "text/plain; charset=utf-8");
            res.body() = "Not found";
            res.prepare_payload();
        }
    }
    catch (const std::exception& e) 
    {
        res = http::response<http::string_body>(http::status::internal_server_error, req.version());
        res.set(http::field::content_type, "text/html; charset=utf-8");
        res.body() = std::string("<h1>Internal error</h1><p>") + e.what() + "</p>";
        res.prepare_payload();
    }

    http::write(socket, res, ec);
    socket.shutdown(tcp::socket::shutdown_send, ec);
}

// ------------------ Запуск сервера -------------------
int run_server(const Config& cfg, Database& db) 
{
    try 
    {
        net::io_context ioc{ 1 };
        tcp::acceptor acceptor{ ioc, {tcp::v4(), static_cast<unsigned short>(cfg.GetServerPort())} };
        std::cout << "[Server] Listening on port " << cfg.GetServerPort() << "...\n";

        for (;;) 
        {
            tcp::socket socket = acceptor.accept();
            std::thread(&handle_session, std::move(socket), std::ref(db)).detach();
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}

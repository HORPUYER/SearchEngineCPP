#include "Spider.h"

#include <boost/beast/version.hpp>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>

using tcp = boost::asio::ip::tcp;
namespace beast = boost::beast;
namespace http = beast::http;
namespace ssl = boost::asio::ssl;

Spider::Spider(Config& config, Database& db, std::size_t threads)
    : m_config(config), m_db(db), m_pool(threads), m_threads(threads){
}

Spider::~Spider() 
{
    try { m_pool.join(); }
    catch (...) {}
}

void Spider::run() 
{
    std::string start = m_config.GetStartPage();
    if (start.empty()) 
    {
        std::cerr << "Start page not configured\n";
        return;
    }

    boost::asio::post(m_pool, [this, start]() {
        crawl(start, 0);
        });

    m_pool.join();
}

void Spider::crawl(const std::string& url, int depth)
{
    if (depth > m_config.GetRecursionDepth()) return;

    {
        std::lock_guard<std::mutex> lg(m_visitedMutex);
        if (!m_visited.insert(url).second) return;
    }

    std::string html;
    try
    {
        html = fetchPage(url);
    }
    catch (const std::exception& e) 
    {
        std::cerr << "fetchPage failed for " << url << " : " << e.what() << std::endl;
        return;
    }

    std::string title = extractTitle(html);
    std::string cleaned = cleanText(html);

    int docId = -1;
    {
        std::lock_guard<std::mutex> lg(m_dbMutex);
        docId = m_db.insertDocument(url, title, cleaned);
    }

    std::unordered_map<std::string, int> freq;
    splitAndCountWords(cleaned, freq);

    for (auto& p : freq) 
    {
        std::lock_guard<std::mutex> lg(m_dbMutex);
        int wordId = m_db.insertWord(p.first);
        m_db.insertDocumentWord(docId, wordId, p.second);
    }

    auto links = extractLinks(html, url);
    for (auto& lnk : links) 
    {
        std::string normalized = normalizeUrl(lnk, url);
        if (normalized.empty()) continue;

        boost::asio::post(m_pool, [this, normalized, depth]() {
            crawl(normalized, depth + 1);
            });
    }
}

std::string Spider::fetchPage(const std::string& url)
{
    static const std::regex urlRe(R"(^(https?)://([^/]+)(/.*)?$)", std::regex::icase);
    std::smatch m;
    if (!std::regex_match(url, m, urlRe))
    {
        throw std::runtime_error("Invalid URL: " + url);
    }

    std::string scheme = m[1].str();
    std::string host = m[2].str();
    std::string target = m[3].str();
    if (target.empty()) target = "/";

    boost::asio::io_context ioc;

    if (scheme == "http")
    {
        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc);

        auto const results = resolver.resolve(host, "80");
        stream.connect(results);

        http::request<http::string_body> req{ http::verb::get, target, 11 };
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);

        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        return res.body();
    }
    else
    { // https
        ssl::context ctx{ ssl::context::sslv23_client };
        ctx.set_default_verify_paths();

        tcp::resolver resolver(ioc);
        beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

        if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())) 
        {
            throw beast::system_error(
                beast::error_code(static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category()),
                "Failed to set SNI Hostname");
        }

        auto const results = resolver.resolve(host, "443");
        beast::get_lowest_layer(stream).connect(results);

        stream.handshake(ssl::stream_base::client);

        http::request<http::string_body> req{ http::verb::get, target, 11 };
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);

        beast::error_code ec;
        beast::get_lowest_layer(stream).socket().shutdown(tcp::socket::shutdown_both, ec);

        return res.body();
    }
}

std::string Spider::extractTitle(const std::string& html) 
{
    static const std::regex titleRe(R"(<title[^>]*>(.*?)</title>)", std::regex::icase);
    std::smatch m;
    if (std::regex_search(html, m, titleRe)) 
    {
        return m[1].str();
    }
    return {};
}

std::string Spider::cleanText(const std::string& html)
{
    std::string s = std::regex_replace(html, std::regex(R"(<script[\s\S]*?</script>)", std::regex::icase), " ");
    s = std::regex_replace(s, std::regex(R"(<style[\s\S]*?</style>)", std::regex::icase), " ");
    s = std::regex_replace(s, std::regex(R"(<[^>]*>)"), " ");

    std::string out;
    out.reserve(s.size());
    for (unsigned char ch : s) 
    {
        if (std::isalnum(ch)) out.push_back(static_cast<char>(ch));
        else out.push_back(' ');
    }

    std::transform(out.begin(), out.end(), out.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    std::string compact;
    bool lastSpace = true;
    for (char c : out) {
        if (std::isspace(static_cast<unsigned char>(c)))
        {
            if (!lastSpace) {
                compact.push_back(' ');
                lastSpace = true;
            }
        }
        else 
        {
            compact.push_back(c);
            lastSpace = false;
        }
    }
    if (!compact.empty() && compact.back() == ' ') compact.pop_back();

    return compact;
}

std::vector<std::string> Spider::extractLinks(const std::string& html, const std::string& baseUrl) 
{
    std::vector<std::string> links;
    static const std::regex hrefRe(R"(<a\s+[^>]*?href\s*=\s*['"]([^'"]+)['"][^>]*>)", std::regex::icase);
    auto it = std::sregex_iterator(html.begin(), html.end(), hrefRe);
    auto end = std::sregex_iterator();
    for (; it != end; ++it)
    {
        std::string raw = (*it)[1].str();
        if (raw.empty()) continue;
        if (raw.rfind("javascript:", 0) == 0) continue;
        if (raw.rfind("mailto:", 0) == 0) continue;
        size_t pos = raw.find('#');
        if (pos != std::string::npos) raw.erase(pos);
        if (raw.empty()) continue;
        links.push_back(raw);
    }
    return links;
}

std::string Spider::normalizeUrl(const std::string& link, const std::string& baseUrl)
{
    if (link.rfind("http://", 0) == 0 || link.rfind("https://", 0) == 0) return link;
    if (link.rfind("//", 0) == 0) return "http:" + link;

    static const std::regex baseRe(R"(^https?://([^/]+)(/.*)?$)", std::regex::icase);
    std::smatch m;
    if (!std::regex_match(baseUrl, m, baseRe)) return {};

    std::string host = m[1].str();
    std::string path = m[2].str();
    if (path.empty()) path = "/";

    if (!link.empty() && link[0] == '/') 
    {
        return "http://" + host + link;
    }
    else
    {
        auto pos = path.find_last_of('/');
        std::string basePath = (pos == std::string::npos) ? "/" : path.substr(0, pos + 1);
        return "http://" + host + basePath + link;
    }
}

void Spider::splitAndCountWords(const std::string& text, std::unordered_map<std::string, int>& outFreq)
{
    std::istringstream iss(text);
    std::string token;
    while (iss >> token)
    {
        if (token.size() < 3 || token.size() > 32) continue;
        outFreq[token]++;
    }
}

std::string Spider::toLower(const std::string& s) 
{
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return r;
}

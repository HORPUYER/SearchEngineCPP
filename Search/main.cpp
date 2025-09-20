#include <iostream>
#include <locale>
#include <memory>
#include <vector>

#include "Config.h"
#include "DBase.h"

using namespace std;

int main() {
    setlocale(LC_ALL, "RU");

    // путь к конфигу
    std::string file_name = "C:\\NetoC++\\SearchEngine\\Configs\\config.ini";
    auto conf = make_unique<Config>(file_name);

    // формируем строку подключения
    std::string connStr =
        "host=" + conf->GetDbHost() +
        " port=" + to_string(conf->GetDbPort()) +
        " dbname=" + conf->GetDbName() +
        " user=" + conf->GetDbUser() +
        " password=" + conf->GetDbPass();

    // подключаемся к базе
    auto dataBase = make_unique<Database>(connStr);

    cout << "Таблицы готовы.\n";

    // --- Тест вставки ---
    int docId = dataBase->insertDocument("https://example.com", "Example", "This is example content.");
    int wordId1 = dataBase->insertWord("example");
    int wordId2 = dataBase->insertWord("content");

    dataBase->insertDocumentWord(docId, wordId1, 3);
    dataBase->insertDocumentWord(docId, wordId2, 1);

    cout << "Документ и слова вставлены. docId=" << docId
        << ", wordId1=" << wordId1 << ", wordId2=" << wordId2 << "\n";

    // --- Проверка методов ---
    cout << "\n=== Проверка GetDocumentId ===\n";
    int checkDocId = dataBase->GetDocumentId("https://example.com");
    cout << "ID документа: " << checkDocId << "\n";

    cout << "\n=== Проверка GetWordId ===\n";
    int checkWordId = dataBase->GetWordId("example");
    cout << "ID слова 'example': " << checkWordId << "\n";

    cout << "\n=== Проверка GetDocumentsByWord ===\n";
    auto docs = dataBase->GetDocumentsByWord("example");
    for (auto& [url, freq] : docs) {
        cout << "Документ: " << url << " | Частота: " << freq << "\n";
    }

    cout << "\n=== Проверка GetWordsByDocument ===\n";
    auto words = dataBase->GetWordsByDocument(docId);
    for (auto& [word, freq] : words) {
        cout << "Слово: " << word << " | Частота: " << freq << "\n";
    }

    cout << "\n=== Проверка SearchDocumentsByWords ===\n";
    vector<string> searchWords = { "example", "content" };
    auto searchResults = dataBase->SearchDocumentsByWords(searchWords);
    for (auto& res : searchResults) {
        cout << "Документ: " << res.url << " | Релевантность: " << res.rank << "\n";
    }

    cout << "\nOk!\n";
    return 0;
}

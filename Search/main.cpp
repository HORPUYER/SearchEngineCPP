#include <iostream>
#include <locale>
#include <memory>
#include <vector>

#include "Config.h"
#include "DBase.h"

using namespace std;

int main() {
    setlocale(LC_ALL, "RU");

    // ���� � �������
    std::string file_name = "C:\\NetoC++\\SearchEngine\\Configs\\config.ini";
    auto conf = make_unique<Config>(file_name);

    // ��������� ������ �����������
    std::string connStr =
        "host=" + conf->GetDbHost() +
        " port=" + to_string(conf->GetDbPort()) +
        " dbname=" + conf->GetDbName() +
        " user=" + conf->GetDbUser() +
        " password=" + conf->GetDbPass();

    // ������������ � ����
    auto dataBase = make_unique<Database>(connStr);

    cout << "������� ������.\n";

    // --- ���� ������� ---
    int docId = dataBase->insertDocument("https://example.com", "Example", "This is example content.");
    int wordId1 = dataBase->insertWord("example");
    int wordId2 = dataBase->insertWord("content");

    dataBase->insertDocumentWord(docId, wordId1, 3);
    dataBase->insertDocumentWord(docId, wordId2, 1);

    cout << "�������� � ����� ���������. docId=" << docId
        << ", wordId1=" << wordId1 << ", wordId2=" << wordId2 << "\n";

    // --- �������� ������� ---
    cout << "\n=== �������� GetDocumentId ===\n";
    int checkDocId = dataBase->GetDocumentId("https://example.com");
    cout << "ID ���������: " << checkDocId << "\n";

    cout << "\n=== �������� GetWordId ===\n";
    int checkWordId = dataBase->GetWordId("example");
    cout << "ID ����� 'example': " << checkWordId << "\n";

    cout << "\n=== �������� GetDocumentsByWord ===\n";
    auto docs = dataBase->GetDocumentsByWord("example");
    for (auto& [url, freq] : docs) {
        cout << "��������: " << url << " | �������: " << freq << "\n";
    }

    cout << "\n=== �������� GetWordsByDocument ===\n";
    auto words = dataBase->GetWordsByDocument(docId);
    for (auto& [word, freq] : words) {
        cout << "�����: " << word << " | �������: " << freq << "\n";
    }

    cout << "\n=== �������� SearchDocumentsByWords ===\n";
    vector<string> searchWords = { "example", "content" };
    auto searchResults = dataBase->SearchDocumentsByWords(searchWords);
    for (auto& res : searchResults) {
        cout << "��������: " << res.url << " | �������������: " << res.rank << "\n";
    }

    cout << "\nOk!\n";
    return 0;
}

#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <algorithm>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

struct Document {
    int id;
    int relevance;
};

// ввод текста
string ReadLine() { 
    string s;
    getline(cin, s);
    return s;
}

// кол-во получаемых строк
int ReadLineWithNumber() { 
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

// перебирает слова из ввода, возвращает вектор
vector<string> SplitIntoWords(const string& text) { 
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

// добавление пары (вывода поисковой системы) в вектор : {ID документа ; вектор текста без стоп-слов}
class SearchServer {
public:
    void AddDocument(int document_id, const string& document) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        documents_.push_back({ document_id, words });
    }

    // перебирает стоп-слова, возвращает set
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    // Возвращает топ-5 самых релевантных документов в виде пар: {id, релевантность}
    vector<Document> FindTopDocuments(const string& raw_query) const {
        set<string> query_words = ParseQuery(raw_query);
        auto docs = FindAllDocuments(query_words);
        sort(docs.begin(), docs.end(), [](const Document& lhs, const Document& rhs) {
            return lhs.relevance > rhs.relevance;
            });
        if (docs.size() > MAX_RESULT_DOCUMENT_COUNT) {
            docs.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return docs;
    }

private:
    struct DocumentContent {
        int id = 0;
        vector<string> words;
    };

    vector<DocumentContent> documents_;
    set<string> stop_words_;

    // вспомогательная функция наличия стоп-слов, авторское решение
    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    // перебирает слова из ввода, исключает стоп-слова, возвращает вектор
    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    // Для каждого документа возвращает его id и релевантность
    vector<Document> FindAllDocuments(const set<string>& query_words) const {
        vector<Document> matched_documents;
        for (const auto& document : documents_) {
            const int relevance = MatchDocument(document, query_words);
            if (relevance > 0) {
                matched_documents.push_back({ document.id, relevance });
            }
        }
        return matched_documents;
    }

    // возвращает set строки ввода (запрос пользователя) без стоп-слов
    set<string> ParseQuery(const string& text) const {
        set<string> query_words;
        for (const string& word : SplitIntoWordsNoStop(text)) {
            query_words.insert(word);
        }
        return query_words;
    }

    // сопоставляет запрос и вывод поисковой системы, возвращает релевантность
    static int MatchDocument(const DocumentContent content, const set<string>& query_words) {
        if (query_words.empty()) {
            return 0;
        }
        set<string> matched_words;
        for (const string& word : content.words) {
            if (matched_words.count(word) != 0) {
                continue;
            }
            if (query_words.count(word) != 0) {
                matched_words.insert(word);
            }
        }
        return static_cast<int>(matched_words.size());
    }
};

SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());
    const int document_count = ReadLineWithNumber();
    for(int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }
    return search_server;
}


int main() {
    SearchServer search_server = CreateSearchServer();
    // Read documents
    const string query = ReadLine();
    // Вместо FindDocuments используйте FindTopDocuments
    for (auto [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", relevance = "s << relevance << " }"s
            << endl;
    }
}
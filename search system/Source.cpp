﻿#include <iostream>
#include "search_server.h"
#include "request_queue.h"
#include "paginator.h"

using namespace std::string_literals;


/*
//---------------------//
// Debag Functions
//---------------------//
 
// func map
template<typename Container_First, typename Container_Second>
ostream& Print(ostream& out, const map<Container_First, Container_Second>& container) {
    bool first = true;
    for (const auto& [key, value] : container) {
        if (!first) {
            out << ", ";
        }
        first = false;
        out << key << ": " << value;
    }
    return out;
}

// func vector, set
template<typename Container>
ostream& Print(ostream& out, const Container& container) {
    bool first = true;
    for (const auto& element : container) {
        if (!first) {
            out << ", ";
        }
        first = false;
        out << element;
    }
    return out;
}

// os vector
template <typename Element>
ostream& operator<<(ostream& out, const vector<Element>& container) {
    return out << '[', Print(out, container), out << ']';
}

// os set
template <typename Element>
ostream& operator<<(ostream& out, const set<Element>& container) {
    return out << '{', Print(out, container), out << '}';
}

// os map
template <typename Key_Element, typename Value_Element>
ostream& operator<<(ostream& out, const map<Key_Element, Value_Element>& container) {
    return out << '{', Print(out, container), out << '}';
}

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
    const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))


void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
    const string& hint) {
    if (!value) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename Function>
void RunTestImpl(const Function& func, const string& function_name) {
    func();
    cerr << function_name << " OK" << endl;
}

#define RUN_TEST(func) RunTestImpl(func, #func)
*/

/*
SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        const string document = ReadLine();
        int ratings_size;
        cin >> ratings_size;

        // создали вектор размера ratings_size из нулей
        vector<int> ratings(ratings_size, 0);

        // считали каждый элемент с помощью ссылки
        for (int& rating : ratings) {
            cin >> rating;
        }

        search_server.AddDocument(document_id, document, DocumentStatus(), ratings);
        ReadLine();
    }

    return search_server;
}
*/

/*
// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT(found_docs.size() == 1);
        const Document& doc0 = found_docs[0];
        ASSERT(doc0.id == doc_id);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
    // возвращает пустой результат
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT(server.FindTopDocuments("in"s).empty());
    }
}

    // Проверяем добавление документов
void TestAddDocument() {
    const int id = 1;
    const string document = "First test AddDocument in SearchServer"s;
    const vector<int> rating = { 3, -5, 1, 1 };
    {
        SearchServer server;
        server.AddDocument(id, document, DocumentStatus::ACTUAL, rating);
        server.AddDocument(2, "Second test AddDocument in SearchServer"s, DocumentStatus::ACTUAL, { 1, 1, 3, -8 });
        server.AddDocument(3, "Third test AddDocument in SearchServer"s, DocumentStatus::ACTUAL, { 2, -5, 1, 1 });
        ASSERT(server.GetDocumentCount() == 3);
    }
}

// Проверяем рейтинг
void TestRating() {
    {
        SearchServer server;
        const int id = 1;
        const string document = "First test for Rating"s;
        const vector<int> rating = { 1, 2, 3, 5, 13, 18 };
        DocumentStatus status = DocumentStatus::ACTUAL;
        server.AddDocument(id, document, status, rating);
        int quantity = rating.size();
        // int sum = accumulate(rating.begin(), rating.end(), 0);
        int sum = 0;
        for (const int rat : rating) {
            sum += rat;
        }
        auto total = sum / quantity;
        vector<Document> ratings = server.FindTopDocuments("First"s);
        ASSERT(total == ratings[0].rating);
    }
    {
        SearchServer server;
        const int id = 2;
        const string document = "Second test for Rating"s;
        const vector<int> rating = { -3, -1, -4, -5, -12 };
        DocumentStatus status = DocumentStatus::ACTUAL;
        server.AddDocument(id, document, status, rating);
        int quantity = rating.size();
        // int sum = accumulate(rating.begin(), rating.end(), 0);
        int sum = 0;
        for (const int rat : rating) {
            sum += rat;
        }
        auto total = sum / quantity;
        vector<Document> ratings = server.FindTopDocuments("Second"s);
        ASSERT(total == ratings[0].rating);
    }
    {
        SearchServer server;
        const int id = 3;
        const string document = "Third test for Rating"s;
        const vector<int> rating = { 3, -1, -4, 5 };
        DocumentStatus status = DocumentStatus::ACTUAL;
        server.AddDocument(id, document, status, rating);
        int quantity = rating.size();
        // int sum = accumulate(rating.begin(), rating.end(), 0);
        int sum = 0;
        for (const int rat : rating) {
            sum += rat;
        }
        auto total = sum / quantity;
        vector<Document> ratings = server.FindTopDocuments("Third"s);
        ASSERT(total == ratings[0].rating);
    }
}

// проверяем корректное добавление документов без стоп-слов
void TestMatchingDocuments() {
    const int id = 7;
    const string document = "Test for MatchDocument"s;
    const vector<int> rating = { 3, -1, 4, 5 };
    DocumentStatus status = DocumentStatus::ACTUAL;
    {
        SearchServer server;
        server.AddDocument(id, document, status, rating);
        string minus = "-MatchDocument"s;
        vector<string> test;
        tie(test, status) = server.MatchDocument(minus, id);
        ASSERT(test.empty());
    }
    {
        SearchServer server;
        server.AddDocument(id, document, status, rating);
        string plus = "MatchDocument"s;
        vector<string> test;
        tie(test, status) = server.MatchDocument(plus, id);
        ASSERT(!test.empty());
    }
}

void TestMinusWords() {
    const int id = 2;
    const string document = "Test for minus words"s;
    const vector<int> rating = { 3, -1, 4, 5 };
    DocumentStatus status = DocumentStatus::ACTUAL;
    {
        SearchServer server;
        server.AddDocument(id, document, status, rating);
        ASSERT(server.FindTopDocuments("-for"s).empty());
    }
}

void TestDocumentStatus() {
    const vector<int> rating = { 8, 3, -4, -5 };
    {
        SearchServer server;
        server.AddDocument(11, "First test for DocumentStatus"s, DocumentStatus::BANNED, rating);
        server.AddDocument(8, "Second test for DocumentStatus"s, DocumentStatus::REMOVED, rating);
        server.AddDocument(2, "Third test for DocumentStatus"s, DocumentStatus::BANNED, rating);
        server.AddDocument(14, "Third test for DocumentStatus"s, DocumentStatus::ACTUAL, rating);
        server.AddDocument(5, "Third test for DocumentStatus"s, DocumentStatus::IRRELEVANT, rating);
        ASSERT(server.FindTopDocuments("test"s, DocumentStatus::BANNED).size() == 2);
        ASSERT(server.FindTopDocuments("test"s, DocumentStatus::REMOVED).size() == 1);
        ASSERT(server.FindTopDocuments("test"s, DocumentStatus::IRRELEVANT).size() == 1);
        ASSERT(server.FindTopDocuments("test"s, DocumentStatus::ACTUAL).size() == 1);
    }
}

void TestRelevance() {
    SearchServer server;
    const vector<int> rating = { 9, -11, -4, 0 };
    const string word_to_rev = "Relevance"s;
    vector<vector<string>> words_to_test{};
    vector<double> TF{}, TF_IDF{};

    words_to_test.push_back(SplitIntoWords("First test for Relevance"s));
    words_to_test.push_back(SplitIntoWords("Second test for Relevance"s));
    words_to_test.push_back(SplitIntoWords("Third test for Relevance"s));
    words_to_test.push_back(SplitIntoWords("First Relevance Second Relevance Third Relevance"s));
    server.AddDocument(11, "First test for Relevance"s, DocumentStatus::ACTUAL, rating);
    server.AddDocument(8, "Second test for Relevance"s, DocumentStatus::ACTUAL, rating);
    server.AddDocument(2, "Third test for Relevance"s, DocumentStatus::BANNED, rating);
    server.AddDocument(1, "First Relevance Second Relevance Third Relevance"s, DocumentStatus::ACTUAL, rating);

    int freq = 0;
    for (auto& document : words_to_test) {
        int word_count = count(document.begin(), document.end(), word_to_rev);
        if (word_count > 0) {
            ++freq;
        }
        TF.push_back(word_count / static_cast<double>(document.size()));
    }

    double IDF = log(words_to_test.size() * 1.0 / static_cast<double>(freq));
    for (int i = 0; i < TF.size(); ++i) {
        if (IDF != 0) {
            TF_IDF.push_back(TF[i] * IDF);
        }
    }

    sort(TF_IDF.begin(), TF_IDF.end(), [](double& lhs, double& rhs) {
            double xhs = lhs - rhs;
            return xhs > EPSILON;
        });

    vector<Document> match = server.FindTopDocuments(word_to_rev);
    for (int i = 0; i < TF_IDF.size(); ++i) {
        ASSERT(abs(TF_IDF[i] - match[i].relevance) < EPSILON);
    }
}


void TestPredicate() {
    const vector<int> rating = { 1, 3, 6, 0 };
    {
        SearchServer server;
        server.SetStopWords("test for"s);
        server.AddDocument(11, "First test for Predicate"s, DocumentStatus::ACTUAL, rating);
        server.AddDocument(8, "Second test for Predicate"s, DocumentStatus::ACTUAL, rating);
        server.AddDocument(2, "Third test for Predicate"s, DocumentStatus::BANNED, rating);
        server.AddDocument(1, "First Second Third Predicate"s, DocumentStatus::ACTUAL, rating);
        int actual = server.FindTopDocuments("Predicate"s).size();
        ASSERT_EQUAL(actual, 3);
        int banned = server.FindTopDocuments("Predicate"s, DocumentStatus::BANNED).size();
        ASSERT_EQUAL(banned, 1);
    }
}


// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestRating);
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestAddDocument);
    RUN_TEST(TestMatchingDocuments);
    RUN_TEST(TestMinusWords);
    RUN_TEST(TestDocumentStatus);
    RUN_TEST(TestRelevance);
    RUN_TEST(TestPredicate);
}

// --------- Окончание модульных тестов поисковой системы -----------
*/

/*void PrintDocument(const Document& document) {
    cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s << endl;
}

void PrintMatchDocumentResult(int document_id, const vector<string>& words, DocumentStatus status) {
    cout << "{ "s
        << "document_id = "s << document_id << ", "s
        << "status = "s << static_cast<int>(status) << ", "s
        << "words ="s;
    for (const string& word : words) {
        cout << ' ' << word;
    }
    cout << "}"s << endl;
}

void AddDocument(SearchServer& search_server, int document_id, const string& document, DocumentStatus status,
    const vector<int>& ratings) {
    try {
        search_server.AddDocument(document_id, document, status, ratings);
    }
    catch (const exception& e) {
        cout << "Ошибка добавления документа "s << document_id << ": "s << e.what() << endl;
    }
}

void FindTopDocuments(const SearchServer& search_server, const string& raw_query) {
    cout << "Результаты поиска по запросу: "s << raw_query << endl;
    try {
        for (const Document& document : search_server.FindTopDocuments(raw_query)) {
            PrintDocument(document);
        }
    }
    catch (const exception& e) {
        cout << "Ошибка поиска: "s << e.what() << endl;
    }
}

void MatchDocuments(const SearchServer& search_server, const string& query) {
    try {
        cout << "Матчинг документов по запросу: "s << query << endl;
        const int document_count = search_server.GetDocumentCount();
        for (int index = 0; index < document_count; ++index) {
            const int document_id = search_server.GetDocumentId(index);
            const auto [words, status] = search_server.MatchDocument(query, document_id);
            PrintMatchDocumentResult(document_id, words, status);
        }
    }
    catch (const exception& e) {
        cout << "Ошибка матчинга документов на запрос "s << query << ": "s << e.what() << endl;
    }
}*/

int main() {
    SearchServer search_server("and in at"s);
    RequestQueue request_queue(search_server);

    search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, { 1, 2, 8 });
    search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
    search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, { 1, 1, 1 });

    // 1439 запросов с нулевым результатом
    for (int i = 0; i < 1439; ++i) {
        request_queue.AddFindRequest("empty request"s);
    }
    // все еще 1439 запросов с нулевым результатом
    request_queue.AddFindRequest("curly dog"s);
    // новые сутки, первый запрос удален, 1438 запросов с нулевым результатом
    request_queue.AddFindRequest("big collar"s);
    // первый запрос удален, 1437 запросов с нулевым результатом
    request_queue.AddFindRequest("sparrow"s);
    std::cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << std::endl;
    return 0;
}
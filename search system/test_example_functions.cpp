#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <sstream>

#include "test_example_functions.h"
#include "search_server.h"
#include "remove_duplicates.h"
#include "process_queries.h"

using namespace std;


void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
    const string& hint) {
    if (!value) {
        cerr << file << "("s << line << "): "s << func << ": "s
            << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

vector<string> SplitToWords(const string& line) {
    vector<string> output;
    istringstream iss(line);
    string word;

    while (iss >> word) {
        output.push_back(word);
    }

    return output;
}

int AverageRating(const vector<int>& ratings) {
    int rating_sum = 0;
    for (const int rating : ratings) {
        rating_sum += rating;
    }
    return rating_sum / static_cast<int>(ratings.size());
}


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
    /* {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT(server.FindTopDocuments("in"s).empty());
    }*/
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
        vector<string_view> test;
        tie(test, status) = server.MatchDocument(minus, id);
        ASSERT(test.empty());
    }
    {
        SearchServer server;
        server.AddDocument(id, document, status, rating);
        string plus = "MatchDocument"s;
        vector<string_view> test;
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

    /*words_to_test.push_back(SplitIntoWords("First test for Relevance"s));
    words_to_test.push_back(SplitIntoWords("Second test for Relevance"s));
    words_to_test.push_back(SplitIntoWords("Third test for Relevance"s));
    words_to_test.push_back(SplitIntoWords("First Relevance Second Relevance Third Relevance"s));*/
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
        SearchServer server ("test for"s);
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

void TestAverageRating() {
    SearchServer server;

    vector<vector<int>>  ratings{ { 7, 2, 7 },{ 5, -12, 2, 1 },{ 8, -3 } };

    server.AddDocument(0, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, ratings[2]);
    server.AddDocument(1, "funny pet with curly hair"s, DocumentStatus::ACTUAL, ratings[0]);
    server.AddDocument(2, "funny pet and curly hair"s, DocumentStatus::ACTUAL, ratings[1]);
    server.AddDocument(3, "nasty rat with curly hair"s, DocumentStatus::ACTUAL, { 9 });
    server.AddDocument(4, "very nasty rat and not very funny pet"s, DocumentStatus::ACTUAL, { 8 });
    server.AddDocument(5, "pet with rat and rat and rat"s, DocumentStatus::ACTUAL, { 8,1,5 });
    server.AddDocument(6, "funny cat with curly hair"s, DocumentStatus::BANNED, { 9,5,2 });

    const auto top_documents = server.FindTopDocuments("funny pet with curly hair"s);
    for (size_t i = 0; i < top_documents.size(); ++i) {
        ASSERT_EQUAL(top_documents[i].rating, AverageRating(ratings[i]));
    }
}

void TestRemoveDuplicates() {
    SearchServer search_server("and with"s);

    search_server.AddDocument(1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

    // дубликат документа 2, будет удалён
    search_server.AddDocument(3, "funny pet with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

    // отличие только в стоп-словах, считаем дубликатом
    search_server.AddDocument(4, "funny pet and curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

    // множество слов такое же, считаем дубликатом документа 1
    search_server.AddDocument(5, "funny funny pet and nasty nasty rat"s, DocumentStatus::ACTUAL, { 1, 2 });

    // добавились новые слова, дубликатом не является
    search_server.AddDocument(6, "funny pet and not very nasty rat"s, DocumentStatus::ACTUAL, { 1, 2 });

    // множество слов такое же, как в id 6, несмотря на другой порядок, считаем дубликатом
    search_server.AddDocument(7, "very nasty rat and not very funny pet"s, DocumentStatus::ACTUAL, { 1, 2 });

    // есть не все слова, не является дубликатом
    search_server.AddDocument(8, "pet with rat and rat and rat"s, DocumentStatus::ACTUAL, { 1, 2 });

    // слова из разных документов, не является дубликатом
    search_server.AddDocument(9, "nasty rat with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

    vector<int> right_result{ 1,2,6,8,9 };
    RemoveDuplicates(search_server);
    size_t index = 0;
    for (const int id : search_server) {
        ASSERT_EQUAL(id, right_result[index]);
        ++index;
    }
}

void TestProcessQueries() {
    {
        SearchServer search_server("and with"s);

        int id = 0;
        for (
            const string& text : {
                "funny pet and nasty rat"s,
                "funny pet with curly hair"s,
                "funny pet and not very nasty rat"s,
                "pet with rat and rat and rat"s,
                "nasty rat with curly hair"s,
            }
            ) {
            search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
        }

        const vector<string> queries = {
            "nasty rat -not"s,
            "not very funny nasty pet"s,
            "curly hair"s
        };
        id = 0;
        for (
            const auto& documents : ProcessQueries(search_server, queries)
            ) {
            cout << documents.size() << " documents for query ["s << queries[id++] << "]"s << endl;
        }
    }

    {
        SearchServer search_server("and with"s);

        int id = 0;
        for (
            const string& text : {
                "funny pet and nasty rat"s,
                "funny pet with curly hair"s,
                "funny pet and not very nasty rat"s,
                "pet with rat and rat and rat"s,
                "nasty rat with curly hair"s,
            }
            ) {
            search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
        }

        const vector<string> queries = {
            "nasty rat -not"s,
            "not very funny nasty pet"s,
            "curly hair"s
        };
        for (const Document& document : ProcessQueriesJoined(search_server, queries)) {
            cout << "Document "s << document.id << " matched with relevance "s << document.relevance << endl;
        }
    }
}

void TestRemoveDocuments() {
    SearchServer search_server("and with"s);

    int id = 0;
    for (
        const string& text : {
            "funny pet and nasty rat"s,
            "funny pet with curly hair"s,
            "funny pet and not very nasty rat"s,
            "pet with rat and rat and rat"s,
            "nasty rat with curly hair"s,
        }
        ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
    }

    const string query = "curly and funny"s;

    auto report = [&search_server, &query] {
        cout << search_server.GetDocumentCount() << " documents total, "s
            << search_server.FindTopDocuments(query).size() << " documents for query ["s << query << "]"s << endl;
    };

    report();
    // однопоточная версия
    search_server.RemoveDocument(5);
    report();
    // однопоточная версия
    search_server.RemoveDocument(execution::seq, 1);
    report();
    // многопоточная версия
    search_server.RemoveDocument(execution::par, 2);
    report();
}

// --------- Окончание модульных тестов поисковой системы -----------


// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestRating);
    RUN_TEST(TestAddDocument);
    RUN_TEST(TestMatchingDocuments);
    RUN_TEST(TestMinusWords);
    RUN_TEST(TestDocumentStatus);
    RUN_TEST(TestRelevance);
    RUN_TEST(TestPredicate);
    RUN_TEST(TestAverageRating);
    RUN_TEST(TestRemoveDuplicates);
    RUN_TEST(TestProcessQueries);
    RUN_TEST(TestRemoveDocuments);
}


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


void PrintDocument(const Document& document) {
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

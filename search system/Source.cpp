#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <algorithm>
#include <map>
#include <cmath>
#include <numeric>
#include <cassert>

using namespace std;

// Debag Functions

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


// Search_Server start

const int MAX_RESULT_DOCUMENT_COUNT = 5;

// ввод текста
string ReadLine() { 
    string s;
    getline(cin, s);
    return s;
}

// кол-во получаемых строк
int ReadLineWithNumber() { 
    int result;
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

struct Document {
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED
};

// добавление пары (вывода поисковой системы) в вектор : {ID документа ; вектор текста без стоп-слов}
class SearchServer {
public:
    // перебирает стоп-слова, возвращает set
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        ++document_count_;
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        documents_rating_.insert({ document_id, ComputeAverageRating(ratings) });
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        status_.emplace( document_id, DocumentData{ComputeAverageRating(ratings), status });
    }

    template <typename Status>
    // Возвращает топ-5 самых релевантных документов в виде пар: {id, релевантность}
    vector<Document> FindTopDocuments(const string& raw_query, Status status) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, status);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                const double EPSILON = 1e-6;
                double xhs = lhs.relevance - rhs.relevance;
                return lhs.relevance > rhs.relevance || ((abs(xhs) < EPSILON) && (lhs.rating > rhs.rating));
            });

        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(raw_query, [&status](int id, DocumentStatus r_status, int rating) {
            return r_status == status;
            });
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, [](int id, DocumentStatus status, int rating) {
            return status == DocumentStatus::ACTUAL;
            });
    }

    int GetDocumentCount() const {
        return status_.size();
    }

    // сопоставляет запрос и вывод поисковой системы, возвращает релевантность
    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        Query query_words = ParseQuery(raw_query);
        vector<string> words;
        for (const string& word : query_words.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                words.push_back(word);
            }
        }
        for (const string& word : query_words.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                words.clear();
                break;
            }
        }
        return { words, status_.at(document_id).status };
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    int document_count_ = 0;
    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, int> documents_rating_;
    map<int, DocumentData> status_;

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

    static int ComputeAverageRating(const vector<int>& ratings) {
        int quantity = ratings.size();
        int sum = 0;
        if (ratings.empty()) {
            return 0;
        }
        sum = accumulate(ratings.begin(), ratings.end(), 0);
        return sum / quantity;
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return {
            text,
            is_minus,
            IsStopWord(text)
        };
    }

    struct Query {
        set<string> minus_words;
        set<string> plus_words;
    };

    // возвращает set строки ввода (запрос пользователя) без стоп-слов
    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                }
                else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(document_count_ * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename Status>
    // Для каждого документа возвращает его id и релевантность
    vector<Document> FindAllDocuments(const Query& query_words, Status status) const {
        map<int, double> document_to_relevance;
        vector<Document> matched_documents;

        for (const string& word : query_words.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto& [id, TF] : word_to_document_freqs_.at(word)) {
                if (status(id, status_.at(id).status, status_.at(id).rating)) {
                    document_to_relevance[id] += TF * inverse_document_freq;
                }
            }
        }

        for (const string& word : query_words.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto& [id, TF] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(id);
            }
        }

        for (const auto& [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({ 
                document_id, 
                relevance, 
                status_.at(document_id).rating });
        }

        return matched_documents;
    }
};


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

// Проверяем рейтинг и корректное добавление документов без стоп слов
void TestRatingAndMatchingDocuments() {
    const int id = 7;
    const string document = "Test for Rating and MatchDocument"s;
    const vector<int> rating = { 3, -1, 4, 5 };
    DocumentStatus status = DocumentStatus::ACTUAL;
    {
        SearchServer server;
        server.AddDocument(id, document, status, rating);
        int quantity = rating.size();
        // int sum = accumulate(rating.begin(), rating.end(), 0);
        int sum = 0;
        for (const int rat : rating) {
            sum += rat;
        }
        auto total = sum / quantity;
        vector<Document> ratings = server.FindTopDocuments("Rating"s);
        ASSERT(total == ratings[0].rating);
    }
    {
        SearchServer server;
        server.AddDocument(id, document, status, rating);
        string minus = "-Rating"s;
        vector<string> test;
        tie(test, status) = server.MatchDocument(minus, id);
        ASSERT(test.empty());
    }
    {
        SearchServer server;
        server.AddDocument(id, document, status, rating);
        string plus = "Rating"s;
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
        ASSERT(server.FindTopDocuments("test"s, DocumentStatus::BANNED).size() == 2);
        ASSERT(server.FindTopDocuments("test"s, DocumentStatus::REMOVED).size() == 1);
    }
}

void TestRevelance() {
    SearchServer server;
    const vector<int> rating = { 9, -11, -4, 0 };
    const string word_to_rev = "Revelance"s;
    vector<vector<string>> words_to_test{};
    vector<double> TF{}, TF_IDF{};

    words_to_test.push_back(SplitIntoWords("First test for Revelance"s));
    words_to_test.push_back(SplitIntoWords("Second test for Revelance"s));
    words_to_test.push_back(SplitIntoWords("Third test for Revelance"s));
    words_to_test.push_back(SplitIntoWords("First Revelance Second Revelance Third Revelance"s));
    server.AddDocument(11, "First test for Revelance"s, DocumentStatus::ACTUAL, rating);
    server.AddDocument(8, "Second test for Revelance"s, DocumentStatus::ACTUAL, rating);
    server.AddDocument(2, "Third test for Revelance"s, DocumentStatus::BANNED, rating);
    server.AddDocument(1, "First Revelance Second Revelance Third Revelance"s, DocumentStatus::ACTUAL, rating);

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

    const double EPSILON = 1e-6;
    sort(TF_IDF.begin(), TF_IDF.end(), [EPSILON](double& lhs, double& rhs) {
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
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestAddDocument);
    RUN_TEST(TestRatingAndMatchingDocuments);
    RUN_TEST(TestMinusWords);
    RUN_TEST(TestDocumentStatus);
    RUN_TEST(TestRevelance);
    RUN_TEST(TestPredicate);
}

// --------- Окончание модульных тестов поисковой системы -----------

void PrintDocument(const Document& document) {
    cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating
        << " }"s << endl;
}


int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;

    SearchServer search_server;
    search_server.SetStopWords("и в на"s);

    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });

    cout << "ACTUAL by default:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s)) {
        PrintDocument(document);
    }

    cout << "BANNED:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED)) {
        PrintDocument(document);
    }

    cout << "Even ids:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
        PrintDocument(document);
    }

    return 0;
}


/*int main() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);

    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });

    cout << "ACTUAL by default:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s)) {
        PrintDocument(document);
    }

    cout << "BANNED:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED)) {
        PrintDocument(document);
    }

    cout << "Even ids:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
        PrintDocument(document);
    }

    return 0;
}*/

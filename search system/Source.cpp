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

//---------------------//
// Search_Server start
//---------------------//

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

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

template <typename StringContainer>
set<string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    set<string> non_empty_strings;
    for (const string& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}

struct Document {
    Document() = default;
    Document(int id, double relevance, int rating) 
        : id(id)
        , relevance(relevance)
        , rating(rating){}

    int id = 0;
    double relevance = 0.0;
    int rating = 0;
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
    SearchServer() = default;
    inline static constexpr int INVALID_DOCUMENT_ID = -1;

    template <typename StringCollection>
    explicit SearchServer(const StringCollection& stop_words) 
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {}
    
    explicit SearchServer(const string& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text)) {}

    // перебирает стоп-слова, возвращает set
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    [[nodiscard]] bool AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        if (document_id < 0 || status_.count(document_id)) {
            return false;
        }

        vector<string> split_into_words_no_stop;
        if (SplitIntoWordsNoStop(document, split_into_words_no_stop)) {
            const vector<string> words = split_into_words_no_stop;
            const double inv_word_count = 1.0 / words.size();
            for (const string& word : words) {
                word_to_document_freqs_[word][document_id] += inv_word_count;
            }
            status_.emplace( document_id, DocumentData{ComputeAverageRating(ratings), status });
            //document_ids_.push_back(document_id);
            return true;
        } 
        return false;
    }

    template <typename Status>
    // Возвращает топ-5 самых релевантных документов в виде пар: {id, релевантность}
    [[nodiscard]] bool FindTopDocuments(const string& raw_query, Status status, vector<Document>& matched_documents) const {
        Query query;
        if (ParseQuery(raw_query, query)) {
            matched_documents = FindAllDocuments(query, status);

            sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
                double xhs = lhs.relevance - rhs.relevance;
                return lhs.relevance > rhs.relevance || ((abs(xhs) < EPSILON) && (lhs.rating > rhs.rating));
                });

            if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
                matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
            }
            return true;
        }
        return false;
    }

    [[nodiscard]] bool FindTopDocuments(const string& raw_query, DocumentStatus status, vector<Document>& matched_documents) const {
        if (FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) { 
            return document_status == status; }, matched_documents)) {
            return true;
        }
        return false;
    }

    [[nodiscard]] bool FindTopDocuments(const string& raw_query, vector<Document>& matched_documents) const {
        if (FindTopDocuments(raw_query, DocumentStatus::ACTUAL, matched_documents)) {
            return true;
        }
        return false;
    }

    int GetDocumentCount() const {
        return status_.size();
    }

    // сопоставляет запрос и вывод поисковой системы, возвращает релевантность
    [[nodiscard]] bool MatchDocument(const string& raw_query, int document_id, tuple<vector<string>, DocumentStatus>& result) const {
        Query query_words;

        if (ParseQuery(raw_query, query_words)) {
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
            result = { words, status_.at(document_id).status };
            return true;
        } 
        return false;
    }

    int GetDocumentId(int index) const {
        if (index<0 || index>status_.size() - 1) {
            return SearchServer::INVALID_DOCUMENT_ID;
        }
        else {
            auto it = status_.begin();
            advance(it, index);
            return it->first;
        }
    }

    /*
    int GetDocumentId(int index) const {
        if (index >= 0 && index < status_.size()) {
            return document_ids_.at(index);
        }
        else {
            return SearchServer::INVALID_DOCUMENT_ID;
        }
    }
    */

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
    //vector<int> document_ids_;

    static bool IsValidWord(const string& word) {
        // A valid word must not contain special characters
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
            });
    }

    // вспомогательная функция наличия стоп-слов, авторское решение
    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    // перебирает слова из ввода, исключает стоп-слова, возвращает вектор
    [[nodiscard]] bool SplitIntoWordsNoStop(const string& text, vector<string>& split_into_words_no_stop) const {
        //vector<string> words;
        if (IsValidWord(text)) {
            for (const string& word : SplitIntoWords(text)) {
                if (!IsStopWord(word)) {
                    split_into_words_no_stop.push_back(word);
                }
            }
            return true;
        }
        return false;
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

    [[nodiscard]] bool ParseQueryWord(string text, QueryWord& query_word) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-' && text[1] != '-' && isblank(text[1])) {
            is_minus = true;
            text = text.substr(1);
            query_word = {
                    text,
                    is_minus,
                    IsStopWord(text)
            };
            return true;
        }

        if (text[0] != '-') {
            is_minus = false;
            query_word = {
                    text,
                    is_minus,
                    IsStopWord(text)
            };
            return true;
        }
        return false;
    }

    struct Query {
        set<string> minus_words;
        set<string> plus_words;
    };

    // возвращает set строки ввода (запрос пользователя) без стоп-слов
    [[nodiscard]] bool ParseQuery(const string& text, Query& query) const {
        if (IsValidWord(text)) {
            for (const string& word : SplitIntoWords(text)) {
                QueryWord query_word;
                if (ParseQueryWord(word, query_word)) {
                    if (!query_word.is_stop) {
                        if (query_word.is_minus) {
                            query.minus_words.insert(query_word.data);
                            return true;
                        } else {
                            query.plus_words.insert(query_word.data);
                        }
                    } 
                } else { return false; }
            }
        } else { return false; }
        return true;
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

void PrintDocument(const Document& document) {
    cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s << endl;
}

void Test() {
    SearchServer search_server(std::string{ "и в на" });
    vector<Document> documents;
    search_server.AddDocument(0, "кот в мешке", DocumentStatus::ACTUAL, { 1, 2, 3 });
    search_server.AddDocument(1, "чук и гек пьют иван-чай", DocumentStatus::ACTUAL, { 3 });
    std::tuple<std::vector<std::string>, DocumentStatus> docs;
    int document_id = 0;
    bool WrongMatch = search_server.MatchDocument("--кот", document_id, docs);
    bool WrongMatch2 = search_server.FindTopDocuments("и в на"s, documents);
    if (WrongMatch == true || WrongMatch2 == true) {
        std::cout << "WRONG MATCH\n";
    }
    else {
        std::cout << "GOOD JOB\n";
    }
}

int main() {
    setlocale(LC_ALL, "ru");
    Test();
    SearchServer search_server("и в на"s);

    // Явно игнорируем результат метода AddDocument, чтобы избежать предупреждения
    // о неиспользуемом результате его вызова
    (void)search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });

    if (!search_server.AddDocument(1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2 })) {
        cout << "Документ не был добавлен, так как его id совпадает с уже имеющимся"s << endl;
    }

    if (!search_server.AddDocument(-1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2 })) {
        cout << "Документ не был добавлен, так как его id отрицательный"s << endl;
    }

    if (!search_server.AddDocument(3, "большой пёс скво\x12рец"s, DocumentStatus::ACTUAL, { 1, 3, 2 })) {
        cout << "Документ не был добавлен, так как содержит спецсимволы"s << endl;
    }

    vector<Document> documents;
    if (search_server.FindTopDocuments("--пушистый"s, documents)) {
        for (const Document& document : documents) {
            PrintDocument(document);
        }
    }
    else {
        cout << "Ошибка в поисковом запросе"s << endl;
    }
}
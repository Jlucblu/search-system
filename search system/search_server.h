#pragma once
#include <set>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <numeric>
#include <stdexcept>
#include <stack>
#include <cmath>

#include "string_processing.h"
#include "document.h"
#include "log_duration.h"

using namespace std::string_literals;


//---------------------//
// Search_Server start //
//---------------------//

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED
};

// добавление пары (вывода поисковой системы) в вектор : {ID документа ; вектор текста без стоп-слов}
class SearchServer {
public:
    // Defines an invalid document id
    // You can refer this constant as SearchServer::INVALID_DOCUMENT_ID
    SearchServer() = default;

    template <typename StringCollection>
    explicit SearchServer(const StringCollection& stop_words); 
    explicit SearchServer(const std::string& stop_words_text);

    void SetStopWords(const std::string& text);

    bool AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings);

    // Возвращает топ-5 самых релевантных документов в виде пар: {id, релевантность}
    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentPredicate document_predicate) const;
    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(const std::string& raw_query) const;

    int GetDocumentCount() const;

    // сопоставляет запрос и вывод поисковой системы, возвращает релевантность
    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query, int document_id) const;

    const std::map<std::string, double>& GetWordFrequencies(int document_id) const;

    [[nodiscard]] std::set<int>::const_iterator begin() const;
    [[nodiscard]] std::set<int>::const_iterator end() const;

    /*int GetDocumentId(int index) const;*/

    void RemoveDocument(int document_id);


private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    std::set<std::string> stop_words_;
    std::map<std::string, std::map<int, double>> word_to_id_freqs_;
    std::map<int, std::map<std::string, double>> id_to_word_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;


    // A valid word must not contain special characters
    static bool IsValidWord(const std::string& word);

    // вспомогательная функция наличия стоп-слов, авторское решение
    bool IsStopWord(const std::string& word) const;

    // перебирает слова из ввода, исключает стоп-слова, возвращает вектор
    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string data;
        bool is_minus;
        bool is_stop;
    };

    // Empty result by initializing it with default constructed QueryWord
    [[nodiscard]] QueryWord ParseQueryWord(std::string text) const;

    struct Query {
        std::set<std::string> minus_words;
        std::set<std::string> plus_words;
    };

    // возвращает set строки ввода (запрос пользователя) без стоп-слов
    [[nodiscard]] Query ParseQuery(const std::string& text) const;

    // Existence required
    double ComputeWordInverseDocumentFreq(const std::string& word) const;

    // Для каждого документа возвращает его id и релевантность
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query_words, DocumentPredicate document_predicate) const;
};

template <typename StringCollection>
SearchServer::SearchServer(const StringCollection& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
    for (const std::string& word : stop_words_) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Text contain invalid symbols"s);
        }
    }
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentPredicate document_predicate) const {
    Query query = ParseQuery(raw_query);

    auto matched_documents = FindAllDocuments(query, document_predicate);

    sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
        if (std::abs(lhs.relevance - rhs.relevance) < EPSILON) {
            return lhs.rating > rhs.rating;
        }
        else {
            return lhs.relevance > rhs.relevance;
        }
        });

    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query_words, DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    std::vector<Document> matched_documents;

    for (const std::string& word : query_words.plus_words) {
        if (word_to_id_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto& [id, TF] : word_to_id_freqs_.at(word)) {
            if (document_predicate(id, documents_.at(id).status, documents_.at(id).rating)) {
                document_to_relevance[id] += TF * inverse_document_freq;
            }
        }
    }

    for (const std::string& word : query_words.minus_words) {
        if (word_to_id_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto& [id, TF] : word_to_id_freqs_.at(word)) {
            document_to_relevance.erase(id);
        }
    }

    for (const auto& [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back({
            document_id,
            relevance,
            documents_.at(document_id).rating });
    }

    return matched_documents;
}
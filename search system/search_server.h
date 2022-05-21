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
#include <execution>
#include <tuple>
#include <mutex>

#include "string_processing.h"
#include "document.h"
#include "log_duration.h"
#include "concurrent_map.h"

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
    explicit SearchServer(std::string_view stop_words_text);

    //void SetStopWords(const std::string& text);

    void AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings);

    // Возвращает топ-5 самых релевантных документов в виде пар: {id, релевантность}
    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const;
    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(std::string_view raw_query) const;
    template <typename DocumentPredicate, typename Policy>
    std::vector<Document> FindTopDocuments(Policy&& policy, std::string_view raw_query, DocumentPredicate document_predicate) const;
    template <typename Policy>
    std::vector<Document> FindTopDocuments(Policy&& policy, std::string_view raw_query, DocumentStatus status) const;
    template <typename Policy>
    std::vector<Document> FindTopDocuments(Policy&& policy, std::string_view raw_query) const;

    int GetDocumentCount() const;

    // сопоставляет запрос и вывод поисковой системы, возвращает релевантность
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::string_view raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::sequenced_policy policy, std::string_view raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::parallel_policy policy, std::string_view raw_query, int document_id) const;

    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

    [[nodiscard]] std::set<int>::const_iterator begin() const;
    [[nodiscard]] std::set<int>::const_iterator end() const;

    /*int GetDocumentId(int index) const;*/

    void RemoveDocument(int document_id);
    template<typename Policy>
    void RemoveDocument(Policy&& policy, int document_id);

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    struct Query {
        std::vector<std::string_view> minus_words;
        std::vector<std::string_view> plus_words;
    };

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    StringSet stop_words_;
    StringSet document_words_;
    std::map<std::string, std::map<int, double>, std::less<>> word_to_id_freqs_;
    std::map<int, std::map<std::string, double>> id_to_word_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;

    // A valid word must not contain special characters
    static bool IsValidWord(const std::string_view word);

    // вспомогательная функция наличия стоп-слов, авторское решение
    bool IsStopWord(std::string_view word) const;

    // перебирает слова из ввода, исключает стоп-слова, возвращает вектор
    std::vector<std::string_view> SplitIntoWordsNoStop(std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    // Empty result by initializing it with default constructed QueryWord
    QueryWord ParseQueryWord(std::string_view text) const;


    // возвращает set строки ввода (запрос пользователя) без стоп-слов
    Query ParseQuery(std::string_view text) const;
    Query ParseQuery(std::execution::parallel_policy policy, std::string_view text) const;
    Query ParseQuery(std::execution::sequenced_policy policy, std::string_view text) const;

    // Existence required
    double ComputeWordInverseDocumentFreq(std::string_view word) const;

    // Для каждого документа возвращает его id и релевантность
    template <typename DocumentPredicate, typename Policy>
    std::vector<Document> FindAllDocuments(Policy&& policy, const Query& query_words, DocumentPredicate document_predicate) const;
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const;
};

template <typename StringCollection>
SearchServer::SearchServer(const StringCollection& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
    for (std::string_view word : stop_words_) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Text contain invalid symbols"s);
        }
    }
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const {
    const auto query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(query, document_predicate);

    sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
        if (std::abs(lhs.relevance - rhs.relevance) < 1e-6) {
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


template <typename DocumentPredicate, typename Policy>
std::vector<Document> SearchServer::FindTopDocuments(Policy&& policy, std::string_view raw_query, DocumentPredicate document_predicate) const {
    const auto query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(policy, query, document_predicate);

    sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
        if (std::abs(lhs.relevance - rhs.relevance) < 1e-6) {
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

template <typename Policy>
std::vector<Document> SearchServer::FindTopDocuments(Policy&& policy, std::string_view raw_query, DocumentStatus status) const {
    return SearchServer::FindTopDocuments(policy, raw_query, [status](int id, DocumentStatus document_status, int rating) {
        return document_status == status;
        });
}

template <typename Policy>
std::vector<Document> SearchServer::FindTopDocuments(Policy&& policy, std::string_view raw_query) const {
    return SearchServer::FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    for (std::string_view word : query.plus_words) {
        if (word_to_id_freqs_.count(std::string(word)) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(std::string(word));
        for (const auto& [document_id, term_freq] : word_to_id_freqs_.at(std::string(word))) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (std::string_view word : query.minus_words) {
        if (word_to_id_freqs_.count(std::string(word)) == 0) {
            continue;
        }
        for (const auto& [document_id, _] : word_to_id_freqs_.at(std::string(word))) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto& [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}

template <typename DocumentPredicate, typename Policy>
std::vector<Document> SearchServer::FindAllDocuments(Policy&& policy, const SearchServer::Query& query, DocumentPredicate document_predicate) const {
    ConcurrentMap<int, double> document_to_relevance(100);

    std::for_each(policy, query.plus_words.begin(), query.plus_words.end(), [&](std::string_view word) {
        if (word_to_id_freqs_.count(std::string(word)) > 0) {
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(std::string(word));
            for (const auto [document_id, term_freq] : word_to_id_freqs_.at(std::string(word))) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
                }
            }
        }
        });

    std::for_each(policy, query.minus_words.begin(), query.minus_words.end(), [&](std::string_view word) {
        if (word_to_id_freqs_.count(std::string(word)) > 0) {
            for (const auto [document_id, _] : word_to_id_freqs_.at(std::string(word))) {
                document_to_relevance.BuildOrdinaryMap().erase(document_id);
            }
        }
        });

    std::vector<Document> matched_documents;
    for (const auto& [document_id, relevance] : document_to_relevance.BuildOrdinaryMap()) {
        matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
    }

    return matched_documents;
}

template<typename Policy>
void SearchServer::RemoveDocument(Policy&& policy, int document_id) {
    const auto& words_freqs = GetWordFrequencies(document_id);
    if (!words_freqs.empty()) {
        std::vector<std::string> words;
        words.reserve(words_freqs.size());
        std::for_each(policy, words_freqs.begin(), words_freqs.end(), [&words](auto& wf) {
            words.push_back(std::string(wf.first));
            });

        std::for_each(policy, words.begin(), words.end(), [this, document_id](const auto& word) {
            return word_to_id_freqs_[word].erase(document_id);
            });
    }

    document_ids_.erase(document_id);
    documents_.erase(document_id);
    id_to_word_freqs_.erase(document_id);
}
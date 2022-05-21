#include "search_server.h"


SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text)) {
    if (!IsValidWord(stop_words_text)) {
        throw std::invalid_argument("Text contain invalid symbols"s);
    }
}

SearchServer::SearchServer(std::string_view stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text)) {
    if (!IsValidWord(stop_words_text)) {
        throw std::invalid_argument("Text contain invalid symbols"s);
    }
}


/*void SearchServer::SetStopWords(const std::string& text) {
    for (const std::string& word : SplitIntoWords(text)) {
        stop_words_.insert(word);
    }
}*/


void SearchServer::AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings) {
    if (document_id < 0) {
        throw std::invalid_argument("Negative ID"s);
    }
    if (documents_.count(document_id) > 0) {
        throw std::invalid_argument("ID out of range");
    }

    const auto& words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (const auto& word : words) {
        word_to_id_freqs_[std::string(word)][document_id] += inv_word_count;
        id_to_word_freqs_[document_id][std::string(word)] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    document_ids_.insert(document_id);
}


std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int id, DocumentStatus document_status, int rating) {
        return document_status == status;
        });
}


std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}


int SearchServer::GetDocumentCount() const {
    return documents_.size();
}


std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::string_view raw_query, int document_id) const {
    // Empty result by initializing it with default constructed tuple
    Query query = ParseQuery(raw_query);

    std::vector<std::string_view> matched_words;
    for (const std::string_view& word : query.minus_words) {
        if (word_to_id_freqs_.count(std::string(word)) == 0) {
            continue;
        }
        if (word_to_id_freqs_.at(std::string(word)).count(document_id)) {
            return { matched_words, documents_.at(document_id).status };
        }
    }

    for (const std::string_view& word : query.plus_words) {
        if (word_to_id_freqs_.count(std::string(word)) == 0) {
            continue;
        }
        if (word_to_id_freqs_.at(std::string(word)).count(document_id)) {
            matched_words.push_back(word);
        }
    }

    return { matched_words, documents_.at(document_id).status };
}


std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::sequenced_policy policy, std::string_view raw_query, int document_id) const {
    return SearchServer::MatchDocument(raw_query, document_id);
}


std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::parallel_policy policy, std::string_view raw_query, int document_id) const {
    const auto& query = ParseQuery(std::execution::par, raw_query);
    std::vector<std::string_view> matched_words{};

    if (std::any_of(std::execution::par, query.minus_words.begin(), query.minus_words.end(),
        [&](auto& word) {
            return word_to_id_freqs_.at(std::string(word)).count(document_id);
        })) {
        return { matched_words, documents_.at(document_id).status };
    }

    matched_words.reserve(query.plus_words.size());
    const auto& words_freqs = id_to_word_freqs_.at(document_id);

    if (words_freqs.empty()) {
        return { matched_words, documents_.at(document_id).status };
    }    

    std::for_each(std::execution::par, std::begin(words_freqs), std::end(words_freqs),
        [&query, &matched_words](auto& wf) {
            for (std::string_view str : query.plus_words) {
                if (str == wf.first) {
                    matched_words.push_back(str);
                }
            }
        });

    std::sort(matched_words.begin(), matched_words.end());
    auto unique_p = std::unique(matched_words.begin(), matched_words.end());
    matched_words.erase(unique_p, matched_words.end());

    return { matched_words, documents_.at(document_id).status };
}


/*int SearchServer::GetDocumentId(int index) const {
    if (index >= 0 && index < GetDocumentCount()) {
        return document_ids_[index];
    }
    throw std::out_of_range("Invalid index"s);
}*/


void SearchServer::RemoveDocument(int document_id) {
        if (id_to_word_freqs_.count(document_id) == 0) {
            return;
        }
        for (const auto& [word, freq] : id_to_word_freqs_.at(document_id)) {
            word_to_id_freqs_.at(word).erase(document_id);
        }
        id_to_word_freqs_.erase(document_id);
        documents_.erase(document_id);
        document_ids_.erase(std::find(document_ids_.begin(), document_ids_.end(), document_id));
}


const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const
{
    static std::map<std::string_view, double> result;
    result.clear();    
    for (const auto& [word, doc] : word_to_id_freqs_) {
        auto it = doc.find(document_id);
        if (it != doc.end())
        {
            result[word] = it->second;
        }
        else continue;
    }        return result;
}


std::set<int>::const_iterator SearchServer::begin() const {
    return document_ids_.begin();
}

std::set<int>::const_iterator SearchServer::end() const {
    return document_ids_.end();
}


bool SearchServer::IsValidWord(std::string_view word) {
    // A valid word must not contain special characters
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}


// вспомогательная функция наличия стоп-слов, авторское решение
bool SearchServer::IsStopWord(std::string_view word) const {
    return stop_words_.count(std::string(word)) > 0;
}


// перебирает слова из ввода, исключает стоп-слова, возвращает вектор
std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(std::string_view text) const {
    std::vector<std::string_view> words;
    for (std::string_view word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Text contain invalid symbols"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}


int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    int quantity = ratings.size();
    int sum = 0;
    if (ratings.empty()) {
        return 0;
    }
    sum = accumulate(ratings.begin(), ratings.end(), 0);
    return sum / quantity;
}


SearchServer::Query SearchServer::ParseQuery(std::string_view text) const {
    Query result;
    for (std::string_view& word : SplitIntoWords(text)) {
        const auto& query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            }
            else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    
    std::sort(result.minus_words.begin(), result.minus_words.end());
    auto minus_words = std::unique(result.minus_words.begin(), result.minus_words.end());
    result.minus_words.erase(minus_words, result.minus_words.end());

    std::sort(result.plus_words.begin(), result.plus_words.end());
    auto plus_word = std::unique(result.plus_words.begin(), result.plus_words.end());
    result.plus_words.erase(plus_word, result.plus_words.end());
    
    return result;
}


SearchServer::Query SearchServer::ParseQuery(std::execution::parallel_policy, std::string_view text) const {
    Query result;
    for (std::string_view& word : SplitIntoWords(text)) {
        const auto& query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            }
            else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    return result;
}


SearchServer::Query SearchServer::ParseQuery(std::execution::sequenced_policy policy, std::string_view text) const {
    return ParseQuery(text);
}


double SearchServer::ComputeWordInverseDocumentFreq(std::string_view word) const {
    return log(GetDocumentCount() * 1.0 / word_to_id_freqs_.at(std::string(word)).size());
}


SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
    if (text.empty()) {
        throw std::invalid_argument("String is empty"s);
    }

    bool is_minus = false;
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    if (text.empty() || text[0] == '-' || !IsValidWord(text)) {
        throw std::invalid_argument("Word(s) contain invalid symbols or invalid sintaxis"s);
    }
    return { text, is_minus, IsStopWord(text) };
}
#include "search_server.h"


SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text)) {
    if (!IsValidWord(stop_words_text)) {
        throw std::invalid_argument("Text contain invalid symbols"s);
    }
}


void SearchServer::SetStopWords(const std::string& text) {
    for (const std::string& word : SplitIntoWords(text)) {
        stop_words_.insert(word);
    }
}


bool SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {
    if (document_id < 0) {
        throw std::invalid_argument("Negative ID"s);
    }
    if (documents_.count(document_id) > 0) {
        throw std::invalid_argument("ID out of range");
    }

    const std::vector<std::string> words = SplitIntoWordsNoStop(document);

    const double inv_word_count = 1.0 / words.size();
    for (const std::string& word : words) {
        word_to_id_freqs_[word][document_id] += inv_word_count;
        id_to_word_freqs_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    document_ids_.insert(document_id);
    return true;
}


std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
        });
}


std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}


int SearchServer::GetDocumentCount() const {
    return documents_.size();
}


std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, int document_id) const {
    // Empty result by initializing it with default constructed tuple
    Query query = ParseQuery(raw_query);

    std::vector<std::string> matched_words;
    for (const std::string& word : query.plus_words) {
        if (word_to_id_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_id_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    for (const std::string& word : query.minus_words) {
        if (word_to_id_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_id_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }
    return std::tuple{ matched_words, documents_.at(document_id).status };
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


const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const {
    if (id_to_word_freqs_.count(document_id)) {
        return id_to_word_freqs_.at(document_id);
    }
    return {};
}


std::set<int>::const_iterator SearchServer::begin() const {
    return document_ids_.begin();
}

std::set<int>::const_iterator SearchServer::end() const {
    return document_ids_.end();
}


bool SearchServer::IsValidWord(const std::string& word) {
    // A valid word must not contain special characters
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}


// вспомогательная функция наличия стоп-слов, авторское решение
bool SearchServer::IsStopWord(const std::string& word) const {
    return stop_words_.count(word) > 0;
}


// перебирает слова из ввода, исключает стоп-слова, возвращает вектор
std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text)) {
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


[[nodiscard]] SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
    // Empty result by initializing it with default constructed Query
    Query result = {};
    for (const std::string& word : SplitIntoWords(text)) {
        QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.insert(query_word.data);
            }
            else {
                result.plus_words.insert(query_word.data);
            }
        }
    }
    return result;
}


double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
    return log(GetDocumentCount() * 1.0 / word_to_id_freqs_.at(word).size());
}


[[nodiscard]] SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const {
    QueryWord result = {};

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
#include <set>
#include <string>
#include <vector>

#include "remove_duplicates.h"


void RemoveDuplicates(SearchServer& search_server) {
    std::set<std::set<std::string_view>> docs;
    std::vector<int> ids_to_remove;
    for (int document_id : search_server) {
        std::set<std::string_view> keys;
        for (auto& [word, freq] : search_server.GetWordFrequencies(document_id)) {
            keys.emplace(word);
        }
        if (!docs.emplace(keys).second) {
            ids_to_remove.push_back(document_id);
        }
    }
    for (auto id : ids_to_remove) {
        std::cout << "Found duplicate document id " << id << std::endl;
        search_server.RemoveDocument(id);
    }
}
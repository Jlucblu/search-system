#include <set>
#include <string>
#include <vector>

#include "remove_duplicates.h"


void RemoveDuplicates(SearchServer& search_server) {
    std::set<std::set<std::string>> docs;
    std::vector<int> ids_to_remove;
    for (int document_id : search_server) {
        std::set<std::string> keys;
        for (const auto& [word, freq] : search_server.GetWordFrequencies(document_id)) {
            keys.insert(word);
        }
        if (!docs.insert(keys).second) {
            ids_to_remove.push_back(document_id);
        }
    }
    for (auto id : ids_to_remove) {
        std::cout << "Found duplicate document id " << id << std::endl;
        search_server.RemoveDocument(id);
    }
}
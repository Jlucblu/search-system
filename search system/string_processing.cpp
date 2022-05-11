#include "string_processing.h"


/*std::vector<std::string_view> SplitIntoWords(std::string_view text) {
    std::vector<std::string_view> result;
    const int64_t pos_end = text.npos;
    while (true) {
        int64_t space = text.find(' ');
        result.push_back(space == pos_end ? text.substr(0) : text.substr(0, space));
        text.remove_prefix(std::min(text.find_first_of(" ") + 1, text.size()));
        if (space == pos_end) {
            break;
        }
    }
    return result;
}*/

std::vector<std::string_view> SplitIntoWords(std::string_view text) {
    std::vector<std::string_view> words;
    std::string_view ttext = text; ttext.remove_prefix(std::min(ttext.size(), ttext.find_first_not_of(" ")));
    auto pos = ttext.find(' ', 0);
    for (; !ttext.empty();) {
        if (pos == 0) {
            ttext.remove_prefix(pos + 1);
            pos = ttext.find(' ', 0);
            continue;
        }
        words.push_back(ttext.substr(0, pos == ttext.npos ? ttext.npos : pos));
        if (pos == ttext.npos) break;
        ttext.remove_prefix(std::min(ttext.size(), pos + 1));
        pos = ttext.find(' ', 0);
    }
    return words;
}

/*std::vector<std::string> SplitIntoWords(const std::string& text) {
    std::vector<std::string> words;
    std::string word;
    for (const auto c : text) {
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
}*/
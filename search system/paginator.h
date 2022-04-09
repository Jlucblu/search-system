#pragma once
#include <iostream>
#include <vector>

#include "document.h"

using namespace std::string_literals;


template <typename Iterator>
class IteratorRange {
public:
    IteratorRange() = default;
    IteratorRange(Iterator begin, Iterator end)
        : begin_(begin)
        , end_(end)
        , size_(distance(begin_, end_)) {
    }

    auto begin() const {
        return begin_;
    }
    auto end() const {
        return end_;
    }
    auto size() const {
        return size_;
    }

private:
    Iterator begin_;
    Iterator end_;
    size_t size_;
};

template <typename Iterator>
class Paginator {
public:
    explicit Paginator(Iterator it_begin, Iterator it_end, size_t page_size)
        : vecIt_(SetPages(it_begin, it_end, page_size)) {}

    auto begin() const {
        return vecIt_.begin();
    }
    auto end() const {
        return vecIt_.end();
    }
    auto size() const {
        return vecIt_.size();
    }

private:
    std::vector<IteratorRange<Iterator>> vecIt_;
    std::vector<IteratorRange<Iterator>> SetPages(Iterator it_begin, Iterator it_end, size_t page_size) {
        std::vector<IteratorRange<Iterator>> vecIt;
        if (page_size > 0) {
            auto it = it_begin;
            auto it_next = it_begin;
            do {
                it_next = it + page_size;
                if (it_next > it_end) {
                    it_next = it_end;
                }
                vecIt.push_back({ it, it_next });
                it = it_next;
            } while (it_next < it_end);
        }
        return vecIt;
    }
};

template <typename Iterator>
std::ostream& operator<<(std::ostream& output, const IteratorRange<Iterator>& page) {
    for (auto it = page.begin(); it != page.end(); ++it) {
        output << *it;
    }
    return output;
}

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}

std::ostream& operator<<(std::ostream& output, const Document& doc) {
    output << "{ document_id = "s << doc.id
        << ", relevance = "s << doc.relevance
        << ", rating = "s << doc.rating
        << " }"s;
    return output;
}
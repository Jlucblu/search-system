#pragma once

#include "search_server.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <sstream>

using namespace std;


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

// func comparison vec
template <typename Container_First, typename Container_Second>
bool IsVectorsAreSimilar(const vector<Container_First>& vec_a, vector<Container_Second> vec_b) {
    if (vec_a.size() != vec_a.size()) {
        return false;
    }

    sort(vec_b.begin(), vec_b.end());

    for (const Container_First& element : vec_a) {
        if (!binary_search(vec_b.begin(), vec_b.end(), element)) {
            return false;
        }
    }

    return true;
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
    const string& hint);


#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))


template <typename Function>
void RunTestImpl(const Function& func, const string& function_name) {
    func();
    cerr << function_name << " OK" << endl;
}


#define RUN_TEST(func) RunTestImpl(func, #func)


int AverageRating(const vector<int>& ratings);

vector<string> SplitToWords(const string& line);

// _______________________________________ //

void TestSearchServer();
void TestExcludeStopWordsFromAddedDocumentContent();
void TestRating();
void TestAddDocument();
void TestMatchingDocuments();
void TestMinusWords();
void TestDocumentStatus();
void TestRelevance();
void TestPredicate();
void TestAverageRating();
void TestRemoveDuplicates();
void TestProcessQueries();
void TestRemoveDocuments();
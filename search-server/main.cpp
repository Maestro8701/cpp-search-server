#include <algorithm>  
#include <cmath>  
#include <iostream>  
#include <map> 
#include <numeric>  
#include <optional>  
#include <set>  
#include <stdexcept>   
#include <string>  
#include <utility>  
#include <vector>  
  
   
using namespace std;  
   
const int MAX_RESULT_DOCUMENT_COUNT = 5;  
const double EPSILON = 1E-06;  
   
string ReadLine() {  
    string s;  
    getline(cin, s);  
    return s;  
}  
   
int ReadLineWithNumber() {  
    int result;  
    cin >> result;  
    ReadLine();  
    return result;  
}  
   
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
   
struct Document {  
    Document() = default;  
   
    Document(int id, double relevance, int rating)  
        : id(id)  
        , relevance(relevance)  
        , rating(rating) {  
    }  
   
    int id = 0;  
    double relevance = 0.0;  
    int rating = 0;  
};  
   
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
   
enum class DocumentStatus {  
    ACTUAL,  
    IRRELEVANT,  
    BANNED,  
    REMOVED,  
};  
   
class SearchServer {  
public:  
   
   template <typename StringContainer>  
    explicit SearchServer(StringContainer stop_words)  
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {  
        for (const string& stop_word : stop_words_) {  
            if (!IsValidWord(stop_word)) {  
                throw invalid_argument("Stop word contains invalid characters");  
            }  
        }  
    }  
   
    explicit SearchServer(const string& stop_words_text)  
        : SearchServer(  
            SplitIntoWords(stop_words_text))  
    {  
    }  
   
    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {  
    if (document_id < 0) {  
        throw invalid_argument("Document ID cannot be negative");  
    }  
  
    if (find(documents_id_.begin(), documents_id_.end(), document_id) != documents_id_.end()) {  
        throw invalid_argument("Document with the same ID already exists");  
    }  
  
    const vector<string> words = SplitIntoWordsNoStop(document);  
  
    const double inv_word_count = 1.0 / words.size();  
    for (const string& word : words) {  
        word_to_document_freqs_[word][document_id] += inv_word_count;  
    }  
  
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });  
    documents_id_.push_back(document_id);  
}  
   
    int GetDocumentId(int index) const { 
    try { 
        return documents_id_.at(index); 
    } catch (const out_of_range& e) { 
        cerr << "Выход за пределы диапазона: " << e.what() << endl; 
        throw; 
    } 
} 
   
   template <typename DocumentPredicate>  
vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const {  
        const Query query = ParseQuery(raw_query);  
  
        auto matched_documents = FindAllDocuments(query, document_predicate);  
  
        sort(matched_documents.begin(), matched_documents.end(),  
            [](const Document& lhs, const Document& rhs) {  
                if (abs(lhs.relevance - rhs.relevance) < EPSILON) {  
                    return lhs.rating > rhs.rating;  
                } else {  
                    return lhs.relevance > rhs.relevance;  
                }  
            });  
  
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {  
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);  
        }  
  
        return matched_documents;  
}  
   
    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {  
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {  
        return document_status == status;  
    });  
}  
  
vector<Document> FindTopDocuments(const string& raw_query) const {  
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);  
}  
   
    int GetDocumentCount() const {  
        return documents_.size();  
    }  
   
    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {    
        const Query query = ParseQuery(raw_query);  
  
        vector<string> matched_words;  
        for (const string& word : query.plus_words) {  
            if (word_to_document_freqs_.count(word) == 0) {  
                continue;  
            }  
            if (word_to_document_freqs_.at(word).count(document_id)) {  
                matched_words.push_back(word);  
            }  
        }  
  
        for (const string& word : query.minus_words) {  
            if (word_to_document_freqs_.count(word) == 0) {  
                continue;  
            }  
            if (word_to_document_freqs_.at(word).count(document_id)) {  
                matched_words.clear();  
                break;  
            }  
        }  
  
        return make_tuple(matched_words, documents_.at(document_id).status);  
}  
   
private:  
    struct DocumentData {  
        int rating;  
        DocumentStatus status;  
    };  
    const set<string> stop_words_;  
    map<string, map<int, double>> word_to_document_freqs_;  
    map<int, DocumentData> documents_;  
    vector<int> documents_id_;  
   
    static bool IsValidWord(const string& word) {  
        return none_of(word.begin(), word.end(), [](char c) {  
            return c >= '\0' && c < ' ';  
            });  
    }  
   
    bool IsStopWord(const string& word) const {  
        return stop_words_.count(word) > 0;  
    }  
   
    vector<string> SplitIntoWordsNoStop(const string& text) const {  
        vector<string> words;  
        for (const string& word : SplitIntoWords(text)) {  
            if (!IsValidWord(word)) {              
                throw invalid_argument("Document text contains invalid characters");  
            }  
                if (!IsStopWord(word) ) {  
                    words.push_back(word);  
                }  
        }  
        return words;  
    }  
   
    static int ComputeAverageRating(const std::vector<int>& ratings) { 
    if (ratings.empty()) { 
        return 0; 
    } 
 
    int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0); 
 
    return rating_sum / static_cast<int>(ratings.size()); 
} 
   
    struct QueryWord {  
        string data;  
        bool is_minus;  
        bool is_stop;  
    };  
   
   QueryWord ParseQueryWord(string text) const {  
    if (text.empty()) {
        throw invalid_argument("Query word is empty");
    }
    bool is_minus = false;  
    if (text[0] == '-') {  
        is_minus = true;  
        text = text.substr(1);  
        if (text.empty() || text[0] == '-' || text.back() == '-') {
            throw invalid_argument("Minus word has invalid format: " + text);
        }
    }
    if (!IsValidWord(text)) {
        throw invalid_argument("Word contains invalid characters: " + text);
    }
    if (text.back() == '-') {
        throw invalid_argument("Word has '-' at the end: " + text);
    }
    if (IsStopWord(text)) {
    	return { text, is_minus, true };
	}
    return { text, is_minus, false};  
} 

   
    struct Query {  
        set<string> plus_words;  
        set<string> minus_words;  
    };  
   
    Query ParseQuery(const string& text) const {  
        Query query;  
        for (const string& word : SplitIntoWords(text)) {  
            const QueryWord query_word = ParseQueryWord(word);  
            if (!query_word.is_stop) {  
                if (query_word.is_minus) {  
                    query.minus_words.insert(query_word.data);  
                }  
                else {  
                    query.plus_words.insert(query_word.data);  
                }  
            }  
        }
        return query;  
    }  
   
    double ComputeWordInverseDocumentFreq(const string& word) const {  
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());  
    }  
   
    template <typename DocumentPredicate>  
    vector<Document> FindAllDocuments(const Query& query,  
        DocumentPredicate document_predicate) const {  
        map<int, double> document_to_relevance;  
        for (const string& word : query.plus_words) {  
            if (word_to_document_freqs_.count(word) == 0) {  
                continue;  
            }  
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);  
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {  
                const auto& document_data = documents_.at(document_id);  
                if (document_predicate(document_id, document_data.status, document_data.rating)) {  
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;  
                }  
            }  
        }  
   
        for (const string& word : query.minus_words) {  
            if (word_to_document_freqs_.count(word) == 0) {  
                continue;  
            }  
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {  
                document_to_relevance.erase(document_id);  
            }  
        }  
   
        vector<Document> matched_documents;  
        for (const auto [document_id, relevance] : document_to_relevance) {  
            matched_documents.push_back(  
                { document_id, relevance, documents_.at(document_id).rating });  
        }  
        return  matched_documents;  
    }  
};  
   
void PrintDocument(const Document& document) {  
    cout << "{ "s  
        << "document_id = "s << document.id << ", "s  
        << "relevance = "s << document.relevance << ", "s  
        << "rating = "s << document.rating << " }"s << endl;  
}  
int main() {  
    setlocale(LC_ALL, "Russian");  
    try {  
        SearchServer search_server("и в на"s);  
          
        (void)search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });  
        search_server.AddDocument(1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2 });  
          
          
        if (!search_server.FindTopDocuments("--пушистый"s).empty()) {  
    for (const Document& document : search_server.FindTopDocuments("--пушистый"s)) {  
        PrintDocument(document);  
    }  
} else {  
    cout << "Ошибка в поисковом запросе"s << endl;  
}  
    } catch (const invalid_argument& e) {  
        cerr << "Invalid argument: " << e.what() << endl;  
    } catch (const out_of_range& e) {  
        cerr << "Out of range: " << e.what() << endl;  
    } catch (...) {  
        cerr << "An unexpected error occurred." << endl;  
    }  
  
    return 0;  
}  

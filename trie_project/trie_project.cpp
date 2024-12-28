// Libraries for data structures
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <climits>
#include <string>

// Libraries for unit tests
#include <cassert>
#include <chrono>

// Libraries for UI
#include <conio.h>

using namespace std;
using namespace std::chrono;

// Text format
const string RESET = "\033[0m";
const string BOLD = "\033[1m";
const string UNDERLINE = "\033[4m";

// Text color
const string GREEN = "\x1b[38;5;10m";
const string YELLOW = "\x1b[38;5;11m";
const string RED = "\x1b[38;5;9m";
const string WHITE = "\x1b[38;5;15m";
const string GRAY = "\x1b[38;5;8m";

// Log function for colored output
void log(const string& message, const string& color = WHITE, const string& style = RESET) {
    cout << style << color << message << RESET << endl;
}

// Cache manager for storing the suggestions of prefixes
struct CacheNode {
    vector<string> suggestions;
    int frequency;

    CacheNode() : frequency(0) {}
};

class CacheManager {
private:
    unordered_map<string, CacheNode> cache;
    int size;
    int capacity;
	bool enableLogging;
public:
    CacheManager(int capacity) : size(0), capacity(capacity), enableLogging(true) {}

    int getSize() {
        return size;
    }

    int getCapacity() {
        return capacity;
    }

	void setLogging(bool enable) {
		enableLogging = enable;
	}

    // Insert a new prefix to the cache
    void insert(const string& prefix, const vector<string>& suggestions) {
		// Do not insert if there is no suggestion
		if (suggestions.empty()) return;

        CacheNode node;
        node.suggestions = suggestions;
        node.frequency = 1;

        cache[prefix] = node;
        size++;

		if (enableLogging) log("[Cache Manager]: Inserted prefix \"" + prefix + "\" with " + to_string(suggestions.size()) + " suggestions", GREEN);
    }

    // Update the prefix with new suggestions and frequency
    void update(const string& prefix, const vector<string>& suggestions, bool updateSuggestions = true, bool updateFrequency = true) {
        if (updateSuggestions) cache[prefix].suggestions = suggestions;
        if (updateFrequency) cache[prefix].frequency++;

        if (updateSuggestions) {
            if (enableLogging) log("[Cache Manager]: Updated suggestions for prefix \"" + prefix + "\"", YELLOW);
        }

        if (updateFrequency) {
            if (enableLogging) log("[Cache Manager]: Updated frequency for prefix \"" + prefix + "\"", YELLOW);
        }
    }

    // Get the suggestions of a prefix
    vector<string> get(const string& prefix) {
        return cache[prefix].suggestions;
    }

    void remove(const string& prefix) {
        cache.erase(prefix);
        size--;

        if (enableLogging) log("[Cache Manager]: Removed prefix \"" + prefix + "\"", RED);
    }

    // Remove all prefixes whose suggestions contain the word
    void removeItemByWord(const string& word) {
        for (auto it = cache.begin(); it != cache.end(); ) {
            auto& suggestions = it->second.suggestions;
            auto found = find(suggestions.begin(), suggestions.end(), word);

            if (found != suggestions.end()) {
                if (enableLogging) log("[Cache Manager]: Removed prefix \"" + it->first + "\"", RED);

                it = cache.erase(it);
                size--;
            }
            else {
                it++;
            }
        }
    }

    // Evict the least frequently used item
    void evict() {
        if (size > capacity) {
            string minFreqPrefix = "";
            int minFreq = INT_MAX;

            for (auto& item : cache) {
                if (item.second.frequency < minFreq) {
                    minFreqPrefix = item.first;
                    minFreq = item.second.frequency;
                }
            }

            cache.erase(minFreqPrefix);
            size--;

            if (enableLogging) log("[Cache Manager]: Evicted prefix \"" + minFreqPrefix + "\" with frequency " + to_string(minFreq), RED);
        }
    }

    void clearCache() {
        size = 0;

        if (enableLogging) log("[Cache Manager]: Cleared cache", RED);
    }

	~CacheManager() {
        clearCache();
	}
};

// Trie data structure for storing words
struct TrieNode {
    TrieNode* children[26];
    bool isEndOfWord;

	TrieNode() : isEndOfWord(false) {
        for (auto& child : children) child = nullptr;
    }
};

class Trie {
private:
    TrieNode* root;
    CacheManager* cache;
	bool enableLogging;

    bool hasWildcard(const string& word) {
        return word.find('.') != string::npos || word.find('[') != string::npos;
    }

    void removeHelper(const string& word, TrieNode*& current, int idx) {
        if (!current) {
			return;
        }

        // If the character reaches the end of word
        if (idx == word.size()) {
            if (current->isEndOfWord) {
                current->isEndOfWord = false;
            }

            // If the character is the actual end of word. In case the removing word
            // is shorter than an existing word, do not delete
            if (isEmpty(current)) {
                delete current;
                current = nullptr;
            }

            return;
        }

        int i = word[idx] - 'a';

        // Recursively iterate every character of the word
        removeHelper(word, current->children[i], idx + 1);

        // If the current character has not child (or its child was removed earlier)
        // and it is not the end of another word (this is the case which the removing word
        // is longer than an existing word)
        if (isEmpty(current) && !current->isEndOfWord) {
            delete current;
            current = nullptr;
        }
    }

    void suggestHelper(vector<string>& results, TrieNode* currentNode, string currentWord, int wordLimit) {
        comparisons++;
        if (!currentNode || results.size() >= wordLimit) {
            return;
        }

        // If the character is the end of an existing word, add it to the list
        comparisons++;
        if (currentNode->isEndOfWord) {
            results.push_back(currentWord);
        }

        // Explore all possible continuations
        for (int i = 0; i < 26; i++) {
			comparisons++;
            
            comparisons++;
            if (currentNode->children[i]) {
                // Append a character to make a new word
                currentWord.push_back('a' + i);
                suggestHelper(results, currentNode->children[i], currentWord, wordLimit);
                currentWord.pop_back();
            }
        }
        comparisons++;
    }

    void searchByRegex(vector<string>& results, string targetWord, TrieNode* currentNode, string currentWord, int& wordLimit) {
        if (!currentNode || results.size() >= wordLimit) return;

        // Iterate through every character of the target word
        for (int i = 0; i < targetWord.size(); i++) {
            char c = targetWord[i];

            // If the character is a wildcard, recursively search all possible continuations
            if (c == '.') {
                string sub = targetWord.substr(i + 1);

                for (int j = 0; j < 26; j++) {
                    if (currentNode->children[j]) {
                        currentWord.push_back('a' + j);
                        searchByRegex(results, sub, currentNode->children[j], currentWord, wordLimit);
                        currentWord.pop_back(); // Backtrack
                    }
                }

                // If the character is a wildcard, the search is done since all possible continuations are explored
                // and there is no need to continue searching for the next character in the target word.
                return;

            }
            else if (c == '[') {
                // If the character is a bracket, find the closing bracket
                bool exclude = false;
                int j = i + 1;

                // If the circumflex is not found, the target word is invalid
                if (j == targetWord.size()) {
                    if (enableLogging) log("[Trie]: Invalid regex: " + targetWord, RED);
                    return;
                }

                // If the first character inside the bracket is the circumflex, exclude the characters inside the bracket
                if (targetWord[j] == '^') {
                    exclude = true;
                    j++;
                }

                // Find the closing bracket
                while (j < targetWord.size() && targetWord[j] != ']') j++;

                // If the closing bracket is not found, the target word is invalid
                if (j == targetWord.size()) {
                    if (enableLogging) log("[Trie]: Invalid regex: " + targetWord, RED);
                    return;
                }

                // Extract the characters inside the bracket
                string sub = exclude ? targetWord.substr(i + 2, j - i - 2) : targetWord.substr(i + 1, j - i - 1);

                if (exclude) {
                    for (int k = 0; k < 26; k++) {
                        // If the character is not in the exclusion list and the child exists, search it
                        if (sub.find('a' + k) == string::npos && currentNode->children[k]) {
                            currentWord.push_back('a' + k);
                            searchByRegex(results, targetWord.substr(j + 1), currentNode->children[k], currentWord, wordLimit);
                            currentWord.pop_back();
                        }
                    }
                }
                else {
                    for (char c : sub) {
                        // If the character is in the inclusion list and the child exists, search it
                        if (currentNode->children[c - 'a']) {
                            currentWord.push_back(c);
                            searchByRegex(results, targetWord.substr(j + 1), currentNode->children[c - 'a'], currentWord, wordLimit);
                            currentWord.pop_back();
                        }
                    }
                }

                // The return is the same as the wildcard case
                return;

                // Otherwise, search the trie as usual
            }
            else {
                if (!currentNode->children[c - 'a']) return;

                currentWord.push_back(c);
                currentNode = currentNode->children[c - 'a'];
            }
        }

        // If the character is the end of an existing word, which means the target word is found
        // with the same length as the word in the trie, add it to the list
        if (currentNode->isEndOfWord) {
            results.push_back(currentWord);
        }
    }

    void fuzzySearchHelper(TrieNode* node, string& query, int maxDistance,
        const vector<int>& previousRow, string currentWord, vector<pair<string, int>>& results) {

        char nodeChar = currentWord.back();
        int numCols = query.size() + 1;
        vector<int> currentRow(numCols);

        // Update DP table for this character

        // The first entry in the current row is always the previous row's first entry + 1.
        // Think of a 2D matrix where the first row is the query and the first column is the word. 
        // Each cell represents the minimum number of operations to convert the prefix of the word to the prefix of the query.
        currentRow[0] = previousRow[0] + 1;

        // Assume that "i" is character index in the query string, and "j" is character index in the node string
        // "w1" is the query string, "w2" is the node string.
        // We start from (1,1) since (0,0) is already initialized (considering empty string).
        for (int col = 1; col < numCols; col++) {
            // In the insertion case, we try to insert a character at w2[j] to match w1[i-1] (which is just before the current character of w1)
            // This makes the character at w2[j] equal to w1[i-1], so we can shift i-1 to i and it costs 1 operation.
            int insertCost = currentRow[col - 1] + 1;

            // In the deletion case, we try to delete a character at w1[i] and shift to w1[i+1] to match w2[j].
            // Similar to the insertion case, we shift i to i+1 and it costs 1 operation.
            int deleteCost = previousRow[col] + 1;

            // In the replacement case, we try to replace the character at w2[j] with w1[i] to make them equal.
            // If two characters are already equal, it costs 0 operation. Otherwise, it costs 1 operation.
            int replaceCost = previousRow[col - 1] + (query[col - 1] != nodeChar ? 1 : 0);

            // Final cost is the minimum of the three operations
            currentRow[col] = min({ insertCost, deleteCost, replaceCost });
        }

        // If the last entry in the current row is within maxDistance and the node is a word, add it to the list
        if (currentRow.back() <= maxDistance && node->isEndOfWord) {
            results.push_back({ currentWord, currentRow.back() });
        }

        // Prune paths where the minimum edit distance exceeds maxDistance
        if (*min_element(currentRow.begin(), currentRow.end()) > maxDistance) {
            return;
        }

        // Recurse to children
        for (int i = 0; i < 26; ++i) {
            if (node->children[i]) {
                currentWord.push_back('a' + i);
                fuzzySearchHelper(node->children[i], query, maxDistance, currentRow, currentWord, results);
                currentWord.pop_back();
            }
        }
    }

    void clearTrie(TrieNode* &node) {
        if (!node) return;

        for (int i = 0; i < 26; i++) {
            if (node->children[i]) {
                clearTrie(node->children[i]);
            }
        }

        delete node;
        node = nullptr;
    }
public:
    int comparisons;

    Trie() : comparisons(0), enableLogging(true) {
        root = new TrieNode();
        cache = new CacheManager(10);
    }

    void setLogging(bool enable) {
		enableLogging = enable;
		cache->setLogging(enable);
    }

    void loadDictionary(const string& filename) {
        ifstream ifile(filename);
        string word;

        if (!ifile.is_open()) {
			if (enableLogging) log("[Trie]: Error opening file", RED);
            return;
        }

        if (enableLogging) log("Loading dictionary...", YELLOW);
        while (getline(ifile, word)) {
            // Assume that the word has no leading and trailing spaces
            if (word.empty()) continue;

            insert(word);
        }

        if (enableLogging) log("[Trie]: Dictionary loaded successfully", GREEN);

        ifile.close();
    }

    bool isEmpty(TrieNode* current) {
        for (int i = 0; i < 26; i++) {
            if (current->children[i]) return false;
        }

        return true;
    }

    void insert(const string& word) {
        TrieNode* current = root;

        for (char c : word) {
            int idx = c - 'a';

            if (!current->children[idx]) {
                current->children[idx] = new TrieNode();
            }

            current = current->children[idx];
        }

        current->isEndOfWord = true;

        // Update the cache by removing all prefixes whose suggestions contain the inserted word.
        // This is more efficient because the prefix will only be updated when it is searched again.
        // So there is no need to update all the cache immediately after inserting the word.
        cache->removeItemByWord(word);

        //log("Inserted word " + word, GREEN);
    }

    TrieNode* searchPrefix(const string& word) {
        TrieNode* current = root;

        for (auto& c : word) {
			comparisons++;
            // If the word being searched is longer than an existing word
            comparisons++;
            if (!current->children[c - 'a']) {
                return nullptr;
            }

            current = current->children[c - 'a'];
        }
		comparisons++;

        // If the word being searched exists in trie, there is no need to traverse further
        return current;
    }

    void remove(const string& word) {
        removeHelper(word, root, 0);

        // The principle is similar to insertion
        cache->removeItemByWord(word);

        // log("[Trie]: Removed word \"" + word + "\"", RED);
    }

    vector<string> suggest(const string& prefix, int wordLimit = 10) {
        // Check if the prefix is in the cache
		// Remove this part to test the trie without cache (used for performance testing)
        // ---------------------------------------------------------------------------------------------- //
        if (enableLogging) {
            vector<string> cachedSuggestions = cache->get(prefix);
            if (!cachedSuggestions.empty() && cachedSuggestions.size() >= wordLimit) {
                log("[Trie]: Found prefix \"" + prefix + "\" in cache", YELLOW);
                cache->update(prefix, cachedSuggestions, false, true);

                // Substring the cached suggestions to the word limit
                if (cachedSuggestions.size() > wordLimit) {
                    return vector<string>(cachedSuggestions.begin(), cachedSuggestions.begin() + wordLimit);
                }

                return cachedSuggestions;
            }
        }
		// ---------------------------------------------------------------------------------------------- //

        // If the prefix is not in the cache, search the trie
        bool isRegex = hasWildcard(prefix);
        vector<string> results;

        // If the prefix is not a regex, search the trie as usual
        if (!isRegex) {
            TrieNode* currentNode = searchPrefix(prefix);

			comparisons++;
            if (currentNode) {
                string currentWord = prefix;
                suggestHelper(results, currentNode, currentWord, wordLimit);
            }

            // Otherwise, search the trie by regex
        }
        else {
            string targetWord = prefix;
            string currentWord = "";
            searchByRegex(results, targetWord, root, currentWord, wordLimit);
        }

        // Update the cache
        // Remove this part to test the trie without cache (used for performance testing)
        // ---------------------------------------------------------------------------------------------- //
        if (enableLogging) {
            if (cache->getSize() > cache->getCapacity()) cache->evict();
            cache->insert(prefix, results);
        }
        // ---------------------------------------------------------------------------------------------- //

        return results;
    }

    vector<string> fuzzySearch(string& query, int maxDistance = 1, int wordLimit = 10) {
        // Check if the query is in the cache
        vector<string> cachedSuggestions = cache->get(query);
        if (!cachedSuggestions.empty() && cachedSuggestions.size() >= wordLimit) {
            log("[Trie]: Found query \"" + query + "\" in cache", YELLOW);
            cache->update(query, cachedSuggestions, false, true);

            // Substring the cached suggestions to the word limit
            if (cachedSuggestions.size() > wordLimit) {
                return vector<string>(cachedSuggestions.begin(), cachedSuggestions.begin() + wordLimit);
            }

            return cachedSuggestions;
        }

        // If the query is not in the cache, search the trie
        vector<pair<string, int>> results;
        vector<int> currentRow(query.size() + 1);

        // Initialize the first row of the DP table. The query starts from index 1 since index 0 represents an empty string.
        for (int i = 0; i <= query.size(); ++i) {
            currentRow[i] = i;
        }

        // Start recursive fuzzy matching
        string currentWord = "";
        for (int i = 0; i < 26; ++i) {
            if (root->children[i]) {
                currentWord.push_back('a' + i);
                fuzzySearchHelper(root->children[i], query, maxDistance, currentRow, currentWord, results);
                currentWord.pop_back();
            }
        }

        // Sort the results by Levenshtein distance
        sort(results.begin(), results.end(), [](const pair<string, int>& a, const pair<string, int>& b) {
            return a.second < b.second;
            });

        // Resize the results to the word limit
        vector<string> finalResults;
        for (int i = 0; i < results.size() && i < wordLimit; i++) {
            finalResults.push_back(results[i].first);
        }

        // Update the cache
        if (cache->getSize() > cache->getCapacity()) cache->evict();
        cache->insert(query, finalResults);

        return finalResults;
    }

    void releaseTrie() {
        clearTrie(root);
        cache->clearCache();
    }

    ~Trie() {
        releaseTrie();
    }
};

// Trie unit tests
class TrieUnitTests {
private:
    void runAllTests() {
        testInsertion();
        testSearchWithExistingWord();
        testSearchWithNonExistingWord();
        testRemoval();
        testEmptiness();
        testSuggestNoRegex();
        testSuggestWithRegex();
        testFuzzySearch();
        log("[Unit Test]: All tests passed", GREEN);
    }

    // Test the basic insertion of words
    void testInsertion() {
        Trie trie;

		trie.setLogging(false);

        // Insert a short word
        trie.insert("apple");
        assert(trie.searchPrefix("apple")->isEndOfWord == true);

        // Insert a long word
        trie.insert("pneumonoultramicroscopicsilicovolcanoconiosis");
        assert(trie.searchPrefix("pneumonoultramicroscopicsilicovolcanoconiosis")->isEndOfWord == true);

        log("[Unit Test]: Basic insertion: 2 test cases passed");
    }

    // Test searching with an existing word
    void testSearchWithExistingWord() {
        Trie trie;

        trie.setLogging(false);

        trie.insert("apple");
        trie.insert("banana");
        trie.insert("app");
        trie.insert("pneumonoultramicroscopicsilicovolcanoconiosis");

        assert(trie.searchPrefix("apple")->isEndOfWord == true);
        assert(trie.searchPrefix("banana")->isEndOfWord == true);
        assert(trie.searchPrefix("app")->isEndOfWord == true);
        assert(trie.searchPrefix("pneumonoultramicroscopicsilicovolcanoconiosis")->isEndOfWord == true);

        log("[Unit Test]: Search with existing word: 4 test cases passed");
    }

    // Test searching with a non-existing word
    void testSearchWithNonExistingWord() {
        Trie trie;

        trie.setLogging(false);

        trie.insert("hello");
        trie.insert("world");
        trie.insert("hell");

        assert(trie.searchPrefix("he")->isEndOfWord == false);
        assert(trie.searchPrefix("wor")->isEndOfWord == false);
        assert(trie.searchPrefix("")->isEndOfWord == false);

        log("[Unit Test]: Search with non-existing word: 3 test cases passed");
    }

    // Test removal of words
    void testRemoval() {
        Trie trie;

        trie.setLogging(false);

        trie.insert("hello");
        trie.insert("world");
        trie.insert("hell");

        trie.remove("hello");
        trie.remove("world");

        // Remove a non-existing word
        trie.remove("banana");

        assert(trie.searchPrefix("hello") == nullptr);
        assert(trie.searchPrefix("world") == nullptr);
        assert(trie.searchPrefix("hell")->isEndOfWord == true);

        log("[Unit Test]: Removal: 3 test cases passed");
    }

    // Test the emptiness of the trie
    void testEmptiness() {
        Trie trie;

        trie.setLogging(false);

        assert(trie.isEmpty(trie.searchPrefix("")) == true);

        trie.insert("hello");
        assert(trie.isEmpty(trie.searchPrefix("")) == false);

        log("[Unit Test]: Emptiness: 2 test cases passed");
    }

    // Test suggesting words without regex
    void testSuggestNoRegex() {
        Trie trie;

        trie.setLogging(false);

        trie.insert("apple");
        trie.insert("zebra");
        trie.insert("appetite");
        trie.insert("banana");
        trie.insert("app");
        trie.insert("application");
        trie.insert("japan");
        trie.insert("japanese");
        trie.insert("appetizer");
        trie.insert("appreciate");
        trie.insert("mango");
        trie.insert("apprehensive");

        vector<string> suggestions = trie.suggest("app", 5);

        assert(suggestions.size() == 5);
        assert(suggestions[0] == "app");
        assert(suggestions[1] == "appetite");
        assert(suggestions[2] == "appetizer");
        assert(suggestions[3] == "apple");
        assert(suggestions[4] == "application");

        log("[Unit Test]: Suggest without regex: 5 test cases passed");
    }

    // Test suggesting words with regex
    void testSuggestWithRegex() {
        Trie trie;

        trie.setLogging(false);

        trie.insert("abefg");
        trie.insert("abcdef");
        trie.insert("abefh");
        trie.insert("acefg");
        trie.insert("acefhi");
        trie.insert("acebh");
        trie.insert("acefh");
        trie.insert("adefh");
        trie.insert("aeefg");
        trie.insert("ajefh");
        trie.insert("axefyz");
        trie.insert("aaefaa");

        // [bcd] means any character in the set b, c, d
        vector<string> suggestions1 = trie.suggest("a[bcd]ef[gh]", 10);

        assert(suggestions1.size() == 5);
        assert(suggestions1[0] == "abefg");
        assert(suggestions1[1] == "abefh");
        assert(suggestions1[2] == "acefg");
        assert(suggestions1[3] == "acefh");
        assert(suggestions1[4] == "adefh");

        // [^bcd] means any character not in the set b, c, d
        vector<string> suggestions2 = trie.suggest("a[^bcd]ef[gh]", 10);

        assert(suggestions2.size() == 2);
        assert(suggestions2[0] == "aeefg");
        assert(suggestions2[1] == "ajefh");

        // . means any character
        vector<string> suggestions3 = trie.suggest("a.ef..", 10);

        assert(suggestions3.size() == 3);
        assert(suggestions3[0] == "aaefaa");
        assert(suggestions3[1] == "acefhi");
        assert(suggestions3[2] == "axefyz");

        log("[Unit Test]: Suggest with regex: 10 test cases passed");
    }

    // Test fuzzy search
    void testFuzzySearch() {
        Trie trie;

        trie.setLogging(false);

        trie.insert("apple");
        trie.insert("apa");
        trie.insert("banana");
        trie.insert("app");
        trie.insert("application");
        trie.insert("mango");
        trie.insert("apprehensive");
        trie.insert("car");
        trie.insert("clr");
        trie.insert("caw");
        trie.insert("carry");
        trie.insert("ear");
        trie.insert("ctr");

        string query1 = "app";
        vector<string> suggestions1 = trie.fuzzySearch(query1, 2, 5);

        assert(suggestions1.size() == 3);
        assert(suggestions1[0] == "app");
        assert(suggestions1[1] == "apa");
        assert(suggestions1[2] == "apple");

        string query2 = "car";
        vector<string> suggestions2 = trie.fuzzySearch(query2, 1, 5);

        assert(suggestions2.size() == 5);
        assert(suggestions2[0] == "car");
        assert(suggestions2[1] == "caw");
        assert(suggestions2[2] == "clr");
        assert(suggestions2[3] == "ctr");
        assert(suggestions2[4] == "ear");

        log("[Unit Test]: Fuzzy search: 8 test cases passed");
    }

    // Test cache manager
public:
    TrieUnitTests() {
        runAllTests();
    }
};

// Trie performance tests
class TriePerformanceTests {
private:
    Trie trie;

    void runAllTest() {
        pair<int, int> runtime_comparisons;
        int simulationPerCase = 10;
        int limits[] = { 100, 500, 1000, 5000, 10000 };

        // Test insertion
        for (int limit : limits) {
            int totalInsertionTime = 0;

            for (int i = 0; i < simulationPerCase; i++) {
				totalInsertionTime += testInsertion(limit);
            }

            log("[Performance Test]: Average insertion time with " + to_string(limit) + " words: " + to_string(totalInsertionTime / simulationPerCase) + " ms\n", GREEN);
        }

        // Test suggest
        int wordLimits[] = { 1, 5, 10, 15, 20 };

        for (int wordLimit : wordLimits) {
            for (int limit : limits) {
                int totalSuggestTime = 0;
				int comparisons = 0;

                for (int i = 0; i < simulationPerCase; i++) {
					runtime_comparisons = testSuggest(limit, wordLimit);
					totalSuggestTime += runtime_comparisons.first;
					comparisons += runtime_comparisons.second;
                }

                log("[Performance Test]: Average suggest time with " + to_string(limit) + " words and " + to_string(wordLimit) + " words limit: " + to_string(totalSuggestTime / simulationPerCase) + " ms", GREEN);
				log("[Performance Test]: Total comparisons: " + to_string(comparisons / simulationPerCase) + "\n", GREEN);
            }

        }

        // Test removal
        for (int limit : limits) {
            int totalRemovalTime = 0;

            for (int i = 0; i < simulationPerCase; i++) {
				totalRemovalTime += testRemoval(limit);
            }

            log("[Performance Test]: Average removal time with " + to_string(limit) + " words: " + to_string(totalRemovalTime / simulationPerCase) + " ms\n", GREEN);
        }

    }

    int testInsertion(int limit) {
        ifstream ifile("words_alpha.txt");
        string word;
        Trie trie;

		trie.setLogging(false);

        if (!ifile.is_open()) {
            log("[Performance Test]: Error opening file", RED);
            return -1;
        }

        vector<string> words;

        while (getline(ifile, word) && words.size() < limit) {
            words.push_back(word);
        }

        ifile.close();

        auto start = high_resolution_clock::now();

        for (const string& word : words) {
            trie.insert(word);
        }

        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(stop - start);

        log("[Performance Test]: Insertion of " + to_string(limit) + " words executed in " + to_string(duration.count()) + " ms");

        return duration.count();
    }

    pair<int, int> testSuggest(int limit, int wordLimit) {
        ifstream ifile("prefixes.txt");
        string prefix;

        if (!ifile.is_open()) {
            log("[Performance Test]: Error opening file", RED);
			return { -1, -1 };
        }

        vector<string> prefixes;

        while (getline(ifile, prefix) && prefixes.size() < limit) {
            prefixes.push_back(prefix);
        }

        ifile.close();


        auto start = high_resolution_clock::now();

        for (const string& prefix : prefixes) {
            trie.suggest(prefix, wordLimit);
            //cout << prefix << ": current comparisons: " << trie.comparisons << "\n";
        }

        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(stop - start);
		int comparisons = trie.comparisons;

		// Reset the comparison counter
		trie.comparisons = 0;

        log("[Performance Test]: Suggest of " + to_string(limit) + " words with " + to_string(wordLimit) + " words limit executed in " + to_string(duration.count()) + " ms");
		log("[Performance Test]: Total comparisons: " + to_string(comparisons));

		return { duration.count(), comparisons };
    }

    int testRemoval(int limit) {
        ifstream ifile("words_alpha.txt");
        string word;
        Trie trie;

		trie.setLogging(false);

        if (!ifile.is_open()) {
            log("[Performance Test]: Error opening file", RED);
            return  -1;
        }

        vector<string> words;

        while (getline(ifile, word) && words.size() < limit) {
            words.push_back(word);
        }

        ifile.close();

        for (const string& word : words) {
            trie.insert(word);
        }

        auto start = high_resolution_clock::now();

        for (const string& word : words) {
            trie.remove(word);
        }

        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(stop - start);

        log("[Performance Test]: Removal of " + to_string(limit) + " words executed in " + to_string(duration.count()) + " ms");

		return duration.count();
    }

public:
    TriePerformanceTests() {
		trie.setLogging(false);
        trie.loadDictionary("words_alpha.txt");
        runAllTest();

    }
};

// Sorted Array with Binary Search structure for storing words
class SortedArray {
private:
    vector<string> words;
	bool enableLogging;

    bool startsWith(const string& word, const string& prefix) {
        int n = word.size();
        int m = prefix.size();

		comparisons++;
        if (m > n) {
            return false;
        }

        for (int i = 0; i < m; i++) {
            if (word[i] != prefix[i]) {
                return false;
            }
            comparisons += 2;
        }
		comparisons++;

        return true;
    }

    // Function to find the range of words matching the prefix
    pair<int, int> findPrefixRange(const vector<string>& words, const string& prefix) {
        int n = words.size();
        int m = prefix.size();

        // Find the lower bound (first word that starts with the prefix)
        int low = 0, high = n - 1, start = -1;
        while (low <= high) {
            comparisons++;
            int mid = low + (high - low) / 2;

			comparisons++;
            if (words[mid] >= prefix) {
                high = mid - 1;
                start = mid;
            }
            else {
                low = mid + 1;
            }
        }

        // If no word starts with the prefix
        comparisons++;
        if (start == -1 || !startsWith(words[start], prefix)) {
            return { -1, -1 };
        }

        // Find the upper bound (last word that starts with the prefix)
        low = start;
        high = n - 1;
        int end = -1;
        while (low <= high) {
			comparisons++;
            int mid = low + (high - low) / 2;

            if (startsWith(words[mid], prefix)) {
                low = mid + 1;
                end = mid;
            }
            else {
                high = mid - 1;
            }
        }
		comparisons++;

        return { start, end };
    }

public:
	int comparisons;

	SortedArray() : comparisons(0), enableLogging(true) {}

    void setLogging(bool enable) {
		enableLogging = enable;
    }

    bool isEmpty() {
        return words.empty();
    }

    void loadDictionary(const string& filename) {
        ifstream ifile(filename);
        string word;

        if (!ifile.is_open()) {
            cerr << "Error opening file " << filename << endl;
            return;
        }

        if (enableLogging) log("Loading dictionary...", YELLOW);
        while (getline(ifile, word)) {
            // Assume that the word has no leading and trailing spaces
            if (word.empty()) continue;

            words.push_back(word);
        }

        sort(words.begin(), words.end());

        if (enableLogging) log("[Sorted Array]: Dictionary loaded successfully", GREEN);

        ifile.close();
    }

    void insert(const string& word) {
        // This function performs a binary search to find the first word in a sorted vector
        // that is greater than or equal to the given word.
        int idx = lower_bound(words.begin(), words.end(), word) - words.begin();

        // If the word already exists, do not insert
        if (idx < words.size() && words[idx] == word) {
            return;
        }

        // If the vector has enough capacity, it shifts all elements from the insertion point
        // to the right by one position to make space for the new word. If the vector does not
        // have enough capacity, it reallocates memory, copies all elements to the new memory
        // and inserts the word at the correct position.
        words.insert(words.begin() + idx, word);
    }

    void remove(const string& word) {
        int idx = lower_bound(words.begin(), words.end(), word) - words.begin();

        // If the word does not exist, do not remove
        if (idx >= words.size() || words[idx] != word) {
            return;
        }

        // After removing the word, it shifts all elements from the removal point to the left
        // and reduces the size of the vector.
        words.erase(words.begin() + idx);
    }

    vector<string> suggest(const string& prefix, int wordLimit = 10) {
        vector<string> results;
        pair<int, int> range = findPrefixRange(words, prefix);

	    comparisons++;
        if (range.first == -1) {
            return results;
        }

        for (int i = range.first; i <= range.second && results.size() < wordLimit; i++) {
            comparisons += 2;
            results.push_back(words[i]);
        }
		comparisons++;

        return results;
    }

	void releaseSortedArray() {
		words.clear();
	}

    ~SortedArray() {
		releaseSortedArray();
    }
};

// Sorted Array performance tests
class SortedArrayPerformanceTests {
private:
    SortedArray sortedArray;

    void runAllTest() {
		pair<int, int> runtime_comparisons;
        int simulationPerCase = 10;
        int limits[] = { 100, 500, 1000, 5000, 10000 };

        // Test insertion
        for (int limit : limits) {
            int totalInsertionTime = 0;
            for (int i = 0; i < simulationPerCase; i++) {
                totalInsertionTime += testInsertion(limit);
            }

            log("[Performance Test]: Average insertion time with " + to_string(limit) + " words: " + to_string(totalInsertionTime / simulationPerCase) + " ms\n", GREEN);
        }

        // Test suggest
        int wordLimits[] = { 1, 5, 10, 15, 20 };

        for (int wordLimit : wordLimits) {
            for (int limit : limits) {
                int totalSuggestTime = 0;
				int comparisons = 0;

                for (int i = 0; i < simulationPerCase; i++) {
					runtime_comparisons = testSuggest(limit, wordLimit);
					totalSuggestTime += runtime_comparisons.first;
					comparisons += runtime_comparisons.second;
                }

                log("[Performance Test]: Average suggest time with " + to_string(limit) + " words and " + to_string(wordLimit) + " words limit: " + to_string(totalSuggestTime / simulationPerCase) + " ms", GREEN);
				log("[Performance Test]: Total comparisons: " + to_string(comparisons / simulationPerCase) + "\n", GREEN);
            }
        }

        // Test removal
        for (int limit : limits) {
            int totalRemovalTime = 0;
            for (int i = 0; i < simulationPerCase; i++) {
                totalRemovalTime += testRemoval(limit);
            }

            log("[Performance Test]: Average removal time with " + to_string(limit) + " words: " + to_string(totalRemovalTime / simulationPerCase) + " ms\n", GREEN);
        }
    }

    int testInsertion(int limit) {
        ifstream ifile("words_alpha.txt");
        string word;
        SortedArray sortedArray;

		sortedArray.setLogging(false);

        if (!ifile.is_open()) {
            log("[Performance Test]: Error opening file", RED);
            return -1;
        }

        vector<string> words;

        while (getline(ifile, word) && words.size() < limit) {
            words.push_back(word);
        }

        ifile.close();

        auto start = high_resolution_clock::now();

        for (const string& word : words) {
            sortedArray.insert(word);
        }

        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(stop - start);

        log("[Performance Test]: Insertion of " + to_string(limit) + " words executed in " + to_string(duration.count()) + " ms");

        return duration.count();
    }

    pair<int, int> testSuggest(int limit, int wordLimit) {
        ifstream ifile("prefixes.txt");
        string prefix;

        if (!ifile.is_open()) {
            log("[Performance Test]: Error opening file", RED);
			return { -1, -1 };
        }

        vector<string> prefixes;

        while (getline(ifile, prefix) && prefixes.size() < limit) {
            prefixes.push_back(prefix);
        }

        ifile.close();

        auto start = high_resolution_clock::now();

        for (const string& prefix : prefixes) {
            sortedArray.suggest(prefix, wordLimit);
            //cout << prefix << ": current comparisons: " << sortedArray.comparisons << "\n";

        }

        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(stop - start);
		int comparisons = sortedArray.comparisons;
        
		// Reset the comparison counter
		sortedArray.comparisons = 0;

        log("[Performance Test]: Suggest of " + to_string(limit) + " words with " + to_string(wordLimit) + " words limit executed in " + to_string(duration.count()) + " ms");
		log("[Performance Test]: Total comparisons: " + to_string(comparisons));

        return { duration.count(), comparisons };
    }

    int testRemoval(int limit) {
        ifstream ifile("words_alpha.txt");
        string word;
        SortedArray sortedArray;

		sortedArray.setLogging(false);

        if (!ifile.is_open()) {
            log("[Performance Test]: Error opening file", RED);
            return -1;
        }

        vector<string> words;

        while (getline(ifile, word) && words.size() < limit) {
            words.push_back(word);
        }

        ifile.close();

        for (const string& word : words) {
            sortedArray.insert(word);
        }

        auto start = high_resolution_clock::now();

        for (const string& word : words) {
            sortedArray.remove(word);
        }

        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(stop - start);

        log("[Performance Test]: Removal of " + to_string(limit) + " words executed in " + to_string(duration.count()) + " ms");

        return duration.count();
    }

public:
    SortedArrayPerformanceTests() {
        sortedArray.loadDictionary("words_alpha.txt");
		sortedArray.setLogging(false);
        runAllTest();
    }

};

// UI class for the program
class UI {
private:
	string toLower(string str) {
		transform(str.begin(), str.end(), str.begin(), ::tolower);
		return str;
	}

	// Function to highlight the matching characters in a suggested word
    void logWordWithHighlight(string word, string query) {
        int i = 0, j = 0;
		string result = "";
        string wildCards = ".[]^";

		// Check if the query contains wildcards
		bool isRegex = false;
		for (char c : query) {
			if (wildCards.find(c) != string::npos) {
				isRegex = true;
				break;
			}
		}

		// If the query does not contain wildcards, highlight the matching characters
        if (!isRegex) {
            while (i < word.size() && j < query.size()) {
                if (word[i] == query[j]) {
                    result += word[i];
                    j++;
                }
                else {
                    result += GRAY + word[i] + RESET;
                }

                i++;
            }

            while (i < word.size()) {
                result += GRAY + word[i] + RESET;
                i++;
            }
        }
		// Otherwise, highlight the entire word
        else {
			result = word;
        }

		log(result);
    }

    void userMode() {
		// Load the dictionary into the trie
        Trie trie;

        trie.loadDictionary("words_alpha.txt");
        log("Dictionary loaded successfully! Press Enter to navigate to UI board.", GREEN);
        _getch();

        while (true) {
            system("cls");

			log("[ > ] : Choose an action, [1] to search, [2] to insert, [3] to remove, [4] to exit: ");
            int action = 0;
			cin >> action;

            if (action == 1) {
                log("[ > ] : Enter a word or a prefix to search, or [exit] to quit: ");
                string query;
                cin >> query;

                // Convert the query to lowercase
                query = toLower(query);

                if (query == "exit") break;

                log("[ > ] : Enter the number of suggestions to display, or [exit] to quit: ");
                int wordLimit;
                cin >> wordLimit;

                auto start = high_resolution_clock::now();
                vector<string> suggestions = trie.suggest(query, wordLimit);
                auto stop = high_resolution_clock::now();
                auto duration = duration_cast<milliseconds>(stop - start);

                if (suggestions.empty()) {
                    log("[ > ] : No suggestions found! Would you like to use fuzzy search? (y/n): ", YELLOW);
                    char choice;
                    cin >> choice;

                    if (choice == 'y') {
                        log("[ > ] : Enter the maximum character deviation: ");
                        int maxDistance;
                        cin >> maxDistance;

                        start = high_resolution_clock::now();
                        suggestions = trie.fuzzySearch(query, maxDistance, wordLimit);
                        stop = high_resolution_clock::now();
                        duration = duration_cast<milliseconds>(stop - start);

                        log("[ * ] : " + to_string(suggestions.size()) + " words matching '" + query + "' with a maximum deviation of " + to_string(maxDistance) + ":\n", GREEN);
                    }
                }
                else {
                    log("[ * ] : " + to_string(suggestions.size()) + " words matching '" + query + "':\n", GREEN);
                }

                for (const string& suggestion : suggestions) {
                    logWordWithHighlight(suggestion, query);
                }
                cout << endl;

                log("[ * ] : Search executed in " + to_string(duration.count()) + " ms. Press [Enter] to continue.", GREEN);
            }
            else if (action == 2) {
                log("[ > ] : Enter a word to insert, or [exit] to quit: ");
                string word;
                cin >> word;

                // Convert the word to lowercase
                word = toLower(word);

                if (word == "exit") break;

                trie.insert(word);
                log("[ * ] : Word inserted successfully! Press [Enter] to continue.", GREEN);
            }
            else if (action == 3) {
                log("[ > ] : Enter a word to remove, or [exit] to quit: ");
                string word;
                cin >> word;

                // Convert the word to lowercase
                word = toLower(word);

                if (word == "exit") break;

                trie.remove(word);
                log("[ * ] : Word removed successfully! Press [Enter] to continue.", GREEN);
            }
            else if (action == 4) {
                break;
            }
            else {
                log("[ ! ] : Invalid choice. Press [Enter] to try again.", RED);
            }

			_getch();
			cin.clear();
        }

		// Release the trie after the user exits
		trie.releaseTrie();
    }

    void trieTestMode() {
        system("cls");
        log("Enter [1] for unit tests, [2] for performance tests, [3] to exit: ");
        int choice;
        cin >> choice;

        try {
            if (choice == 1) {
                TrieUnitTests tests;
            }
            else if (choice == 2) {
                TriePerformanceTests tests;
            }
            else if (choice == 3) {
                return;
            }
            else {
                throw invalid_argument("Invalid choice");
            }
        }
        catch (const exception& e) {
            log("Tests failed: " + string(e.what()), RED);
        }

        log("[ * ] : All tests finished! Press Enter to continue.", GREEN);
        _getch();
    }

    void sortedArrayTestMode() {
        cout << "Enter [1] for performance tests, [2] to exit: ";
        int choice;
        cin >> choice;

        try {
            if (choice == 1) {
                SortedArrayPerformanceTests tests;
            }
            else if (choice == 2) {
                return;
            }
            else {
                throw invalid_argument("Invalid choice");
            }
        }
        catch (const exception& e) {
			log("Tests failed: " + string(e.what()), RED);
        }

        log("[ * ] : All tests finished! Press Enter to continue.", GREEN);
        _getch();
    }

public:
    UI() {

    }

    void run() {
        while (true) {
            system("cls");

            log("Enter [1] for user mode, [2] for trie tests, [3] for sorted array tests, [4] to exit: ");
            int choice;
            cin >> choice;

            if (choice == 1) {
                userMode();
            }
            else if (choice == 2) {
                trieTestMode();
            }
            else if (choice == 3) {
                sortedArrayTestMode();
            }
            else if (choice == 4) {
                break;
            }
            else {
                log("[ ! ] : Invalid choice. Press Enter to try again.", RED);
                _getch();
            }
        }
    }
};

int main() {
    // string keys[] = { "the", "a", "there",
    //                   "answer", "any", "by",
    //                   "bye", "their", "hero", "heroplane", "t"};
    // int n = sizeof(keys) / sizeof(keys[0]);

    // Trie trie;

    // for (int i=0; i<n; i++) {
    //     trie.insert(keys[i]);
    // }

    // Start the timer

    auto start = high_resolution_clock::now();

    //Trie trie;
    //trie.loadDictionary("words_alpha.txt");

    //vector<string> words = trie.suggest("aba", 5);

    //for (const string& s : words) {
    //    cout << s << endl;
    //}
    //cout << "Total comparisons: " << trie.comparisons << endl;

    //SortedArray sortedArray;
    //sortedArray.loadDictionary("words_alpha.txt");

    //words = sortedArray.suggest("aba", 5);

    //for (const string& s : words) {
    //    cout << s << endl;
    //}
    //cout << "Total comparisons: " << sortedArray.comparisons << endl;

    // cout << endl;

    // trie.remove("ababua");

    // words = trie.suggest("a.b.", 20);

    // for (auto& s : words) {
    //     cout << s << endl;
    // }

    // string query = "app";
    // vector<string> results = trie.fuzzySearch(query, 2, 10);
    // cout << results.size() << " words matching 'car':\n";

    // for (const string& result : results) {
    //     cout << result << endl;
    // }

    // try {
    //     TrieUnitTests tests;
    // } catch (const exception& e) {
    //     cerr << "Tests failed: " << e.what() << endl;
    // }

    // try {
    //     TriePerformanceTests tests;
    // } catch (const exception& e) {
    //     cerr << "Tests failed: " << e.what() << endl;
    // }

    // SortedArray sortedArray;
    // sortedArray.loadDictionary("words_alpha.txt");

    // vector<string> words = sortedArray.suggest("aba");

    // for (const string& s : words) {
    //     cout << s << endl;
    // }

    // try {
    //     SortedArrayPerformanceTests tests;
    // } catch (const exception& e) {
    //     cerr << "Tests failed: " << e.what() << endl;
    // }

    UI ui;
    ui.run();

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);
    cout << "Program executed in " << duration.count() << " ms" << endl;

    return 0;
}
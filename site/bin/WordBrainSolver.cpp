#include<iostream>
#include<fstream>
#include<string>
#include<sstream>
#include<vector>
#include<utility>
#include<queue>
#include<set>

using namespace std;

class TrieNode {
public:
    vector<TrieNode*> next{26, nullptr};
    string word;

    // insert a word into TrieNode
    // we may prefer this function per word in word list than buildTrieTree below beacuse these's no need to
    // hold the wordList vector of word list file in memeory
    void insert(const string &word) {
        TrieNode *curNode = this;
        for (char c: word) {
            int ind = c - 'a';
            if (curNode->next[ind] == nullptr)
                curNode->next[ind] = new TrieNode();
            curNode = curNode->next[ind];
        }
        // only when a TrieNode is the end of a word, the word variable in TrieNode is the word itself
        // otherwise the word variable is empty
        curNode->word = word;
    }
};

// build a trie tree according to the whole wordList
TrieNode *buildTrieTree(const vector<string> &wordList) {
    TrieNode *root = new TrieNode();
    for (const string &word: wordList) {
        TrieNode *curNode = root;
        for (char c: word) {
            int ind = c - 'a';
            if (curNode->next[ind] == nullptr)
                curNode->next[ind] = new TrieNode();
            curNode = curNode->next[ind];
        }
        // only when a TrieNode is the end of a word, the word variable in TrieNode is the word itself
        // otherwise the word variable is empty
        curNode->word = word;
    }
    return root;
}

// store puzzle's rowSize and columnSize in global variable
int rowSize = 0, columnSize = 0;
// define matrix type to simplify code
typedef vector<vector<char>> matrix;

// search the whole puzzle start at (rowIndex, columnIndex)
// return value is true when find a path matches targetStr, meanwhile resultStr is result word
// otherwise return value is false
bool dfsSearch(
    matrix &puzzle, 
    int rowIndex, 
    int columnIndex, 
    TrieNode *curTrieNode, 
    const string &targetStr, 
    int targetStrIndex, 
    vector<pair<string, matrix>> &rslt,
    bool isLast) {
    char targetChar = targetStr[targetStrIndex];
    char puzzleChar = puzzle[rowIndex][columnIndex];
    // if (targetStrIndex == 0) cout << puzzleChar << endl;
    // return when current char does not match existing character in targer word
    if (puzzleChar == '#')
        return false;
    if (targetChar != '*' && puzzleChar != targetChar)
        return false;
    // return when no matching prefix in trie
    int ind = puzzleChar - 'a';
    if (curTrieNode->next[ind] == nullptr)
        return false;
    
    // return true when the path match the whole word
    curTrieNode = curTrieNode->next[ind];
    if (!curTrieNode->word.empty()) {
        puzzle[rowIndex][columnIndex] = '#';
        if (isLast)
            rslt.push_back(pair<string, matrix>(curTrieNode->word, matrix()));
        // not only push match string into result
        // but also COPY current puzzle into result
        // from c11 may use move instead copy during implicit object
        else
            rslt.push_back(pair<string, matrix>(curTrieNode->word, puzzle));
        puzzle[rowIndex][columnIndex] = puzzleChar;
        return true;
    }

    // search adjacent cells in puzzle
    bool returnValue = false;
    puzzle[rowIndex][columnIndex] = '#';
    for (int dRow = (rowIndex > 0 ? -1 : 0); dRow <= (rowIndex < rowSize - 1 ? 1 : 0); dRow++) {
        for (int dColumn = (columnIndex > 0 ? -1 : 0); dColumn <= (columnIndex < columnSize - 1 ? 1 : 0); dColumn++) {
            if (dRow != 0 || dColumn != 0) {
                // cout << rowIndex + dRow << " " << columnIndex + dColumn << endl;
                bool search = dfsSearch(puzzle, rowIndex + dRow, columnIndex + dColumn, curTrieNode, 
                    targetStr, targetStrIndex + 1, rslt, isLast);
                returnValue |= search;
            }
        }
    }
    puzzle[rowIndex][columnIndex] = puzzleChar;
    return returnValue;
}

// drop elements in puzzle in order to fill all blank space
void drop(matrix &puzzle) {
    // o(rowSize * columnSize) method to calculate new puzzle after drop
    for (int columnIndex = 0; columnIndex < columnSize; columnIndex++) {
        // fast pointer j and slow pointer i
        int i = rowSize - 1;
        int j = rowSize - 1;
        while (true) {
            while (i >= 0 && puzzle[i][columnIndex] != '#') {
                i--;
                j--;
            }
            while (j >= 0 && puzzle[j][columnIndex] == '#') {
                j--;
            }
            if (j < 0) {
                break;
            }
            else {
                puzzle[i][columnIndex] = puzzle[j][columnIndex];
                puzzle[j][columnIndex] = '#';
            }
        }
    }
}

bool solve(
    matrix &puzzle, 
    const vector<TrieNode*> &triesBucket,
    const vector<string> &targets,
    vector<vector<string>> &rslt) {
    // init the state of current puzzle solver
    bool returnValue = false;
    rowSize = puzzle.size();
    columnSize = puzzle.size();
    int puzzleSize = rowSize * columnSize;
    int targetNum = targets.size();
    vector<int> targetLength;
    for (const string &s: targets) targetLength.push_back(s.size());
    // pruning when not have this length's trie tree
    if (triesBucket[targetLength[0] - 1] == nullptr) return false;
    
    // vector<string> -- current matching word list
    // matrix -- current state of matrix
    typedef pair<vector<string>, matrix> record;
    // use a queue instead of bfs
    queue<record> bfsQueue;
    // entrance = rowIndex * columnSize + columnIndex
    // init the queue
    vector<pair<string, matrix>> matchingResults;
    for (int entrance = 0; entrance < puzzleSize; entrance++) {
        if (dfsSearch(puzzle, entrance / columnSize, entrance % columnSize,
        triesBucket[targetLength[0] - 1], targets[0], 0, matchingResults, targetNum == 1)) {
            for (const auto &match: matchingResults) {
                bfsQueue.push(record(vector<string>(1, match.first), match.second));
            }
            matchingResults.clear();
        }
    }

    // start loop -- the same order as bfs
    while (!bfsQueue.empty()) {
        int curTargetNum = bfsQueue.front().first.size();
        if (curTargetNum == targetNum) {
            rslt.push_back(bfsQueue.front().first);
            returnValue = true;
        }
        else {
            // for(const auto &row: bfsQueue.front().second) {
            //     for (char c: row) cout << c;
            //     cout << endl;
            // }
            // cout << endl;
            if (triesBucket[targetLength[curTargetNum] - 1] == nullptr) {
                bfsQueue.pop();
                continue;
            }
            drop(bfsQueue.front().second);
            vector<pair<string, matrix>> matchingResults;
            for (int entrance = 0; entrance < puzzleSize; entrance++) {
                if (dfsSearch(bfsQueue.front().second, entrance / columnSize, entrance % columnSize,
                triesBucket[targetLength[curTargetNum] - 1], targets[curTargetNum], 0, matchingResults, curTargetNum == targetNum - 1)) {
                    for (const auto &match: matchingResults) {
                        vector<string> matchingWords(bfsQueue.front().first);
                        matchingWords.push_back(match.first);
                        bfsQueue.push(record(matchingWords, match.second));
                    }
                    matchingResults.clear();
                }
            }
        }
        bfsQueue.pop();
    }

    return returnValue;
}

int main(int argc, char *argv[]) {
    // exit if number of argument < 3
    if (argc < 3) {
        return 1;
    }
    char *smallListFileName = argv[1];
    char *largeListFileName = argv[2];

    ifstream smallListFS(smallListFileName);
    // exit if can not open samll_word_list file
    if (!smallListFS.is_open()) {
        return 2;
    }
    // build trie trees from samll word list
    // consider that the length of each target word is known
    // so build several trie trees and words in the same tree have same length
    // this is an important pre pruning step
    vector<TrieNode*> smallTriesBucket(30, nullptr);
    vector<TrieNode*> largeTriesBucket(30, nullptr);
    bool hasBuildLarge = false;
    string word;
    while (smallListFS >> word) {
        int bucketIndex = word.size() - 1;
        if (smallTriesBucket[bucketIndex] == nullptr) {
            smallTriesBucket[bucketIndex] = new TrieNode();
            smallTriesBucket[bucketIndex]->insert(word);
        }
        else {
            smallTriesBucket[bucketIndex]->insert(word);
        }
    }

    // puzzle matrix
    matrix puzzle;
    // loop and waiting for input until an EOF or CTRL+D
    string line;
    while (getline(cin, line)) {
        // exit when input an empty line
        if (line.empty()) {
            break;
        }
        // start solve a puzzle when this line is target hint line
        else if (line.find('*') != string::npos) {
            // use istringstream in order to split a string
            vector<string> targetList;
            istringstream ss(line);
            string word;
            while (ss >> word) targetList.push_back(word);
            // notice that dfsSearch function and matchTargets both use reference of vector to transfer result
            // the return value only show success or not
            // so we must initialize resultList first
            vector<vector<string>> solvedList;
            if (!solve(puzzle, smallTriesBucket, targetList, solvedList)) {
                // small word list can not solve this puzzle
                // initial large word list
                if (!hasBuildLarge) {
                    ifstream largeListFS(largeListFileName);
                    if (!largeListFS.is_open()) return 2;
                    while (largeListFS >> word) {
                        int bucketIndex = word.size() - 1;
                        if (largeTriesBucket[bucketIndex] == nullptr) {
                            largeTriesBucket[bucketIndex] = new TrieNode();
                            largeTriesBucket[bucketIndex]->insert(word);
                        }
                        else {
                            largeTriesBucket[bucketIndex]->insert(word);
                        }
                    }
                    hasBuildLarge = true;
                }
                solve(puzzle, largeTriesBucket, targetList, solvedList);
            }

            // start output solved answers
            // use an set to Deduplicate and output in dictionary order
            set<string> answerSet;
            for (const auto &answer: solvedList) {
                stringstream ss;
                for (int i = 0; i < answer.size() - 1; i++)
                    ss << answer[i] << " ";
                ss << answer[answer.size() - 1];
                answerSet.insert(ss.str());
            }
            for (const string &answerLine: answerSet) cout << answerLine << endl;
            cout << "." << endl;
            puzzle.clear();
        }
        // initialize a row in a puzzle
        else {
            vector<char> puzzleRow(line.begin(), line.end());
            puzzle.push_back(puzzleRow);
        }
    }

    return 0;
}
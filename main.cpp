#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <thread>

using namespace std;

const string char_order = "esaoriltnudpmychgbkfwvzjxq";
const int num_threads = 16;
mutex get_handler;
mutex put_handler;
int next_init = 0;

unsigned int encode_string(string word){
    unsigned int val = 0;
    for (char l : word){
        val = val | (1u << char_order.find(l));
    }
    return val;
} 

void CartesianRecurse(vector<vector<string>> &accum, vector<string> stack,
    vector<vector<string>> sequences, int index)
{
    vector<string> sequence = sequences[index];
    for (string i : sequence)
    {       
        stack.push_back(i);
        if (index == sequences.size() - 1)
            accum.push_back(stack);
        else
            CartesianRecurse(accum, stack, sequences, index + 1);
        stack.pop_back();
    }
}
vector<vector<string>> CartesianProduct(vector<vector<string>> sequences)
{
    vector<vector<string>> accum;
    vector<string> stack;
    if (sequences.size() > 0)
        CartesianRecurse(accum, stack, sequences, 0);
    return accum;
}


void dfs_search(vector<unsigned int> &current, 
                vector<unsigned int> candidates, 
                int target_len, 
                unsigned int restrict,
                map<unsigned int, vector<string>> &words,
                map<unsigned int, vector<string>> &answers, 
                fstream &output){
    if(target_len == 0){
        vector<vector<string>> decoded;
        decoded.push_back(answers[current[0]]);
        for (size_t i = 1; i < current.size(); i++){
            decoded.push_back(words[current[i]]);
        }
        vector<vector<string>> permuted = CartesianProduct(decoded);
        for (vector<string> sol : permuted){
            put_handler.lock();
                for (string w : sol){
                    output << w << "; ";
                }
                output << endl;
            put_handler.unlock();
        }
        
    } else {
        for(size_t i = 0; i < candidates.size(); i++){
            unsigned int word = candidates[i];
            unsigned int new_restrict = restrict | word;

            vector<unsigned int> new_candidates;
            for(size_t j = i + 1; j < candidates.size(); j++){
                if((new_restrict & candidates[j]) == 0){
                    new_candidates.push_back(candidates[j]);
                }
            }

            current.push_back(word);
            dfs_search(current, new_candidates, target_len - 1, new_restrict, words, answers, output);
            current.pop_back();
        }
    }
}

void threadfunc(vector<unsigned int> &inits, 
                vector<unsigned int> &candidates,
                map<unsigned int, vector<string>> &words,
                map<unsigned int, vector<string>> &answers, 
                fstream &output){
    get_handler.lock();
    while(next_init < inits.size()){
        unsigned int init = inits[next_init++];   
        cout << "Checking initial word " << next_init << "/" << inits.size() << endl;     
        get_handler.unlock();

        vector<unsigned int> initial{init};
        vector<unsigned int> new_candidates;
        for(size_t j = 0; j < candidates.size(); j++){
            if((init & candidates[j]) == 0){
                new_candidates.push_back(candidates[j]);
            }
        }
        dfs_search(initial, new_candidates, 6, init, words, answers, output);
        
        get_handler.lock();
    }
    get_handler.unlock();
}

int main(){
    string temp_string;

    fstream ansfs("wordle-answers.txt", fstream::in);
    map<unsigned int, vector<string>> answers;
    vector<unsigned int> initials;
    while(getline(ansfs, temp_string)){
        unsigned int encoding = encode_string(temp_string);
        answers[encoding].push_back(temp_string);
        initials.push_back(encoding);
    }
    ansfs.close();
    sort(initials.begin(),initials.end());
    initials.erase(unique(initials.begin(), initials.end()), initials.end());

    fstream wordfs("wordle-words.txt", fstream::in);
    map<unsigned int, vector<string>> words;
    vector<unsigned int> candidates;
    while(getline(wordfs, temp_string)){
        unsigned int encoding = encode_string(temp_string);
        words[encoding].push_back(temp_string);
        candidates.push_back(encoding);
    }
    wordfs.close();
    sort(candidates.begin(),candidates.end());
    candidates.erase(unique(candidates.begin(), candidates.end()), candidates.end());

    vector<thread> threads;
    fstream output("solution.txt", fstream::out | fstream::trunc);
    for(size_t i = 0; i < num_threads; i++){
        threads.emplace_back(threadfunc, ref(initials), ref(candidates), ref(words), ref(answers), ref(output));
    }

    for(size_t i = 0; i < num_threads; i++){
        threads[i].join();
    }
}
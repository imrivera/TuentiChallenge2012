
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <cctype>   // For isspace()
#include <cryptopp/md5.h>

using namespace std;

typedef map<char, string> trans_t;

trans_t final_transformations;

CryptoPP::Weak::MD5 hash_md5;


/* Split a string into a list o tokens using a delimiter */
vector<string> split_string(const string& str, const string& delim) {
    vector<string> tokens;

    size_t pos = 0;
    string token;

    while (pos < str.length()) {
        size_t end_pos = str.find(delim, pos);

        if (end_pos == string::npos)
            token = str.substr(pos, str.length() - pos);
        else
            token = str.substr(pos, end_pos - pos);

        // Don't allow empty tokens
        if (token.length())
            tokens.push_back(token);

        if (end_pos != string::npos)
            pos = end_pos + delim.length();
        else
            break;
    }

    return tokens;
}

/* Removes all spaces in a string */
void trim_string(string& s)
{
    for (size_t pos = 0; pos < s.length(); ) {
        if (isspace(s[pos]))
            s.erase(pos, 1);
        else 
            pos++;
    }
}

int main(void) {

    string queue;
    getline(cin, queue);

    vector<trans_t> transformations;

    while (!cin.eof()) {
        string s;
        getline(cin, s);

        trim_string(s);
        vector<string> t = split_string(s, ",");

        if (s.length() == 0)
            continue;
    
        trans_t people_and_their_clones;

        for (size_t i = 0; i < t.size(); i++) {
            vector<string> p2c = split_string(t[i], "=>");

            people_and_their_clones.insert(make_pair(p2c[0][0], p2c[1]));
                        
        }

        transformations.push_back(people_and_their_clones);        
    }

    set<char> set_of_people;

    for (size_t i = 0; i < queue.length(); i++)
        set_of_people.insert(queue[i]);

    for (set<char>::const_iterator it_person = set_of_people.begin(); it_person != set_of_people.end(); it_person++) {
        char person = *it_person;
        string clones;
        clones += person;

        for (size_t num_step = 0; num_step < transformations.size(); num_step++) {
            string new_clones;
            trans_t& this_transformation = transformations[num_step];

            for (size_t c = 0; c < clones.length(); c++) {
                
                trans_t::const_iterator it_new_clones = this_transformation.find(clones[c]);
                if (it_new_clones == this_transformation.end())
                    new_clones += clones[c];
                else
                    new_clones += it_new_clones->second;
            }

            clones = new_clones;
        }

        final_transformations.insert(make_pair(person, clones));
    }


    /* Calculate the MD5.
       For each person we update the MD5 with his final transformations
    */
    for (size_t i = 0; i < queue.length(); i++) {
        trans_t::const_iterator it = final_transformations.find(queue[i]);
        
        const string final_state = it->second;
        hash_md5.Update((const byte*)final_state.data(), final_state.length());
    }

    byte hash[CryptoPP::Weak::MD5::DIGESTSIZE];
    hash_md5.Final(hash);

    for (int i = 0; i < CryptoPP::Weak::MD5::DIGESTSIZE; i++)
        cout << hex << (unsigned int)hash[i];

    cout << endl;

    return 0;
}


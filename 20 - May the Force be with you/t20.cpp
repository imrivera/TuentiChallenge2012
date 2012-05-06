#include <pthread.h>
#include <iostream>
#include <sstream>
#include <openssl/des.h>
#include <openssl/evp.h>
#include <cstring>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <unistd.h>
#include <vector>

// *** Challenge 20: May the Force be with you ***

// 0 for autodetection
#define NUMBER_OF_CPUS 0

// Global variables used by the threads
volatile int key_found = 0;
int number_of_cpus = NUMBER_OF_CPUS;

unsigned char *global_ciphertext;
size_t global_ciphertext_size;


/* a-z 0-9
  This are the valid letters in the key. The lsb bit is ignored, so we don't need to
  check all the letters
*/                             
const char *letters = "acegikmoqsuwyz02468";
const int num_letters = 19;

using namespace std;


bool check_valid_game_message(unsigned char* buffer, size_t size) {
    const string magic="& Puzzle=";
    const char* p = magic.c_str();

    for (size_t i = 0; i < size - magic.size(); i++)
        if (memcmp(p, buffer + i, magic.size()) == 0) 
            return true;

    return false;
}

bool check_valid_aes_message(unsigned char* buffer, size_t size) {
    // The decrypted message should follow the RFC1423 padding
    
    bool valid = false;

    uint8_t padding = buffer[size-1];
    // The AES block size is 16 bytes
    if (padding > 1 && padding < 16) {
        valid = true;
        for (uint8_t i = 1; i < padding && valid; i++) {
                if (buffer[size - 1 - i] != padding)
                    valid = false;
        }
    }

    return valid;    
}


void* thread_decode(void *arg) {

    uint32_t start = *((uint32_t*)arg);
    uint64_t tid = (uint64_t) pthread_self();

    DES_key_schedule schedule;
    DES_cblock ascii_key;

    bool thread_key_found = false;

    memset(ascii_key, '0', 8);

    uint8_t key[4];

    uint32_t x = start;
    for (int i = 0; i < 4; i++, x /= num_letters) {       
        ascii_key[i] = letters[x % num_letters];
        key[i] = x;
    }

    unsigned char* buffer = new unsigned char[global_ciphertext_size + 1];

    while (!key_found) {

        DES_set_key_unchecked(&ascii_key, &schedule);
        DES_ecb_encrypt((DES_cblock*)(global_ciphertext) , (DES_cblock*)(buffer), &schedule, DES_DECRYPT);

        if (memcmp(buffer, "Key=", 4) == 0) {
            for (int j = 8; j < global_ciphertext_size; j += 8) {
                DES_ecb_encrypt((DES_cblock*)(global_ciphertext + j), (DES_cblock*)(buffer + j), &schedule, DES_DECRYPT);
            }

            if (check_valid_game_message(buffer, global_ciphertext_size)) {
                key_found = 1;
                thread_key_found = true;

                // Remove padding
                uint8_t padding = buffer[global_ciphertext_size - 1];
                if (padding >= 1 || padding <= 8) {
                    for (int j = 0; j < padding; j++) {
                        buffer[global_ciphertext_size - 1 - j] = 0x00;
                    }
                }
            }
        }

        // Increment the key in base 19. Only 4 "digits"
        int overflow = number_of_cpus;
        for (int i = 0; i < 4; i++) {
            int x = key[i] + overflow;
            overflow = x / num_letters;
            key[i] = x % num_letters;
            ascii_key[i] = letters[key[i]];
        }

        if (overflow > 0)   // We have ran out of keys to test
            break;
    }

    string *str = NULL;

    if(thread_key_found) {
        str = new string;
        *str = (const char*)buffer;
    }

    delete[] buffer;

    return str;
}


void hex2bin(const string& text, vector<uint8_t>& out) {
    out.clear();
    out.reserve(text.length() / 2);

    for(size_t i = 0; i < text.length(); i+=2) {
        const string byte = text.substr(i, 2);
        uint8_t x = strtoul(byte.c_str(), NULL, 16);
        out.push_back(x);
    }
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

string brute_force_DES(const string& ciphertext) {
    vector<uint8_t> binary_ciphertext;
    hex2bin(ciphertext, binary_ciphertext);

    global_ciphertext = binary_ciphertext.data();
    global_ciphertext_size = binary_ciphertext.size();

#if NUMBER_OF_CPUS == 0
    number_of_cpus = sysconf(_SC_NPROCESSORS_ONLN);
#endif

    uint32_t *start_list = new uint32_t[number_of_cpus];
    pthread_t *tid_list = new pthread_t[number_of_cpus];

    key_found = 0;
    memset(tid_list, 0, sizeof(pthread_t) * number_of_cpus);

    for (int i = 0; i < number_of_cpus && !key_found; i++) {
        start_list[i] = i;

        pthread_create(&tid_list[i], NULL, thread_decode, &start_list[i]);
    }

    string plaintext;
    
    for (int i = 0; i < number_of_cpus; i++) {
        if (tid_list[i] != 0) {
            void *retval;
            pthread_join(tid_list[i], &retval);
            if (retval) {
                plaintext = *((string*)retval);
                delete ((string*)retval);
            }
        }
    }

    delete[] start_list;
    delete[] tid_list;

    return plaintext;
}



string decrypt_AES(const string& ciphertext, const string& key) {

    vector<uint8_t> binary_ciphertext;

    hex2bin(ciphertext, binary_ciphertext);

    unsigned char *buffer = new unsigned char[binary_ciphertext.size() + 1];
    memset(buffer, 0, binary_ciphertext.size() + 1);

    EVP_CIPHER_CTX ctx;

    EVP_CIPHER_CTX_init(&ctx);
    EVP_DecryptInit_ex(&ctx, EVP_aes_128_ecb(), NULL, (const unsigned char*)key.data(), NULL);

    int outl = binary_ciphertext.size();
    EVP_DecryptUpdate(&ctx, buffer, &outl, binary_ciphertext.data(), binary_ciphertext.size());

    string plaintext;

    if (check_valid_aes_message(buffer, binary_ciphertext.size())) {
        uint8_t padding = buffer[binary_ciphertext.size() - 1];       
        if (padding >= 1 || padding <= 16) {
            for (int j = 0; j < padding; j++) {
                buffer[binary_ciphertext.size() - 1 - j] = 0x00;
            }
        }

        plaintext = (const char*)buffer;
    }

    return plaintext;
}


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


int main(void) {

    while(!cin.eof()) {
        string line;
        getline(cin, line);

        trim_string(line);
        if (line.length() == 0)
            continue;

        const vector<string> v1 = split_string(line, ":");
        const vector<string> des_messages = split_string(v1[1], "~");
        
        for(size_t i = 0; i < des_messages.size(); i++) {
            const string message = brute_force_DES(des_messages[i]);
            const string aes_key = message.substr(4,16);

            const string plaintext = decrypt_AES(v1[0], aes_key);
            if (plaintext.length() > 0) {
                cout << plaintext << endl;
                break;
            }
        }
    }    
}


string bin2hex(const void* buffer, size_t size)
{
    ostringstream ostr;
    unsigned char* b = (unsigned char*)buffer;

    for (size_t i = 0; i < size; i++)
    {
        
        ostr << hex << setfill('0') << setw(2) << (unsigned int)b[i] << " ";
    }

    return ostr.str();

}


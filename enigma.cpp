#include <iostream>
#include <cstdint>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include "enigma.h"

using namespace std;


enigma::enigma() : reflector_index(0){
    cout << "CTOR" << endl;

};

bool enigma::load_rotors_configs(string path, string reflector_path){
    ifstream infile(path, ios::in);
    if(!infile){
        cout << "File Error: can't open rotor config" << endl;
        return false;
    }
    string buff;
    char *temp1, *temp2;
    const char*buff_temp;
    for (int ri = 0; infile >> buff; ++ri)
    {
        cout << "rotor : " << ri << endl;
        temp1 = static_cast<char *>(malloc(26));
        temp2 = static_cast<char *>(malloc(26));
        rotors_set.push_back(temp1);
        inverse_rotors_set.push_back(temp2);
        buff_temp = buff.c_str();
        for (int i = 0; i < 26; ++i){
            /* 
                all input rotor should be upper case letters
                assign config to rotors_set
            */
            temp1[i] = buff_temp[i] - 65;
            temp2[buff_temp[i] - 65] = i;
        }
        cout << buff << endl;
    }
    infile.close();
    infile.open(reflector_path, ios::in);
    if(!infile){
        cout << "File Error: can't find reflector path" << endl;
        return false;
    }
    for (int ri = 0; infile >> buff; ++ri){
        temp1 = static_cast<char *>(malloc(26));
        reflector_set.push_back(temp1);
        buff_temp = buff.c_str();
        for (int i = 0; i < 26; ++i){
            /*
                due to reflector is valid for two direction,
                reflector only have one set.
            */
            temp1[i] = buff_temp[i] - 65;
        }
        cout << "reflector : " << ri << endl;
        cout << buff << endl;
    }
    num_of_rotors = rotors_set.size();
    return true;
}
void enigma::init_steps(){
    current_steps.assign(num_of_rotors, 0);
}
char enigma::single_enigma_calculate(int step, char c){
    int num_of_rotors_set = rotors_set.size();
    int step_arr[num_of_rotors_set];
    ++step;
    for (int i = num_of_rotors_set-1; i >= 0; --i){
        step_arr[i] = step % 26;
        step /= 26;
    }
    // phase - circuit from right to left
    for (int i = num_of_rotors_set - 1; i >= 0; --i){
        c = rotors_set[current_rotors_index[i]][(c + step_arr[i]) % 26] - step_arr[i];
        // by exp, test zero is five times faster than add and mod //
        if(c < 0)
            c += 26;
    }
    // phase - reflect
    c = reflector_set.at(reflector_index)[static_cast<unsigned char>(c)];
    // phase - circuit from left to right
    for(int i = 0; i < num_of_rotors_set; ++i){
        c = inverse_rotors_set[current_rotors_index[i]][(c + step_arr[i]) % 26] - step_arr[i];
        if(c < 0)
            c += 26;
    }
    return c;
}

string enigma::string_enigma_calculate(string plain){
    string cipher;
    for (int i = 0; i < plain.size(); ++i){
        cipher += static_cast<char>(single_enigma_calculate( i, plain[i]-65) + 65);
    }
    return cipher;
}

void enigma::printChar(char num){
    cout << static_cast<char>(num + 65);
}


int main(){
    enigma e;
    e.load_rotors_configs();
    cout << e.string_enigma_calculate("HELLO") << endl;
    return 0;
}
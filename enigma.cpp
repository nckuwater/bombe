#include <iostream>
#include <cstdint>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include "enigma.h"

using namespace std;

vector<char*> enigma::rotors_set = vector<char*>();
vector<char *> enigma::inverse_rotors_set = vector<char *>();
vector<char *> enigma::reflector_set = vector<char *>();

enigma::enigma() : reflector_index(0)
{
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
void enigma::select_rotors(vector<int> vec_of_index){
    selected_rotors_index = vec_of_index;
}

void enigma::set_init_steps(vector<int> input_init_steps){
    init_steps = input_init_steps;
    current_steps = init_steps;
}
vector<int> enigma::set_current_steps(int step){
    /* this function will add step to vector init_steps and store result to vector current_steps */
    //++step;
    vector<int> result(init_steps);
    for (int i = init_steps.size()-1; i >= 0; --i){
        result[i] += step;
        if(result[i] > 25){
            result[i] %= 26;
        }
        step /= 26;
    }
    current_steps = result;
    return result;
}
void enigma::add_one_step(){
    ++current_steps.at(current_steps.size() - 1);
    for (int i = current_steps.size() -2 ; i >= 0; --i){
        if(current_steps[i + 1] > 25){
            ++current_steps[i];
            current_steps[i + 1] -= 26;
        }
    }
    if(current_steps[0] > 25)
        current_steps[0] %= 26;
}
string enigma::plugboard_convert(string str){
    for (int i = 0; i < str.length(); ++i){
        str[i] = plugboard_set[str[i] - 65] + 65;
    }
    return str;
}

char enigma::single_enigma_calculate(int step, char c){
    int num_of_selected_rotors = selected_rotors_index.size();
    set_current_steps(++step);
    // phase - circuit from right to left
    for (int i = num_of_selected_rotors - 1; i >= 0; --i){
        c = rotors_set[selected_rotors_index[i]][(c + current_steps[i]) % 26] - current_steps[i];
        // by exp, test zero is five times faster than add and mod //
        if(c < 0)
            c += 26;
        
    }
    // phase - reflect
    
    c = reflector_set.at(reflector_index)[static_cast<unsigned char>(c)];
    // phase - circuit from left to right
    for(int i = 0; i < num_of_selected_rotors; ++i){
        
        c = inverse_rotors_set[selected_rotors_index[i]][(c + current_steps[i]) % 26] - current_steps[i];
        if(c < 0)
            c += 26;
    }
    return c;
}

string enigma::string_enigma_calculate(string plain){
    string cipher;
    cipher = plugboard_convert(cipher);
    cout << "plain : " << endl
         << plain << endl;
    for (int i = 0; i < plain.size(); ++i){
        //cipher += static_cast<char>(single_enigma_calculate( i, plain[i]-65) + 65);
        cipher += static_cast<char>(single_enigma_calculate( i, plain[i]-65) + 65);
    }
    return plugboard_convert(cipher);
}

void enigma::printChar(char num){
    cout << static_cast<char>(num + 65);
}
void enigma::print_vec(vector<int> vec){
    for (int i = 0; i < vec.size(); ++i){
        cout << vec[i] << " ";
    }
    cout << endl;
}


/*int main(){
    enigma e;
    e.load_rotors_configs();
    vector<int> selected_rotors({ 0, 1, 2 });
    e.select_rotors(selected_rotors);
    e.set_init_steps(vector<int>({0, 0, 0}));
    cout << e.string_enigma_calculate("HELLO") << endl;
    return 0;
}*/
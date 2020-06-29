#include <iostream>
#include <cstdint>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <ctime>
using namespace std;

class enigma{
    public:
        /* All wire value are index, not ascii letter. */
        vector<char*> rotors_set, inverse_rotors_set, reflector_set;
        int reflector_index;

        enigma() : reflector_index(0){
            cout << "CTOR" << endl;
        };

        bool load_rotors_configs(string path = "./rotors_config.txt", string reflector_path = "./reflector_config.txt"){
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
            return true;
        }
        char single_enigma_calculate(const int &step, unsigned char c){
            // phase - circuit from right to left
            int num_of_rotors_set = rotors_set.size(), temp_step = 1;
            int step_arr[num_of_rotors_set];
            for (int i = num_of_rotors_set-1; i >= 0; --i){
                step_arr[i] = (step / temp_step);
                temp_step *= 26;
            }
            for (int i = num_of_rotors_set - 1; i >= 0; --i){
                c = (rotors_set[i][(c + step_arr[i]) % 26] - step_arr[i]) % 26;
            }
            c = reflector_set.at(reflector_index)[c];
            for(int i = 0; i < num_of_rotors_set; ++i){
                c = inverse_rotors_set[i][(c + step_arr[i]) % 26];
            }
            return c;
        }
};

int main(){
    enigma e;
    e.load_rotors_configs();
    for (int i = 0; i < 10; ++i)
        cout << static_cast<char>(e.single_enigma_calculate(i, 'A') + 65) << endl;
    clock_t begin = clock();
    int num;
    num = -1;

    for (unsigned long long i = 0; i < 1000000000; ++i){
        
        if(num < 0){
            num += 26;
        }
    }
    cout << num - 100 << endl;
    cout << clock() - begin << endl;
    begin = clock();
    num = -1;

    for (unsigned long long i = 0; i < 1000000000; ++i){
        
        num += 26;
        num %= 26;
    }
    cout << num-100 << endl;
    cout << clock() - begin << endl;

    return 0;
}
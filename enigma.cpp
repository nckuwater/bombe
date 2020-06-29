#include <iostream>
#include <cstdint>
#include <vector>
#include <fstream>
#include <cstdlib>
using namespace std;

class enigma{
    public:
        vector<char*> rotors_set, inverse_rotors_set;

        bool load_rotors_configs(string path = "./rotors_config.txt"){
            ifstream infile(path, ios::in);
            if(!infile){
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
                    temp1[i] = buff_temp[i]-65;
                    temp2[buff_temp[i] - 65] = i;
                    cout << rotors_set.at(ri)[i];
                }
                cout << endl;
            }
            return true;
        }
        char calculate(char c){
            // phase - circuit from right to left
            int num_of_rotors_set = rotors_set.size();
            for (int i = num_of_rotors_set-1; i >= 0; --i){
                
            }
        }
};

int main(){
    enigma e;
    e.load_rotors_configs();
    return 0;
}
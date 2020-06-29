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
        enigma();
        bool load_rotors_configs(string path = "./rotors_config.txt", string reflector_path = "./reflector_config.txt");
        char single_enigma_calculate(const int &step, char c);
        string string_enigma_calculate(string plain);
        void printChar(char num);
};
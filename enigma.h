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
        static vector<char*> rotors_set, inverse_rotors_set, reflector_set;
        vector<int> current_rotors_index, current_steps;
        int reflector_index, num_of_rotors;
        static bool is_loaded_config_files;
        /* Functions */
        enigma();
        bool load_rotors_configs(string path = "./rotors_config.txt", string reflector_path = "./reflector_config.txt");
        void init_steps();
        char single_enigma_calculate(int step, char c);
        string string_enigma_calculate(string plain);
        void printChar(char num);
};
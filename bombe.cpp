#include <iostream>
#include <vector>
#include "enigma.h"

using namespace std;

class bombe_rotor{
    /* each bombe_rotor have one enigma circuit */
    public:
        enigma enigma_machine;
        int total_tested_init_steps_set;
        static vector<int[2]> bombe_menu[26];
        static vector<vector<int>> vec_loops;
        void add_text_to_bombe_menu(string plain, string cipher){
            const char *pptr = plain.c_str();
            const char *cptr = cipher.c_str();
            for (int i = 0; i < plain.size(); ++i){
                bombe_menu[pptr[i] - 65].push_back({i, cptr[i] - 65});
                bombe_menu[cptr[i] - 65].push_back({i, pptr[i] - 65});
            }
        }
        void find_loops(){

        }
        vector<int> sort_loop(const vector<int> &loop){
            int min_num = loop[0], min_index = 0;
            vector<int> new_loop;
            for (int i = 2; i < loop.size(); i+=2){
                if(loop[i] < min_num){
                    min_num = loop[i];
                    min_index = i;
                }
            }
            if(min_index == 0)
                return loop;
            new_loop.assign(loop.begin() + min_index, loop.end());
            new_loop.insert(new_loop.end() ,loop.begin() + 1, loop.begin() + min_index +1);
            for (int i = 0; i < new_loop.size(); ++i){
                cout << new_loop[i] << " ";
            }
            cout << endl;
            return new_loop;
        }


};

int main(){
    
    return 0;
}
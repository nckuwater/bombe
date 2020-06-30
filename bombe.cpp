#include <iostream>
#include <vector>
#include <array>
#include <fstream>
#include "enigma.h"

using namespace std;

class bombe_rotor{
    /* each bombe_rotor have one enigma circuit */
    public:
        enigma enigma_machine();
        int total_tested_init_steps_set;
        vector<int> remain_letters;
        static vector<array<int, 2>> bombe_menu[26];
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
            remain_letters.resize(25);
            vector<int> process;
            vector < array<int, 2>> *menu_ptr;
            vector<vector<int>> vec_process;
            int letter_index;
            for (int i = 1; i < 26; ++i){
                // A is not need to be add
                remain_letters[i] = i;
            }
            vec_process.assign(1, {0});
            while(!vec_process.empty())
                for (int i = 0; i < vec_process.size(); ++i){
                    letter_index = *vec_process.at(i).rbegin();
                    process = vec_process.at(i);
                    menu_ptr = &bombe_menu[letter_index];
                    vec_process.at(i).push_back(menu_ptr[0][0]);
                    vec_process.at(i).push_back(menu_ptr[0][1]);
                    for (int route = 1; route < bombe_menu[letter_index].size(); ++route){
                        process.push_back(bombe_menu)
                    }
                }
        }
        bool is_loop_exists(const vector<int> &loop){
            /* This function will compare in the vec_loops */
            for (int i = 0; i < vec_loops.size(); ++i){
                if(loop == vec_loops[i])
                    return true;
            }
            return false;
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
    bombe_rotor br;
    br.find_loops();
    return 0;
}
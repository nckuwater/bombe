#include <iostream>
#include <vector>
#include <array>
#include <fstream>
#include <algorithm>
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
            // array<int, 2> = {step, letter_index}
            const char *pptr = plain.c_str();
            const char *cptr = cipher.c_str();
            for (int i = 0; i < plain.size(); ++i){
                bombe_menu[pptr[i] - 65].push_back({i, cptr[i] - 65});
                bombe_menu[cptr[i] - 65].push_back({i, pptr[i] - 65});
            }
        }
        void find_loops(){
            remain_letters.resize(25);
            vector<int> process, base_process;
            vector < array<int, 2>> *menu_ptr;
            vector<vector<int>> vec_process;
            int letter_index, find_index;
            for (int i = 1; i < 26; ++i){
                // A is not need to be add
                remain_letters[i] = i;
            }
            vec_process.assign(1, {0});
            while(!vec_process.empty()){
                for (int i = 0; i < vec_process.size(); ++i){
                    letter_index = (vec_process.at(i).at(vec_process.at(0).size()-1));
                    menu_ptr = &bombe_menu[letter_index];
                    base_process = vec_process.at(i);
                    vec_process.erase(vec_process.begin() + i);
                    --i;
                    cout << i << " " << letter_index << endl;
                    //vec_process.at(i).push_back((*menu_ptr)[0][0]);
                    //vec_process.at(i).push_back((*menu_ptr)[0][1]);
                    for (int route = 0; route < (*menu_ptr).size(); ++route){
                        if((*menu_ptr).at(route)[0] == *(base_process.end()-2))
                            continue;
                        process = base_process;
                        process.push_back((*menu_ptr).at(route)[0]);
                        process.push_back((*menu_ptr).at(route)[1]);
                        for (int j = 0; j < process.size(); j+=2){
                            cout << static_cast<char>(process[j] + 65) << " : " << process[j] << endl;
                        }
                        find_index = is_value_exists((*menu_ptr)[route][1], process);
                        if (find_index != -1)
                        {
                            // find closed loop, append into vec_loops.
                            process = sort_loop(process);
                            if(!is_loop_exists(process))
                                vec_loops.push_back(vector<int>(process.begin() + find_index, process.end()));
                        }
                        else
                        {
                            // not form closed loop yet, append into vec_process and keep trying.
                            vec_process.push_back(process);
                        }
                    }
                }                
            }
            for (int i = 0; i < vec_loops.size(); ++i){
                for (int k = 0; k < vec_loops[i].size(); ++k){
                    if(k%2==0){
                        cout << static_cast<char>(vec_loops[i][k] + 65) << " ";
                    }
                    else{
                        cout << vec_loops[i][k] << " ";
                    }
                }
                cout << endl;
            }
        }
        int is_value_exists(int value, const vector<int> &loop){
            for (int i = 0; i < loop.size()-1; i+=2){
                /* check every letter value, not steps value */
                if(value == loop[i])
                    return i;
            }
            return -1;
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
            if(new_loop[1] > *(new_loop.end()-2))
                reverse(new_loop.begin(), new_loop.end());
            /*for (int i = 0; i < new_loop.size(); ++i){
                cout << new_loop[i] << " ";
            }
            cout << endl;*/
            return new_loop;
        }

};
vector<array<int, 2>> bombe_rotor::bombe_menu[] = {};
vector<vector<int>> bombe_rotor::vec_loops = vector<vector<int>>();
int main(){
    bombe_rotor br;
    for (int i = 0; i < 26; ++i){
        br.bombe_menu[i] = vector<array<int, 2>>(0);
    }
    br.add_text_to_bombe_menu("WSNPNLKLSTCS", "ATTACKATDAWN");
    br.find_loops();
    return 0;
}
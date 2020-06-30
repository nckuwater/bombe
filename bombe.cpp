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
        enigma enigma_machine;
        int total_tested_init_steps_set;
        bool test_finished;
        /* selected_rotors_index, init_step store enigma setting */
        vector<int> remain_letters, selected_rotors_index, init_steps, plugboard[26];
        vector<array<vector<int>, 26>> plugboard_possibilities;
        static vector<array<int, 2>> bombe_menu[26];
        static vector<vector<int>> vec_loops;
        static vector<string> vec_plain, vec_cipher;
        bool test_enigma_init_steps(){
            enigma_machine.init_steps = init_steps;
            for (int i = 0; i < 26; ++i){
                plugboard[i].resize(0);
            }
            int test_letter, target_letter;
            bool is_letter_match;// at least one letter pass all test //
            for (int loop_index = 0; loop_index < vec_loops.size(); ++loop_index){
                target_letter = *(vec_loops[loop_index].end());
                is_letter_match = false;
                for (int letter = 0; letter < 26; ++letter){
                    /* iter letter in loop, if no letter valid in loop, init_step assume wrong */
                    test_letter = letter;    
                    for (int route_index = 1; route_index < vec_loops[loop_index].size(); route_index+=2){
                        /* The enigma circuit process */
                        /* [route_index] = letter, [route_index + 1] = step */
                        test_letter = enigma_machine.single_enigma_calculate(vec_loops[loop_index][route_index], test_letter);
                    }
                    if(test_letter == letter){
                        //init_step first check valid, need plug board check
                        is_letter_match = true;
                        plugboard[letter].push_back(target_letter);
                        plugboard[target_letter].push_back(letter);
                    }
                }
                if(!is_letter_match){
                    /* if any one of loop has no letter match, means that the init_step set is wrong. */
                    return false;
                }
            }
            /* check if plugboard is valid */
            /* 
                because if only one possibility match, it must be the right one,
                if it conflict with other, means that the init_step set is wrong.
            */
            bool used_plug_table[26];
            for (int i = 0; i < 26; i++)
                used_plug_table[i] = false;
            bool is_any_set_valid;
            for (int i = 0; i < 26; ++i){
                if(plugboard[i].empty())
                    continue;
                if(plugboard[i].size() == 1){
                    is_any_set_valid = false;
                    for (int k = 0; k < plugboard[i].size(); ++k){
                        if(!(used_plug_table[i] || used_plug_table[plugboard[i][k]])){
                            /* if both of them are false */
                            is_any_set_valid = true;
                            used_plug_table[i] = used_plug_table[plugboard[i][k]] = true; 
                        }else{ // the only possible plug (certain one) conflict, means init_step set is wrong.
                            return false;
                        }
                    }
                }
            }
        }
        void add_one_init_step(){
            ++init_steps.at(init_steps.size() - 1);
            for (int i = init_steps.size() -2 ; i >= 0; --i){
                if(init_steps[i + 1] > 25){
                    ++init_steps[i];
                    init_steps[i + 1] -= 26;
                }else{
                    break;
                }
            }
            if(init_steps[0] > 25){
                test_finished = true;
                init_steps[0] %= 26;
            }
        }
        void add_text_to_bombe_menu(string plain, string cipher){
            // array<int, 2> = {step, letter_index}
            vec_plain.push_back(plain);
            vec_cipher.push_back(cipher);
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
            for (int i = 0; i < 26; ++i){
                remain_letters[i] = i;
            }
        
            for(int init_letter = 0; init_letter<26; ++init_letter){
                vec_process.assign(1, {init_letter});
                while(!vec_process.empty()){
                    letter_index = (vec_process.at(0).at(vec_process.at(0).size()-1));
                    menu_ptr = &bombe_menu[letter_index];
                    base_process = vec_process.at(0);
                    vec_process.erase(vec_process.begin());
                    //cout << "letter index: " << letter_index << endl;
                    //vec_process.at(i).push_back((*menu_ptr)[0][0]);
                    //vec_process.at(i).push_back((*menu_ptr)[0][1]);
                    for (int route = 0; route < (*menu_ptr).size(); ++route){
                        if((*menu_ptr).at(route)[0] == *(base_process.end()-2))
                            continue;
                        process = base_process;
                        process.push_back((*menu_ptr).at(route)[0]);
                        process.push_back((*menu_ptr).at(route)[1]);
                        /*for (int j = 0; j < process.size(); j+=2){
                            cout << static_cast<char>(process[j] + 65) << " : " << process[j] << endl;
                        }*/
                        find_index = is_value_exists((*menu_ptr)[route][1], process);
                        if (find_index != -1)
                        {
                            // find closed loop, append into vec_loops.
                            process.assign(process.begin() + find_index, process.end());
                            process = sort_loop(process);
                            if(!is_loop_exists(process))
                                vec_loops.push_back(process);
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
            if(min_index != 0){
                new_loop.assign(loop.begin() + min_index, loop.end());
                new_loop.insert(new_loop.end() ,loop.begin() + 1, loop.begin() + min_index +1);
            }
            else{
                new_loop = loop;
            }
            if(new_loop[1] > *(new_loop.end()-2)){
                reverse(new_loop.begin(), new_loop.end());
            }
            /*for (int i = 0; i < new_loop.size(); ++i){
                cout << new_loop[i] << " ";
            }
            cout << endl;*/
            return new_loop;
        }

};
vector<array<int, 2>> bombe_rotor::bombe_menu[] = {};
vector<vector<int>> bombe_rotor::vec_loops = vector<vector<int>>();
vector<string> bombe_rotor::vec_plain = vector<string>();
vector<string> bombe_rotor::vec_cipher = vector<string>();

int main(){
    bombe_rotor br;
    for (int i = 0; i < 26; ++i){
        br.bombe_menu[i] = vector<array<int, 2>>(0);
    }
    br.add_text_to_bombe_menu("WSNPNLKLSTCS", "ATTACKATDAWN");
    br.find_loops();
    return 0;
}
#include <iostream>
#include <vector>
#include <array>
#include <fstream>
#include <algorithm>
#include <iomanip>
#include <cstdlib>
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
        vector<array<int, 26>> plugboard_possibilities;
        static vector<array<int, 2>> bombe_menu[26];
        static vector<vector<int>> vec_loops;
        static vector<string> vec_plain, vec_cipher;
        bombe_rotor() : enigma_machine(){

        }
        bool test_enigma_init_steps(){
            /* This test should be call after set init_step */
            enigma_machine.init_steps = init_steps;
            for (int i = 0; i < 26; ++i){
                plugboard[i].resize(0);
            }
            int test_letter, target_letter, num_of_match_letter, last_letter;// last_letter is for confirm_case record
            bool is_letter_match;// at least one letter pass all test //
            array<int, 26> confirmed_plugs;
            confirmed_plugs.fill(-1);
            array<int, 26> minimum_founded_possibilitiy_for_plug;// one of the reduce method, record the minimum possibility of loops.
            minimum_founded_possibilitiy_for_plug.fill(26);
            for (int loop_index = 0; loop_index < vec_loops.size(); ++loop_index)
            {
                cout << loop_index << endl;
                target_letter = vec_loops[loop_index][0];
                //cout << "TLL: " << target_letter << endl;
                is_letter_match = false;
                num_of_match_letter = 0;
                for (int letter = 0; letter < 26; ++letter)
                {
                    /* iter letter in loop, if no letter valid in loop, init_step assume wrong */
                    test_letter = letter;
                    for (int route_index = 1; route_index < vec_loops[loop_index].size(); route_index += 2)
                    {
                        /* The enigma circuit process */
                        /* [route_index -1] = letter, [route_index] = step */
                        test_letter = enigma_machine.single_enigma_calculate(vec_loops[loop_index][route_index], test_letter);
                    }
                    
                    if (test_letter == letter)
                    {
                        //init_step first check valid, need plug board check
                        is_letter_match = true;
                        ++num_of_match_letter;
                        /*if (!is_value_in_vec(plugboard[letter], target_letter))
                            plugboard[letter].push_back(target_letter);*/
                        if (!is_value_in_vec(plugboard[target_letter], letter))
                            plugboard[target_letter].push_back(letter);
                        last_letter = letter;
                        //cout << "test" << endl;
                    }
                    else{
                        //cout << "not match" << endl;
                    }
                }
                if(!is_letter_match){
                    /* if any one of loop has no letter match, means that the init_step set is wrong. */
                    cout << "no letter match for one of loops" << endl;
                    return false;
                }
                if(num_of_match_letter == 1){//confirm case
                    if(!connect_plugboard(confirmed_plugs, target_letter, last_letter)){
                        cout << "confirmed_plug detect conflict" << endl;
                        return false;
                    }
                    //confirmed_plugs[target_letter] = true;
                    //confirmed_plugs[last_letter] = true;
                    //plugboard[target_letter].erase(plugboard[target_letter].begin(), plugboard[target_letter].end() - 1);
                    //plugboard[last_letter].erase(plugboard[last_letter].begin(), plugboard[last_letter].end() - 1);
                    //plugboard[target_letter].assign(1, last_letter);
                    //plugboard[last_letter].assign(1, target_letter);
                }else if(num_of_match_letter < minimum_founded_possibilitiy_for_plug[target_letter]){
                    minimum_founded_possibilitiy_for_plug[target_letter] = num_of_match_letter;
                    plugboard[target_letter].erase(plugboard[target_letter].end() - num_of_match_letter, plugboard[target_letter].end());
                }
            }
            cout << "confirmed plugs:" << endl;
            int temp_index;
            for (int i = 0; i < 26; ++i){
                cout << confirmed_plugs[i] << " ";
                if(confirmed_plugs[i] != -1){
                    //if(!is_value_in_vec(plugboard[i], confirmed_plugs[i]))
                        plugboard[i].assign(1, confirmed_plugs[i]);
                    //if(!is_value_in_vec(plugboard[confirmed_plugs[i]], i))
                        plugboard[confirmed_plugs[i]].assign(1, i);
                        for (int k = 0; k < 26; ++k){
                            temp_index = is_value_exists_full_check(i, plugboard[k]);
                            if(temp_index!=-1 && k!=i && k!=confirmed_plugs[i]){
                                plugboard[k].erase(plugboard[k].begin() + temp_index);
                                //cout << endl << "delete: " << i << " " << k << endl;
                            }
                            
                            temp_index = is_value_exists_full_check(confirmed_plugs[i], plugboard[k]);
                            if(temp_index!=-1 && k!=i && k!=confirmed_plugs[i]){
                                //cout << "delete: " << i << " " << k << endl;
                                plugboard[k].erase(plugboard[k].begin() + temp_index);
                            }
                        }
                }
            }
            cout << endl;
            /* check if plugboard is valid */
            /* 
                because if only one possibility match, it must be the right one,
                if it conflict with other, means that the init_step set is wrong.
            */
            cout << "checking first conflict" << endl;
            /*bool used_plug_table[26];
            for (int i = 0; i < 26; i++)
                used_plug_table[i] = false;
            bool is_any_set_valid;
            for (int i = 0; i < 26; ++i){
                // iter all plug of letter
                if(plugboard[i].empty())
                    continue;
                if(plugboard[i].size() == 1 && !used_plug_table[i]){
                    is_any_set_valid = false;
                    if(!(used_plug_table[i] || used_plug_table[plugboard[i][0]])){*/
                        /* if both of them are false */
                        /*is_any_set_valid = true;
                        used_plug_table[i] = used_plug_table[plugboard[i][0]] = true;
                    }else{ // the only possible plug (certain one) conflict, means init_step set is wrong.
                        cout << "first conflict check fail" << endl;
                        return false;
                    }
                }
            }*/
            cout << "spanning possibilities" << endl;
            /* do whole possibilities check by span all */
            for (int i = 0; i < 26; ++i){
                cout << i << " psize " << plugboard[i].size() << endl;
                print_vec(plugboard[i]);
            }
            
            for (int i = 0; i < 26; ++i)
            { // iter plugboard, replace empty by A~Z
                if (plugboard[i].empty())
                {
                    cout << "ADDDD" << endl;
                    plugboard[i].resize(26);
                    for (int k = 0; k < 26; ++k){
                        plugboard[i][k] = k;
                    }
                }
            }
            cout << endl;
            array<int, 26> base_arr, try_arr;
            base_arr.fill(-1);
            // set letter A possibility to be the init.
            plugboard_possibilities.assign(plugboard[0].size(), array<int, 26>());
            for (int i = 0; i < plugboard[0].size(); ++i){
                arrcpy(plugboard_possibilities[i], base_arr);
                connect_plugboard(plugboard_possibilities[i], 0, plugboard[0][i]);
            }
            int num_of_task = plugboard_possibilities.size();
            int temp_try_arr[2];
            /* Process: remove base_arr, and add new_arr to possibilities */
            for (int letter = 1; letter < 26; ++letter){
                cout << "tasks: " << num_of_task << endl;
                num_of_task = plugboard_possibilities.size();
                // tasks are those process with same connected plugs
                for (int task_index = 0; task_index < num_of_task; ++task_index){
                    //cout << "task_index: " << task_index << endl;
                    // store first possibility to base_arr and process later.
                    // base_arr now have define letter-1 plugs
                    arrcpy(base_arr, plugboard_possibilities[0]);
                    arrcpy(try_arr, base_arr);
                    //print_arr(base_arr);
                    plugboard_possibilities.erase(plugboard_possibilities.begin());
                    for (int k = 0; k < plugboard[letter].size(); ++k)
                    {
                        //base_arr[letter] = plugboard[letter][k];
                        /*if(plugboard[letter][k] == -1){
                            connect_plugboard(base_arr, letter, plugboard[letter][k]);
                        }
                        if (is_plugboard_valid(base_arr.data())){
                            plugboard_possibilities.push_back(array<int, 26>());
                            plugboard_possibilities[plugboard_possibilities.size() - 1] = base_arr;
                        }*/
                        //arrcpy(try_arr, base_arr); replaced by reset to base_arr.
                        // backup for resume to base_arr
                        temp_try_arr[0] = try_arr[letter];
                        temp_try_arr[1] = try_arr[plugboard[letter][k]];
                        if(is_value_in_vec(plugboard[plugboard[letter][k]], letter)){
                            if(connect_plugboard(try_arr, letter, plugboard[letter][k])){
                                /* no conflict during plug set */
                                plugboard_possibilities.push_back(array<int, 26>());
                                arrcpy(plugboard_possibilities[plugboard_possibilities.size() - 1], try_arr);
                                //cout << "try_arr " << letter << ":" << plugboard[letter][k] << endl;
                                ///print_arr(try_arr);
                                // resume to base_arr
                                try_arr[letter] = temp_try_arr[0];
                                try_arr[plugboard[letter][k]] = temp_try_arr[1];
                                }
                            else{

                            }
                        }
                        
                        
                    }
                    if(plugboard[letter].size()==0){
                        cout << "plugboard[letter[ size == 0" << endl;
                    }
                }
            }
            cout << "plugboard final print" << endl;
            print_plugboard_possibilities(plugboard_possibilities);
            cout << plugboard_possibilities.size() << endl;
            /* brute test all possibilities */
            bool bool_pass_all_text_test;
            for (int i = 0; i < plugboard_possibilities.size(); ++i){
                arrcpy(enigma_machine.plugboard_set, plugboard_possibilities[i]);
                bool_pass_all_text_test = true;
                for (int text_index = 0; text_index < vec_plain.size(); ++text_index){
                    if (!(enigma_machine.string_enigma_calculate(vec_plain[text_index]) == vec_cipher[text_index])){
                        cout << "plain, cipher test failed" << endl;
                        bool_pass_all_text_test = false;
                        break;
                    }
                }
                if(bool_pass_all_text_test){
                    cout << "final result: found right init step and rotors set" << endl;
                    print_arr(plugboard_possibilities[i]);
                    return true;
                }
            }
            return true;
        }
        inline void arrcpy(array<int, 26> &a, const array<int, 26> &b){
            for (int i = 0; i < 26; ++i)
                a[i] = b[i];
        }
        inline void arrcpy(array<bool, 26> &a, const array<bool, 26> &b){
            for (int i = 0; i < 26; ++i)
                a[i] = b[i];
        }
        inline bool connect_plugboard(array<int, 26> &arr, const int &a, const int &b){
            if(a!=b){
                if((arr[a] == -1) && (arr[b] == -1)){
                    arr[a] = b;
                    arr[b] = a;
                    return true;
                }
                if((arr[a]==b) && (arr[b]==a))
                    return true;
                return false;
            }
            else{
                if(arr[a] == a){
                    return true;
                }
                if(arr[a] == -1){
                    arr[a] = a;
                    return true;
                }
                return false;
            }
        }
        inline bool is_value_in_vec(const vector<int> &vec, const int &num){
            for (int i = 0; i < vec.size(); ++i){
                if(vec[i] == num)
                    return true;
            }
            return false;
        }
        inline void print_vec(const vector<int> &vec){
            for (int i = 0; i < vec.size(); ++i){
                cout << vec[i] << " ";
            }
            cout << endl;
        }
        inline void print_loop(const vector<int> &loop){
            for (int i = 0; i < loop.size(); ++i){
                if(i%2==0){
                    cout << static_cast<char>(loop[i] + 65);
                }else{
                    cout << loop[i];
                }
                cout << " ";
            }
            cout << endl;
        }
        inline void print_arr(const array<int, 26> &arr){
            for (int i = 0; i < 26;++i){
                cout << setw(2) << arr[i] << " ";
            }
            cout << endl;
        }
        bool is_plugboard_valid(int plugboard[26]){
            /*int bool_table_used[26];
            for (int i = 0; i < 26; i++){
                bool_table_used[i] = false;
            }*/
            for (int i = 0; i < 26; ++i){
                if(plugboard[i] == -1)
                    return true;
                if((plugboard[i]!=i)||(plugboard[plugboard[i]]!=i)){
                    /* connect each other or itself */
                    return false;
                }
            }
            return true;
        }
        inline void print_plugboard_possibilities(const vector<array<int, 26>> &pb){
            return;
            for (int i = 0; i<pb.size(); ++i){
                for (int k = 0; k < 26; ++k){
                    cout << k << "-" << pb[i][k] << " ";
                }
                cout << endl;
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
            remain_letters.resize(26);
            vector<int> process, base_process;
            vector<array<int, 2>> *menu_ptr;
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
                        find_index = is_letter_exists_in_loop((*menu_ptr)[route][1], process);
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
                            if(process.size()<=12)
                                vec_process.push_back(process);
                        }
                    }
                }
            }

            for (int i = 0; i < vec_loops.size(); ++i)
            {
                for (int k = 0; k < vec_loops[i].size(); ++k)
                {
                    if (k % 2 == 0)
                    {
                        cout << static_cast<char>(vec_loops[i][k] + 65) << " ";
                    }
                    else{
                        cout << vec_loops[i][k] << " ";
                    }
                }
                cout << endl;
            }
        }
        void reduce_vec_process(vector<vector<int>> &vec_process){
            /* goal: sort process and remove the same process */
        }
        void reorganize_loops(){
            /* 
                due to the plugboard relationship will all relative to the head letter of the loop 
                this function's goal is to make loops for every letter to make maximum data usage and the minimum possibilities try cost.
            */
            array<bool, 26> bool_head_letters, bool_letter_in_loop, spanable_letters;
            array<int, 26> arr_loop_for_head_letter;
            arr_loop_for_head_letter.fill(0);
            bool_head_letters.fill(false);
            bool_letter_in_loop.fill(false);
            for (int i = 0; i < vec_loops.size(); ++i){
                bool_head_letters[vec_loops[i][0]] = true;
                for (int k = 0; k < vec_loops[i].size(); ++k){
                    bool_letter_in_loop[vec_loops[i][k]] = true;
                }
            }
            cout << "reorganize_loops" << endl;
            for (int i = 0; i < 26; ++i){
                if(bool_head_letters[i]){
                    cout << static_cast<char>(i + 65) << " ";
                }
            }
            cout << endl;
            arrcpy(spanable_letters, bool_letter_in_loop);
            cout << "all letter in loop" << endl;
            for (int i = 0; i < 26; ++i){
                if(bool_letter_in_loop[i]){
                    cout << static_cast<char>(i + 65) << " ";
                    if(bool_head_letters[i]){
                        spanable_letters[i] = false;
                    }
                }
            }
            cout << endl;
            /* phase: span the letters in spanable_letters */
            vector<int> span_loop;
            for (int i = 0; i < 26; ++i){
                if(spanable_letters[i]){
                    for (int loop_index = 0; loop_index < vec_loops.size(); ++loop_index){
                        if(is_letter_exists_in_loop(i, vec_loops[loop_index]) != (-1)){
                            span_loop = change_loop_start_letter(vec_loops[loop_index], i);
                            if(span_loop.empty()){
                                cout << "find span_letter unexpected error";
                                return;
                            }
                            vec_loops.push_back(span_loop);
                            // current set one to be the goal num of loop for new spanned letter. so break here.
                            break;
                        }
                    }
                }
            }
            vector<vector<vector<int>>> vec_loops_for_letter(26);
            vector<int> temp_swap;
            for (int i = 0; i < vec_loops.size(); ++i){
                vec_loops_for_letter[vec_loops[i][0]].push_back(vec_loops[i]);
            }
            for (int i = 0; i < 26; ++i){
                for (int k = vec_loops_for_letter[i].size() - 1; k >= 0; --k)
                {
                    for (int j = 0; j < k; ++j){
                        if(vec_loops_for_letter[i][j].size()<vec_loops_for_letter[i][j+1].size()){
                            temp_swap = vec_loops_for_letter[i][j];
                            vec_loops_for_letter[i][j] = vec_loops_for_letter[i][j + 1];
                            vec_loops_for_letter[i][j + 1] = temp_swap;
                        }
                    }
                }
            }
            vec_loops.resize(0);
            for (int i = 0; i < 26; ++i){
                vec_loops.insert(vec_loops.end(), vec_loops_for_letter[i].begin(), vec_loops_for_letter[i].end());
            }

            for (int i = 0; i < vec_loops.size(); ++i)
            {
                if (arr_loop_for_head_letter[vec_loops[i][0]] >= 10)
                {
                    vec_loops.erase(vec_loops.begin() + i);
                    --i;
                }
                ++arr_loop_for_head_letter[vec_loops[i][0]];
            }
            /* may need to find a path for unloop letter to connet to those loops */
            cout << "num of loop for each letter after organize process." << endl;
            for (int i = 0; i < 26; ++i){
                cout << static_cast<char>(i + 65) << " : " << arr_loop_for_head_letter[i] << " ";
            }
            for (int i = 0; i < vec_loops.size(); ++i){
                cout << "loop " << i << endl;
                print_loop(vec_loops[i]);
            }
                cout << endl;
        }

        int is_letter_exists_in_loop(int value, const vector<int> &loop){
            /* This is a function for loop, not universal */
            /* equal to index function, return -1 if none match value */
            for (int i = 0; i < loop.size()-1; i+=2){
                /* check every letter value, not steps value */
                if(value == loop[i])
                    return i;
            }
            return -1;
        }
        int is_value_exists_full_check(int value, const vector<int> &loop){
            for (int i = 0; i < loop.size(); ++i){
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
            //int min_num = loop[0], min_index = 0;
            vector<int> new_loop;
            /*for (int i = 2; i < loop.size(); i+=2){
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
            }*/
            new_loop = loop;
            if(new_loop[1] > *(new_loop.end()-2)){
                reverse(new_loop.begin(), new_loop.end());
            }
            /*for (int i = 0; i < new_loop.size(); ++i){
                cout << new_loop[i] << " ";
            }
            cout << endl;*/
            return new_loop;
        }
        vector<int> change_loop_start_letter(const vector<int> &loop, const int &start_value){
            int index = -1;
            for (int i = 0; i < loop.size(); ++i){
                if(loop[i]==start_value){
                    index = i;
                    break;
                }
            }
            if(index == -1)
                return vector<int>(0);
            vector<int> new_loop;
            if(index != 0){
                new_loop.assign(loop.begin() + index, loop.end());
                new_loop.insert(new_loop.end() ,loop.begin() + 1, loop.begin() + index +1);
            }
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
    br.add_text_to_bombe_menu("HELLOWORLD", "ILBDAAMTAZ");
    br.add_text_to_bombe_menu("WORLDWARTWO", "KIXDIACTHJL");
    br.add_text_to_bombe_menu("WEATHERREPORT", "KLZFMNNTWLLLN");
    br.add_text_to_bombe_menu("ADDITIONALMSG", "BAJQXLMMLPTAH");
    br.add_text_to_bombe_menu("MOREANDMORE", "DIXWOEFNMXN");
    //br.add_text_to_bombe_menu("ABCDEFGHIJK", "BJELRQZVJWA");
    br.find_loops();
    br.reorganize_loops();
    cout << "find finished" << endl;
    br.enigma_machine.load_rotors_configs();
    br.enigma_machine.selected_rotors_index = {0, 1, 2};
    br.init_steps = {0, 0, 0};
    br.test_enigma_init_steps();
    
    return 0;
}
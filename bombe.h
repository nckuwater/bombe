#ifndef BOMBE_H
#define BOMBE_H

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
        array<int, 26> plugboard_result;
        /* static member datas */
        static vector<array<int, 2>> bombe_menu[26];
        static vector<vector<int>> vec_loops;
        static vector<string> vec_plain, vec_cipher;
        bombe_rotor();
        /* Basic data operations */
        inline void arrcpy(array<int, 26> &a, const array<int, 26> &b);
        inline void arrcpy(array<bool, 26> &a, const array<bool, 26> &b);
        /* Basic input operations */
        void load_text_from_file(string path = "./bombe_pc.txt");
        /* Basic output operations */
        inline void print_vec(const vector<int> &vec);
        inline void print_loop(const vector<int> &loop);
        inline void print_arr(const array<int, 26> &arr);
        inline void print_plugboard(const array<int, 26> &pb);
        inline void print_plugboard_possibilities(const vector<array<int, 26>> &pb);
        /* 
            Find loop functions 
        */
        void reorganize_loops();
        int is_letter_exists_in_loop(int value, const vector<int> &loop);
        int is_value_exists_full_check(int value, const vector<int> &loop);
        bool is_loop_exists(const vector<int> &loop);
        vector<int> sort_loop(const vector<int> &loop);
        vector<int> change_loop_start_letter(const vector<int> &loop, const int &start_value);
        /* 
            Combine plugboard funtions 
        */
        bool test_enigma_init_steps();

        inline bool is_value_in_vec(const vector<int> &vec, const int &num);
        inline bool connect_plugboard(array<int, 26> &arr, const int &a, const int &b);
        bool is_plugboard_valid(int plugboard[26]);
        void add_one_init_step();
        void add_text_to_bombe_menu(string plain, string cipher);
        void find_loops();
        void reduce_vec_process(vector<vector<int>> &vec_process);
};

#endif
//
// Created by aleksa on 3/10/26.
//

#pragma once
#include <iostream>
#include <string>
#include <limits>

class InputHandler {
public:
    bool get_input(const std::string& prompt, std::string& out_value) {
        std::cout << prompt;
        if (!(std::cin >> out_value)) {
            std::cerr << "Failed to read input.\n";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return false;
        }

        if (out_value.empty()) {
            std::cerr << "Input cannot be empty.\n";
            return false;
        }

        return true;
    }
};
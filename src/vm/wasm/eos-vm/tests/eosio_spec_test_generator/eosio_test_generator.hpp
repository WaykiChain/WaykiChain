#pragma once
#include <map>
#include <set>
#include <string>
#include <vector>

struct spec_test {
   std::string name;
   int assert_trap_start_index;
   int assert_trap_end_index;
   int assert_return_start_index;
   int assert_return_end_index;
};

std::string convert_to_valid_cpp_identifier(std::string val);

void write_tests(std::vector<spec_test> tests);

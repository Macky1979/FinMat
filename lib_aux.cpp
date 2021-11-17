#include<string>
#include <algorithm>

// convert string to an upper case
std::string to_upper(const std::string &input_str)
{
    // convert data type to upper case
    std::string _input_str = input_str;
    std::transform(_input_str.begin(), _input_str.end(), _input_str.begin(), ::toupper);
    
    // return upper-cased string
    return _input_str;
}

// convert string to an upper case
std::string to_lower(const std::string &input_str)
{
    // convert data type to upper case
    std::string _input_str = input_str;
    std::transform(_input_str.begin(), _input_str.end(), _input_str.begin(), ::tolower);
    
    // return upper-cased string
    return _input_str;
}
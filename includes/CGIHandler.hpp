#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include "WebServErr.hpp"
#include "LogSys.hpp"

class CGIHandler
{
private:
    std::vector<std::string> envp;
public:
    CGIHandler();
    CGIHandler(const CGIHandler &copy);
    ~CGIHandler();
    CGIHandler &operator=(const CGIHandler &copy);

    //Setters
    std::vector<std::string> setENVP(std::unordered_map<std::string, std::string> headers);

    //Getters
    void    getCGIOutput();
};

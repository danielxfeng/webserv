#pragma once

#include "WebServErr.hpp"
#include "LogSys.hpp"

class MethodHandler
{
private:
    
public:
    MethodHandler();
    MethodHandler(const MethodHandler &copy);
    ~MethodHandler();
    MethodHandler &operator=(const MethodHandler &copy);
};

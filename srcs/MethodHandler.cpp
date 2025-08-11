#include "../includes/MethodHandler.hpp"

MethodHandler::MethodHandler()
{}

MethodHandler::MethodHandler(const MethodHandler &copy)
{
    *this = copy;
}

MethodHandler::~MethodHandler()
{}

MethodHandler &MethodHandler::operator=(const MethodHandler &copy)
{
    if (this != &copy)
    {

    }
    return (*this);
}

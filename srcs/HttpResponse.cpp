#include "HttpResponse.hpp"
#include "HttpRequests.hpp"

std::vector<char> HttpResponse::responseSerializer(HttpRequests request,std::string &page)
{
    std::vector<char> result;
    std::cout<<"Response Part:"<<std::endl
            <<request.getHttpVersion()<<std::endl
            <<"Time: "<<getTime()<<std::endl
            <<page<<std::endl;
    return (result);
}

std::string HttpResponse::getTime()
{
    std::ostringstream oss; // here we create ostringstream.
	std::time_t result = std::time(nullptr);
    oss << std::asctime(std::localtime(&result));
    return (oss.str()); // return oss.str().
}
GIT 
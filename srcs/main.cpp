#include "WebServ.hpp"
#include "signalHandler.hpp"

bool validateConfigFile(std::string_view &fileName)
{
	size_t found;

	found = fileName.find(".json");
	if (found == std::string::npos)
	{
		std::cerr << "Error, you have to include the config file.";
		return (false);
	}
	if (found != (fileName.size() - 5))
	{
		std::cerr << "Error, file name is not correct.";
		return (false);
	}

	return (true);
}


int main(int argc, char **argv)
{
	if (argc != 2)
	{
		std::cerr << "Error, you have to include the config file.";
		return (1);
	}
	setup_signal_handlers();
	std::string_view fileName = argv[1];
	if (!validateConfigFile(fileName)){
		std::cerr << "Error, configuration file not found.";
		return (1);
	}
	auto webserv = WebServ(argv[1]);
	webserv.eventLoop();

	return (0);
}


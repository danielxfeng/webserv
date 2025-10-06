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
	/**
	try
	{
		std::string_view fileName = argv[1];
		if (!validateConfigFile(fileName))
			throw std::invalid_argument("Invalid configuration file.");
		auto webserv = WebServ(argv[1]);
		webserv.eventLoop();
	}
	catch (const std::exception &e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return (1);
	}
	*/
	std::string_view fileName = argv[1];
	if (!validateConfigFile(fileName))
		throw std::invalid_argument("Invalid configuration file.");
	auto webserv = WebServ(argv[1]);
	webserv.eventLoop();

	return (0);
}

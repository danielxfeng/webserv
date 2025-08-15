#include "../includes/utils.hpp"

void    checkIfRegFile(const std::string &path)
{
    if (!std::filesystem::is_regular_file(path))
	{
		LOG_ERR("File is not a regular file: ", path);
		throw WebServErr::MethodException(OTHER, "File is not a regular file");
	}
}

void    checkIfDirectory(const std::string &path)
{
    if (std::filesystem::is_directory(path))
	{
		LOG_WARN("Target is a Directory: ", path);
		throw WebServErr::MethodException(ERR_403, "Target is a directory");
	}
}

void    checkIfLocExists(const std::string &path)
{
    if (!std::filesystem::exists(path))
	{
		LOG_ERR("Permission Denied: Cannot Delete: ", path);
		throw WebServErr::MethodException(OTHER, "Permission Denied, cannot delete selected file");
	}
}

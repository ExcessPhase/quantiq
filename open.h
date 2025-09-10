#pragma once
#include <system_error>
#include <cerrno>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


namespace foelsche
{
namespace linux
{
	/// an RAII wrapper for socket()/close()
struct open
{	const int m_i;
	open(const char*const _pFileName, const int _iFlags)
		:m_i(::open(_pFileName, _iFlags))
	{	if (m_i == -1)
			throw std::system_error(std::error_code(errno, std::generic_category()), "open() failed!");
	}
	~open(void)
	{	close(m_i);
	}
};
}
}

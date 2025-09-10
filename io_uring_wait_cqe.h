#pragma once
#include <liburing.h>
#include <system_error>
#include <cerrno>
#include <new>


namespace foelsche
{
namespace linux
{
	/// an RAII wrapper for the uring call of the same name
struct io_uring_wait_cqe
{	struct ::io_uring *const m_pRing;
	::io_uring_cqe *const m_pCQE;
	static ::io_uring_cqe *getCQE(struct ::io_uring *const _p)
	{	::io_uring_cqe *pCQE;
		const auto i = ::io_uring_wait_cqe(_p, &pCQE);
		if (i < 0)
			throw std::system_error(std::error_code(-i, std::generic_category()), "io_uring_wait_cqe() failed!");
		return pCQE;
	}
	io_uring_wait_cqe(struct ::io_uring *const _p)
		:m_pRing(_p),
		m_pCQE(getCQE(_p))
	{
	}
	~io_uring_wait_cqe(void)
	{	::io_uring_cqe_seen(m_pRing, m_pCQE);
	}
};
}
}

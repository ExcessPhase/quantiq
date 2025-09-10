#include "io_uring_queue_init.h"
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <regex>
#include <cstring>
#include <filesystem>


namespace foelsche
{
namespace linux
{
namespace
{
	/// a read request (from file)
struct io_data_read:io_data
{	//const std::shared_ptr<io_data_created_buffer> m_sBuffer;
	std::vector<char> m_sBuffer;
	std::vector<char> m_sBuffer2;
	std::function<void(io_data&, foelsche::linux::io_uring_queue_init*const , ::io_uring_cqe* const)> m_sRead;
	const int m_iFD;

	io_data_read(
		foelsche::linux::io_uring_queue_init *const _pRing,
		int _iFD,
		std::vector<char> &&_rBuffer,
		std::vector<char> &&_rBuffer2,
		std::function<void(io_data&, foelsche::linux::io_uring_queue_init*const , ::io_uring_cqe* const)>&&_rRead
	)
		://io_data(_rFD),
		m_sBuffer(std::move(_rBuffer)),
		m_sBuffer2(std::move(_rBuffer2)),
		m_sRead(std::move(_rRead)),
		m_iFD(_iFD)
	{	//const auto sFD = std::dynamic_pointer_cast<io_data_created_fd>(m_sData);
		//fcntl(sFD->m_iID, F_SETFL, fcntl(sFD->m_iID, F_GETFL, 0) | O_NONBLOCK);
		io_uring_sqe* const sqe = io_uring_queue_init::io_uring_get_sqe(&_pRing->m_sRing);
		sqe->user_data = reinterpret_cast<uintptr_t>(this);
		const auto iOffset = m_sBuffer.size();
		m_sBuffer.resize(iOffset + 16*1024);
		io_uring_prep_read(sqe, m_iFD, m_sBuffer.data() + iOffset, m_sBuffer.size() - iOffset, -1);
		io_uring_sqe_set_data(sqe, this);
		io_uring_submit(&_pRing->m_sRing);
	}
	virtual void handle(io_uring_queue_init*const _pRing, ::io_uring_cqe* const _pCQE) override
	{	m_sRead(*this, _pRing, _pCQE);
	}
	virtual std::vector<char> &getBuffer(void) override
	{	return m_sBuffer;
	}
	virtual std::vector<char> &getBuffer2(void) override
	{	return m_sBuffer2;
	}
	virtual int getFD(void) override
	{	return m_iFD;
	}
	virtual std::size_t getOffset(void) override
	{	return 0;
	}
	virtual std::function<void(io_data&, foelsche::linux::io_uring_queue_init*const , ::io_uring_cqe* const, bool)> getWrite(void) override
	{	return nullptr;
	}
	virtual std::function<void(io_data&, foelsche::linux::io_uring_queue_init*const , ::io_uring_cqe* const)> getRead(void) override
	{	return std::move(m_sRead);
	}
#if 0
	{	auto p = m_sBuffer->m_s.begin(), pEnd = m_sBuffer->m_s.end(), pLast = p;
		for (; p != pEnd; ++p)
			if (*p == '\n')
			{	std::reverse(pLast, p);
				pLast = p + 1;
			}
		_pRing->createWrite(
		);
	}
#endif
};
	/// a receive request (from socket)
struct io_data_write:io_data
{	std::vector<char> m_sBuffer;
	std::vector<char> m_sBuffer2;
	const std::size_t m_iOffset;
	std::function<void(io_data&, foelsche::linux::io_uring_queue_init*const , ::io_uring_cqe* const, bool)> m_sWrite;
	const int m_iFD;
	io_data_write(
		foelsche::linux::io_uring_queue_init *const _pRing,
		int _iFD,
		std::vector<char> &&_rBuffer,
		std::vector<char> &&_rBuffer2,
		std::size_t _iOffset,
		std::function<void(io_data&, foelsche::linux::io_uring_queue_init*const , ::io_uring_cqe* const, bool)>&&_rWrite
	)
		:
		m_sBuffer(std::move(_rBuffer)),
		m_sBuffer2(std::move(_rBuffer2)),
		m_iOffset(_iOffset),
		m_sWrite(std::move(_rWrite)),
		m_iFD(_iFD)
	{
		io_uring_sqe* const sqe = io_uring_queue_init::io_uring_get_sqe(&_pRing->m_sRing);
		sqe->user_data = reinterpret_cast<uintptr_t>(this);
		io_uring_prep_write(
			sqe,
			m_iFD,
			m_sBuffer.data() + m_iOffset,
			m_sBuffer.size() - m_iOffset,
			-1
		);
		io_uring_sqe_set_data(sqe, this);
		io_uring_submit(&_pRing->m_sRing);
	}
	virtual void handle(io_uring_queue_init*const ring, ::io_uring_cqe* const cqe) override
	{	m_sWrite(*this, ring, cqe, true);
	}
	virtual std::vector<char> &getBuffer(void) override
	{	return m_sBuffer;
	}
	virtual std::vector<char> &getBuffer2(void) override
	{	return m_sBuffer2;
	}
	virtual int getFD(void) override
	{	return m_iFD;
	}
	virtual std::size_t getOffset(void) override
	{	return 0;
	}
	virtual std::function<void(io_data&, foelsche::linux::io_uring_queue_init*const , ::io_uring_cqe* const, bool)> getWrite(void) override
	{	return std::move(m_sWrite);
	}
	virtual std::function<void(io_data&, foelsche::linux::io_uring_queue_init*const , ::io_uring_cqe* const)> getRead(void) override
	{	return nullptr;
	}
#if 0
	{	const auto iOffset = m_sBuffer->m_iOffset;
		std::cerr << "write request finished " << cqe->res << std::endl;
		if (iOffset + cqe->res < m_sBuffer->m_s.size())
		{	std::cerr << "scheduling missing write for missing data of " << m_sBuffer->m_s.size() - iOffset + cqe->res << std::endl;
			m_sBuffer->m_iOffset += cqe->res;
			ring->createWrite(
				std::dynamic_pointer_cast<io_data_created_fd>(m_sData),
				m_sBuffer
			);
		}
		else
		{	std::cerr << "scheduling read" << std::endl;
			ring->createRecv(std::dynamic_pointer_cast<io_data_created_fd>(m_sData), std::make_shared<io_data_created_buffer>(std::vector<char>(BUFFER_SIZE), 0));
		}
	}
#endif
};
}
	/// create an read request
std::shared_ptr<io_data> io_uring_queue_init::createRead(
	int _iFD,
	std::vector<char> &&_rBuffer,
	std::vector<char> &&_rBuffer2,
	std::function<void(io_data&, foelsche::linux::io_uring_queue_init*const , ::io_uring_cqe* const)>&&_rRead
)
{	return *m_sIoData.insert(
		std::make_shared<io_data_read>(this, _iFD, std::move(_rBuffer), std::move(_rBuffer2), std::move(_rRead)).get()->shared_from_this()
	).first;
}
std::shared_ptr<io_data> io_uring_queue_init::createWrite(
	int _iFD,
	std::vector<char> &&_rBuffer,
	std::vector<char> &&_rBuffer2,
	std::size_t _iOffset,
	std::function<void(io_data&, foelsche::linux::io_uring_queue_init*const , ::io_uring_cqe* const, bool)>&&_rWrite
)
{	return *m_sIoData.insert(
		std::make_shared<io_data_write>(this, _iFD, std::move(_rBuffer), std::move(_rBuffer2), _iOffset, std::move(_rWrite)).get()->shared_from_this()
	).first;
}
}
}

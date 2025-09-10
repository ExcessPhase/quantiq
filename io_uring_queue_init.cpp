#include "io_uring_queue_init.h"
#include <fcntl.h>
#include <list>
#include <type_traits>


namespace foelsche
{
namespace linux
{
namespace
{
template<typename T>
struct new_delete
{
	using TrivialClone = std::aligned_storage_t<sizeof(T), alignof(T)>;
	static std::list<TrivialClone> s_sFree, s_sUsed;
	static void* alloc(void)
	{	if (s_sFree.empty())
			s_sFree.emplace_front();
		s_sUsed.splice(s_sUsed.begin(), s_sFree, s_sFree.begin());
		return &s_sUsed.front();
	}
	static void free(void* const _p)
	{	for (auto p = s_sUsed.begin(); p != s_sUsed.end(); ++p)
			if (&*p == _p)
			{	s_sFree.splice(s_sFree.begin(), s_sUsed, s_sUsed.begin());
				break;
			}
	}
};
template<typename T>
std::list<typename new_delete<T>::TrivialClone> new_delete<T>::s_sFree;
template<typename T>
std::list<typename new_delete<T>::TrivialClone> new_delete<T>::s_sUsed;
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
	static void* operator new(std::size_t size);
	static void operator delete(void* ptr);
};
void* io_data_read::operator new(std::size_t)
{	return new_delete<io_data_read>::alloc();
}
void io_data_read::operator delete(void* const _p)
{	return new_delete<io_data_read>::free(_p);
}
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
	static void* operator new(std::size_t size);
	static void operator delete(void* ptr);
};
void* io_data_write::operator new(std::size_t)
{	return new_delete<io_data_write>::alloc();
}
void io_data_write::operator delete(void* const _p)
{	return new_delete<io_data_write>::free(_p);
}
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

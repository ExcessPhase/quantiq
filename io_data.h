#pragma once
#include <liburing.h>
#include <memory>
#include <functional>
namespace foelsche
{
namespace linux
{
struct io_uring_queue_init;
	/// an abstract wrapper for any resources created
	/// pointed to by a stdLLshared_ptr
	/// the C++ base type of a request
struct io_data:std::enable_shared_from_this<io_data>
{	//const std::shared_ptr<io_data_created> m_sData;
	static std::size_t s_iNextId;
	const std::size_t m_iId = s_iNextId++;
	io_data(void) = default;
	virtual ~io_data(void) = default;
	virtual void handle(io_uring_queue_init*const _pRing, ::io_uring_cqe* const _pCQE) = 0;
	void handleW(io_uring_queue_init*const _pRing, ::io_uring_cqe* const _pCQE);
	static constexpr const std::size_t BUFFER_SIZE = 16384;
	virtual std::vector<char> &getBuffer(void) = 0;
	virtual std::vector<char> &getBuffer2(void) = 0;
	virtual int getFD(void) = 0;
	virtual std::size_t getOffset(void) = 0;
	virtual std::function<void(io_data&, io_uring_queue_init*const , ::io_uring_cqe* const, bool)> getWrite(void) = 0;
	virtual std::function<void(io_data&, io_uring_queue_init*const , ::io_uring_cqe* const)> getRead(void) = 0;
};
}
}

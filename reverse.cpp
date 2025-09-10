#include "io_uring_queue_init.h"
#include "io_uring_wait_cqe.h"
#include "open.h"
#include <iostream>
#include <memory>
#include <fcntl.h>
#include <cstring>
#include <fstream>
#include <regex>
#include <string>
#include <filesystem>
#include <functional>
namespace foelsche
{
namespace reverse
{
using namespace foelsche::linux;
static constexpr const std::size_t BUFFER_SIZE = 16384;
	/// the event loop
	/// blocking
	/// only serves maximally 8 events
static void event_loop(io_uring_queue_init* const ring)
{	while (!ring->m_sIoData.empty())
	{	io_uring_wait_cqe sCQE(&ring->m_sRing);
		if (sCQE.m_pCQE->res < 0)
			std::cerr << "Async operation failed: " << std::strerror(-sCQE.m_pCQE->res) << std::endl;
		else
		{	auto data(static_cast<io_data*>(io_uring_cqe_get_data(sCQE.m_pCQE))->shared_from_this());
			data->handleW(ring, sCQE.m_pCQE);
		}
	}
}
}
}
int main(int, char**argv)
{	using namespace foelsche::linux;
	using namespace foelsche::reverse;

	constexpr int QUEUE_DEPTH = 256;
	constexpr int PORT = 8080;
		/// RAII initialize and destroy a uring structre
	foelsche::linux::io_uring_queue_init sRing(QUEUE_DEPTH, 0);
		/// RAII initialize and destroy a socket
	foelsche::linux::open sRead(argv[1], O_RDONLY);
	fcntl(sRead.m_i, F_SETFL, fcntl(sRead.m_i, F_GETFL, 0) | O_NONBLOCK);
	foelsche::linux::open sWrite(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 00600);
	fcntl(sWrite.m_i, F_SETFL, fcntl(sWrite.m_i, F_GETFL, 0) | O_NONBLOCK);
	std::vector<char> sLeftOver, sBuffer;
	std::function<void(io_data&, foelsche::linux::io_uring_queue_init*const, ::io_uring_cqe* const)> sReadF;
	const std::function<void(io_data&, foelsche::linux::io_uring_queue_init*const, ::io_uring_cqe* const, bool)> sWriteF = [&](io_data&_r, foelsche::linux::io_uring_queue_init*const _pRing, ::io_uring_cqe* const _pCQE, bool)
	{	
		if (_r.getOffset() + _pCQE->res < _r.getBuffer().size())
			_pRing->createWrite(
				sWrite.m_i,
				std::move(_r.getBuffer()),
				std::move(_r.getBuffer2()),
				_r.getOffset() + _pCQE->res,
				//_r.getWrite()
				std::function<void(io_data&, foelsche::linux::io_uring_queue_init*const, ::io_uring_cqe* const, bool)>(sWriteF)
			);
		else
			_pRing->createRead(
				sRead.m_i,
				std::move(_r.getBuffer2()),
				std::move(_r.getBuffer()),
				std::function<void(io_data&, foelsche::linux::io_uring_queue_init*const, ::io_uring_cqe* const)>(sReadF)
			);
	};
	sReadF = [&](io_data&_r, foelsche::linux::io_uring_queue_init*const _pRing, ::io_uring_cqe* const _pCQE)
	{	if (!_pCQE->res)
			return;
		_r.getBuffer().resize(_pCQE->res);
		auto p = _r.getBuffer().begin(), pEnd = _r.getBuffer().end(), pLast = p;
		for (; p != pEnd; ++p)
			if (*p == '\n')
			{	std::reverse(pLast, p);
				pLast = p + 1;
			}
		_r.getBuffer2().assign(pLast, pEnd);
		_r.getBuffer().resize(pLast - _r.getBuffer().begin());
		_pRing->createWrite(
			sWrite.m_i,
			std::move(_r.getBuffer()),
			std::move(_r.getBuffer2()),
			0,
			std::function<void(io_data&, foelsche::linux::io_uring_queue_init*const, ::io_uring_cqe* const, bool)>(sWriteF)
		);
	};
	
	sRing.createRead(
		sRead.m_i,
		std::move(sLeftOver),
		std::move(sBuffer),
		std::function<void(io_data&, foelsche::linux::io_uring_queue_init*const, ::io_uring_cqe* const)>(sReadF)
	);
		/// call the event loop
	event_loop(&sRing);
}

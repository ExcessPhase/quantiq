#include "io_data.h"
#include "io_uring_queue_init.h"


namespace foelsche
{
namespace linux_ns
{
void io_data::handleW(io_uring_queue_init*const _pRing, ::io_uring_cqe* const _pCQE)
{		/// calling the handler
	handle(_pRing, _pCQE);
		/// and removing the object from the set so that it can go away if not referenced by somebody else
	_pRing->m_sIoData.erase(shared_from_this());
}
std::size_t io_data::s_iNextId;
}
}

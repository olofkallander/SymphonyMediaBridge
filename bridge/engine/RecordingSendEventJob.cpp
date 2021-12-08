#include "bridge/engine/RecordingSendEventJob.h"
#include "transport/RecordingTransport.h"
#include "transport/recp/RecHeader.h"

namespace bridge
{

RecordingSendEventJob::RecordingSendEventJob(std::atomic_uint32_t& ownerJobsCounter,
    memory::Packet* packet,
    memory::PacketPoolAllocator& allocator,
    transport::RecordingTransport& transport,
    PacketCache& recEventPacketCache,
    UnackedPacketsTracker& unackedPacketsTracker)
    : CountedJob(ownerJobsCounter),
      _packet(packet),
      _allocator(allocator),
      _transport(transport),
      _recEventPacketCache(recEventPacketCache),
      _unackedPacketsTracker(unackedPacketsTracker)
{
}

RecordingSendEventJob::~RecordingSendEventJob()
{
    if (_packet)
    {
        _allocator.free(_packet);
        _packet = nullptr;
    }
}

void RecordingSendEventJob::run()
{
    auto recHeader = recp::RecHeader::fromPacket(*_packet);
    if (!recHeader)
    {
        return;
    }

    const auto sequenceNumber = recHeader->sequenceNumber.get();
    _recEventPacketCache.add(_packet, sequenceNumber);
    _transport.protectAndSend(_packet, _allocator);
    _unackedPacketsTracker.onPacketSent(sequenceNumber, utils::Time::getAbsoluteTime() / 1000000ULL);
    _packet = nullptr;
}

} // namespace bridge
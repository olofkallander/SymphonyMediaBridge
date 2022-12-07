#include "UdpEndpointImpl.h"
#include "dtls/SslDtls.h"
#include "memory/Packet.h"
#include "memory/PacketPoolAllocator.h"
#include "rtp/RtcpHeader.h"
#include "rtp/RtpHeader.h"
#include "utils/Function.h"
#include <cstdint>

#include "crypto/SslHelper.h"

#define DEBUG_ENDPOINT 0

#if DEBUG_ENDPOINT
#define LOG(fmt, ...) logger::debug(fmt, ##__VA_ARGS__)
#else
#define LOG(fmt, ...)
#endif
namespace transport
{

// When this endpoint is shared the number of registration jobs and packets in queue will be plenty
// and the data structures are therefore larger
UdpEndpointImpl::UdpEndpointImpl(jobmanager::JobManager& jobManager,
    size_t maxSessionCount,
    memory::PacketPoolAllocator& allocator,
    const SocketAddress& localPort,
    RtcePoll& epoll,
    bool isShared)
    : BaseUdpEndpoint("UdpEndpoint", jobManager, maxSessionCount, allocator, localPort, epoll, isShared),
      _iceListeners(maxSessionCount * 2),
      _dtlsListeners(maxSessionCount * 2),
      _iceResponseListeners(maxSessionCount * 4)
{
}

UdpEndpointImpl::~UdpEndpointImpl()
{
    logger::debug("removed", _name.c_str());
}

void UdpEndpointImpl::sendStunTo(const transport::SocketAddress& target,
    __uint128_t transactionId,
    const void* data,
    size_t len,
    uint64_t timestamp)
{
    auto* msg = ice::StunMessage::fromPtr(data);
    if (msg->header.isRequest() && !_iceResponseListeners.contains(transactionId))
    {
        auto names = msg->getAttribute<ice::StunUserName>(ice::StunAttribute::USERNAME);
        if (names)
        {
            auto localUser = names->getNames().second;
            auto it = _iceListeners.find(localUser);
            if (it != _iceListeners.cend())
            {
                assert(it->second);

                logger::info("!!! sendStunTo, going ot emplace _iceResponseListeners, num of listeners %sz",
                    _name.c_str(),
                    _iceResponseListeners.size());

                auto pair = _iceResponseListeners.emplace(transactionId, it->second);
                if (!pair.second)
                {
                    logger::warn("Pending ICE request lookup table is full", _name.c_str());
                }
                else
                {
                    const IndexableInteger<__uint128_t, uint32_t> id(transactionId);
                    LOG("register ICE listener for %04x%04x%04x", _name.c_str(), id[1], id[2], id[3]);
                }
            }
        }
    }

    sendTo(target, memory::makeUniquePacket(_allocator, data, len));
}

void UdpEndpointImpl::unregisterListener(IEvents* listener)
{
    if (!_receiveJobs.post(utils::bind(&UdpEndpointImpl::internalUnregisterListener, this, listener)))
    {
        logger::error("failed to post unregister job", _name.c_str());
    }
}

void UdpEndpointImpl::cancelStunTransaction(__uint128_t transactionId)
{
    logger::info("!!! cancelStunTransaction, going ot erase _iceResponseListeners, num of listeners %sz",
        _name.c_str(),
        _iceResponseListeners.size());

    const bool posted = _receiveJobs.post([this, transactionId]() { _iceResponseListeners.erase(transactionId); });
    if (!posted)
    {
        logger::warn("failed to post unregister STUN transaction job", _name.c_str());
    }
}

void UdpEndpointImpl::internalUnregisterListener(IEvents* listener)
{
    // Hashmap allows erasing elements while iterating.
    LOG("unregister %p", _name.c_str(), listener);
    for (auto& item : _iceListeners)
    {
        if (item.second == listener)
        {
            _iceListeners.erase(item.first);
            listener->onUnregistered(*this);
            break;
        }
    }

    for (auto& responseListener : _iceResponseListeners)
    {
        if (responseListener.second == listener)
        {
            logger::debug("!!! internalUnregisterListener, going ot erase _iceResponseListeners, num of listeners %sz",
                _name.c_str(),
                _iceResponseListeners.size());

            _iceResponseListeners.erase(responseListener.first);
            // must be iceListener to be iceResponseListener so no extra unreg notification
        }
    }

    for (auto& item : _dtlsListeners)
    {
        if (item.second == listener)
        {
            _dtlsListeners.erase(item.first);
            listener->onUnregistered(*this);
        }
    }
}

void UdpEndpointImpl::dispatchReceivedPacket(const SocketAddress& srcAddress,
    memory::UniquePacket packet,
    const uint64_t timestamp)
{
    UdpEndpointImpl::IEvents* listener = _defaultListener;

    if (ice::isStunMessage(packet->get(), packet->getLength()))
    {
        auto msg = ice::StunMessage::fromPtr(packet->get());

        if (msg->header.isRequest())
        {
            auto users = msg->getAttribute<ice::StunUserName>(ice::StunAttribute::USERNAME);
            if (users)
            {
                auto userName = users->getNames().first;
                listener = _iceListeners.getItem(userName);
            }
            LOG("ICE request for %s src %s",
                _name.c_str(),
                users->getNames().first.c_str(),
                srcAddress.toString().c_str());
        }
        else if (msg->header.isResponse())
        {
            auto transactionId = msg->header.transactionId.get();
            listener = _iceResponseListeners.getItem(transactionId);
            if (listener)
            {
                const IndexableInteger<__uint128_t, uint32_t> id(transactionId);
                LOG("STUN response received for transaction %04x%04x%04x", _name.c_str(), id[1], id[2], id[3]);

                logger::info("!!! dispatchReceivedPacket, going ot erase _iceResponseListeners, num of listeners %sz",
                    _name.c_str(),
                    _iceResponseListeners.size());
                _iceResponseListeners.erase(transactionId);
            }
        }

        if (listener)
        {
            listener->onIceReceived(*this, srcAddress, _socket.getBoundPort(), std::move(packet), timestamp);
            return;
        }
        else
        {
            LOG("cannot find listener for STUN", _name.c_str());
        }
    }
    else if (transport::isDtlsPacket(packet->get()))
    {
        listener = _dtlsListeners.getItem(srcAddress);
        listener = listener ? listener : _defaultListener.load();
        if (listener)
        {
            listener->onDtlsReceived(*this, srcAddress, _socket.getBoundPort(), std::move(packet), timestamp);
            return;
        }
        else
        {
            LOG("cannot find listener for DTLS source %s", _name.c_str(), srcAddress.toString().c_str());
        }
    }
    else if (rtp::isRtcpPacket(packet->get(), packet->getLength()))
    {
        auto rtcpReport = rtp::RtcpReport::fromPtr(packet->get(), packet->getLength());
        if (rtcpReport)
        {
            listener = _dtlsListeners.getItem(srcAddress);

            if (listener)
            {
                listener->onRtcpReceived(*this, srcAddress, _socket.getBoundPort(), std::move(packet), timestamp);
                return;
            }
            else
            {
                LOG("cannot find listener for RTCP", _name.c_str());
            }
        }
    }
    else if (rtp::isRtpPacket(packet->get(), packet->getLength()))
    {
        auto rtpPacket = rtp::RtpHeader::fromPacket(*packet);
        if (rtpPacket)
        {
            listener = _dtlsListeners.getItem(srcAddress);

            if (listener)
            {
                listener->onRtpReceived(*this, srcAddress, _socket.getBoundPort(), std::move(packet), timestamp);
                return;
            }
            else
            {
                LOG("cannot find listener for RTP", _name.c_str());
            }
        }
    }
    else
    {
        LOG("Unexpected packet from %s", _name.c_str(), srcAddress.toString().c_str());
    }
    // unexpected packet that can come from anywhere. We do not log as it facilitates DoS
}

void UdpEndpointImpl::registerListener(const std::string& stunUserName, IEvents* listener)
{
    if (_iceListeners.contains(stunUserName))
    {
        return;
    }

    _iceListeners.emplace(stunUserName, listener);
    listener->onRegistered(*this);
}

/** If using ICE, must be called from receive job queue to sync unregister */
void UdpEndpointImpl::registerListener(const SocketAddress& srcAddress, IEvents* listener)
{
    auto it = _dtlsListeners.emplace(srcAddress, listener);
    if (it.second)
    {
        logger::debug("register listener for %s", _name.c_str(), srcAddress.toString().c_str());
        listener->onRegistered(*this);
    }
    else if (it.first != _dtlsListeners.cend() && it.first->second == listener)
    {
        // already registered
    }
    else
    {
        _receiveJobs.post(utils::bind(&UdpEndpointImpl::swapListener, this, srcAddress, listener));
    }
}

void UdpEndpointImpl::swapListener(const SocketAddress& srcAddress, IEvents* newListener)
{
    auto it = _dtlsListeners.find(srcAddress);
    if (it != _dtlsListeners.cend())
    {
        // src port is re-used. Unregister will look at listener pointer
        if (it->second == newListener)
        {
            return;
        }

        if (it->second)
        {
            it->second->onUnregistered(*this);
        }
        it->second = newListener;
        newListener->onRegistered(*this);
        return;
    }

    logger::warn("dtls listener swap on %s skipped. Already removed", _name.c_str(), srcAddress.toString().c_str());
}

void UdpEndpointImpl::focusListener(const SocketAddress& remotePort, IEvents* listener)
{
    _receiveJobs.post([this, remotePort, listener]() {
        for (auto& item : _dtlsListeners)
        {
            if (item.second == listener && item.first != remotePort)
            {
                LOG("focus listener on %s, unlisten %s",
                    _name.c_str(),
                    remotePort.toString().c_str(),
                    item.first.toString().c_str());
                _dtlsListeners.erase(item.first);
                listener->onUnregistered(*this);
            }
        }
    });
}

} // namespace transport

#pragma once

#include "bridge/engine/EngineCommand.h"
#include "bridge/engine/EngineStats.h"
#include "concurrency/MpmcPublish.h"
#include "concurrency/MpmcQueue.h"
#include "concurrency/ThreadUtils.h"
#include "memory/List.h"
#include "memory/PoolAllocator.h"
#include "utils/Time.h"
#include "utils/Trackers.h"
#include <memory>
#include <sys/types.h>
#include <thread>

namespace bridge
{

class Mixer;
class EngineMixer;
class EngineMessageListener;

class Engine
{
public:
    Engine();

    void setMessageListener(EngineMessageListener* messageListener);
    void stop();
    void run();

    void pushCommand(const EngineCommand::Command& command);

    EngineStats::EngineStats getStats();

private:
    static const size_t maxMixers = 4096;
    static const uint32_t STATS_UPDATE_TICKS = 200;

    EngineMessageListener* _messageListener;
    std::atomic<bool> _running;

    concurrency::MpmcQueue<EngineCommand::Command> _pendingCommands;
    memory::List<EngineMixer*, maxMixers> _mixers;

    concurrency::MpmcPublish<EngineStats::EngineStats, 4> _stats;
    uint32_t _tickCounter;

    utils::AvgTracker _engineIdle;

    std::thread _thread; // must be last member

    void addMixer(EngineCommand::Command& nextCommand);
    void removeMixer(EngineCommand::Command& nextCommand);
    void addAudioStream(EngineCommand::Command& nextCommand);
    void removeAudioStream(EngineCommand::Command& nextCommand);
    void addAudioBuffer(EngineCommand::Command& nextCommand);
    void addVideoStream(EngineCommand::Command& nextCommand);
    void removeVideoStream(EngineCommand::Command& nextCommand);
    void addRecordingStream(EngineCommand::Command& command);
    void removeRecordingStream(EngineCommand::Command& command);
    void updateRecordingStreamModalities(EngineCommand::Command& command);
    void addDataStream(EngineCommand::Command& nextCommand);
    void removeDataStream(EngineCommand::Command& nextCommand);
    void startTransport(EngineCommand::Command& nextCommand);
    void startRecordingTransport(EngineCommand::Command& nextCommand);
    void reconfigureAudioStream(EngineCommand::Command& nextCommand);
    void reconfigureVideoStream(EngineCommand::Command& nextCommand);
    void addVideoPacketCache(EngineCommand::Command& nextCommand);
    void processSctpControl(EngineCommand::Command& command);
    void pinEndpoint(EngineCommand::Command& command);
    void sendEndpointMessage(EngineCommand::Command& command);
    void startRecording(EngineCommand::Command& nextCommand);
    void stopRecording(EngineCommand::Command& nextCommand);
    void addRecordingRtpPacketCache(EngineCommand::Command& nextCommand);
    void addTransportToRecordingStream(EngineCommand::Command& nextCommand);
    void removeTransportFromRecordingStream(EngineCommand::Command& nextCommand);
};

} // namespace bridge
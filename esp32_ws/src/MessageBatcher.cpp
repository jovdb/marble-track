#include "MessageBatcher.h"
#include "Logging.h"

void MessageBatcher::send(const String &message)
{
    if (_batchingActive)
    {
        if (_queue.size() >= _maxQueueSize)
        {
            MLOG_WARN("Message batch queue full (%u). Dropping message.", static_cast<unsigned>(_maxQueueSize));
            return;
        }
        _queue.push_back(message);
        return;
    }

    if (_canWriteFn && !_canWriteFn())
    {
        MLOG_WARN("Transport send buffer full. Dropping message.");
        return;
    }

    String arrayMessage = "[" + message + "]";
    MLOG_WS_SEND("%s", arrayMessage.c_str());
    if (_sendFn) _sendFn(arrayMessage);
}

void MessageBatcher::beginBatch()
{
    _batchingActive = true;
    _queue.clear();
}

void MessageBatcher::endBatch()
{
    _batchingActive = false;

    if (_queue.empty())
    {
        return;
    }

    if (_canWriteFn && !_canWriteFn())
    {
        MLOG_WARN("Transport send buffer full. Dropping %u batched messages.", static_cast<unsigned>(_queue.size()));
        _queue.clear();
        return;
    }

    String batchMessage = "[";
    bool firstMessage = true;
    for (const auto &msg : _queue)
    {
        if (msg.isEmpty())
        {
            continue;
        }
        if (!firstMessage)
        {
            batchMessage += ",";
        }
        firstMessage = false;
        MLOG_WS_SEND("%s", msg.c_str());
        batchMessage += msg;
    }
    batchMessage += "]";

    if (_sendFn) _sendFn(batchMessage);
    _queue.clear();
}

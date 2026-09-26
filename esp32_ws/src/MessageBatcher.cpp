#include "MessageBatcher.h"
#include "Logging.h"

void MessageBatcher::send(const String &message)
{
    if (!_mutex || xSemaphoreTake(_mutex, portMAX_DELAY) != pdTRUE)
        return;

    if (_batchingActive)
    {
        if (_queue.size() >= _maxQueueSize)
        {
            MLOG_WARN("Message batch queue full (%u). Dropping message.", static_cast<unsigned>(_maxQueueSize));
            xSemaphoreGive(_mutex);
            return;
        }
        _queue.push_back(message);
        xSemaphoreGive(_mutex);
        return;
    }

    if (_canWriteFn && !_canWriteFn())
    {
        MLOG_WARN("Transport send buffer full. Dropping message.");
        xSemaphoreGive(_mutex);
        return;
    }

    String arrayMessage = "[" + message + "]";
    MLOG_WS_SEND("%s", arrayMessage.c_str());
    if (_sendFn) _sendFn(arrayMessage);
    xSemaphoreGive(_mutex);
}

void MessageBatcher::beginBatch()
{
    if (!_mutex || xSemaphoreTake(_mutex, portMAX_DELAY) != pdTRUE)
        return;

    _batchingActive = true;
    _queue.clear();
    xSemaphoreGive(_mutex);
}

void MessageBatcher::endBatch()
{
    if (!_mutex || xSemaphoreTake(_mutex, portMAX_DELAY) != pdTRUE)
        return;

    _batchingActive = false;

    if (_queue.empty())
    {
        xSemaphoreGive(_mutex);
        return;
    }

    if (_canWriteFn && !_canWriteFn())
    {
        MLOG_WARN("Transport send buffer full. Dropping %u batched messages.", static_cast<unsigned>(_queue.size()));
        _queue.clear();
        xSemaphoreGive(_mutex);
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
    xSemaphoreGive(_mutex);
}

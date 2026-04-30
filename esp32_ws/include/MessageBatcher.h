#pragma once

#include <Arduino.h>
#include <functional>
#include <vector>

/**
 * @brief Batches outbound WebSocket messages into JSON arrays.
 *
 * Wire protocol: every flush produces a JSON array. Outside a batch, send()
 * emits a single-element array "[<msg>]". Between beginBatch()/endBatch(),
 * messages are queued and emitted together as one multi-element array.
 *
 * The batcher does not know about AsyncWebSocket; the actual transport is
 * injected as two callbacks at construction:
 *   - sendFn:           writes the framed string to all connected clients
 *   - canWriteFn:       returns true if the transport is ready (back-pressure)
 *
 * Messages are dropped (with a warning) when the queue exceeds maxQueueSize
 * or when canWriteFn() returns false. Callers must not rely on guaranteed
 * delivery of non-critical updates.
 */
class MessageBatcher
{
public:
    using SendFn = std::function<void(const String &payload)>;
    using CanWriteFn = std::function<bool()>;

    MessageBatcher(SendFn sendFn, CanWriteFn canWriteFn, size_t maxQueueSize = 64)
        : _sendFn(std::move(sendFn)),
          _canWriteFn(std::move(canWriteFn)),
          _maxQueueSize(maxQueueSize),
          _batchingActive(false)
    {
    }

    /**
     * @brief Send a single message, or queue it if a batch is active.
     */
    void send(const String &message);

    /**
     * @brief Begin a batch. All subsequent send() calls are queued until endBatch().
     */
    void beginBatch();

    /**
     * @brief End the active batch and flush all queued messages as one JSON array.
     */
    void endBatch();

    bool isBatching() const { return _batchingActive; }

private:
    SendFn _sendFn;
    CanWriteFn _canWriteFn;
    size_t _maxQueueSize;
    bool _batchingActive;
    std::vector<String> _queue;
};

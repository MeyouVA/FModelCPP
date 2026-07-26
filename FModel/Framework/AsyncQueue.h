#pragma once
// Ported from FModel/Framework/AsyncQueue.cs — the FIFO ThreadWorkerViewModel drains one job at a time.
//
// C# builds it out of TPL Dataflow (`BufferBlock<T>`) plus a `SemaphoreSlim(1)`, and exposes it as an
// `IAsyncEnumerable<T>`: `await foreach (var job in _jobs)` pulls items until the buffer runs dry, and the
// semaphore guarantees only one consumer drains it at a time. The port has no threading layer yet (see
// ThreadWorkerViewModel.h), so:
//
//   * the buffer is a std::deque and Enqueue/Count keep their meaning exactly;
//   * the async enumerator becomes tryDequeue(), which is what a single-threaded drain loop needs — the
//     `while (Count > 0) yield return await ReceiveAsync()` shape reduces to it one-for-one;
//   * the semaphore has no counterpart, because there is only ever one consumer. It is what stops a second
//     ProcessQueues from interleaving in C#; the equivalent guard here is `_draining` on the consumer, which
//     is where the reentrancy actually arises (a job that enqueues another job).
//
// Deliberate difference worth knowing: because the queue is drained synchronously, a job enqueued *by* a
// running job is picked up by the same drain loop rather than a later one. C# behaves the same way — the
// `while (Count > 0)` is re-checked after every item — so the ordering is preserved, not just the contents.

#include <deque>
#include <optional>
#include <utility>

namespace FModel::Framework
{
    template <typename T>
    class AsyncQueue
    {
    public:
        int count() const { return static_cast<int>(_buffer.size()); }

        void enqueue(T item) { _buffer.push_back(std::move(item)); }

        // The `while (Count > 0) yield return await _buffer.ReceiveAsync(token)` body, one item at a time.
        std::optional<T> tryDequeue()
        {
            if (_buffer.empty())
                return std::nullopt;

            T item = std::move(_buffer.front());
            _buffer.pop_front();
            return item;
        }

        void clear() { _buffer.clear(); }

    private:
        std::deque<T> _buffer;
    };
}

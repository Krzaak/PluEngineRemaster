//
// Created by Plutex on 6/21/26.
//

#ifndef PLUENGINE_TRIPLEBUFFER_H
#define PLUENGINE_TRIPLEBUFFER_H

#include <atomic>
#include <array>
#include <cstdint>
#include <new> // std::hardware_destructive_interference_size

// Lock-free Triple Buffer.
// Writer (Main thread) never blocks - always has a free slot to write into.
// Reader (Render thread) never blocks - always has the latest published data,
// or reuses the previous one if nothing new was published yet.
//
// Classic algorithm (sometimes called "dvyukov triple buffer" / used widely
// in game engines for Main->Render snapshot handoff).
namespace Plu
{
    template<typename T>
    class TripleBuffer
    {
    public:
        TripleBuffer() = default;

        // --- Writer side (Main thread) -----------------------------------

        // Get mutable reference to the buffer you should write into this frame.
        T& GetWriteBuffer()
        {
            return buffers_[writeIdx_];
        }

        // Non-blocking check: has the previously Publish()-ed snapshot NOT been
        // picked up by the reader yet? Main can use this to skip rebuilding the
        // snapshot this frame (saving the copy/build cost) when Render is the
        // bottleneck and hasn't consumed the last one.
        //
        // Note: if nothing was ever published, this returns false (nothing pending).
        bool HasPendingSnapshot() const
        {
            return (readyIdx_.load(std::memory_order_acquire) & kNewDataFlag) != 0;
        }

        // Call once you're done filling the write buffer.
        // Publishes it atomically as the new "ready" snapshot.
        void Publish()
        {
            // Mark our just-filled buffer as the new ready one (with NEW_DATA flag set)
            const uint8_t newReady = static_cast<uint8_t>(writeIdx_ | kNewDataFlag);
            const uint8_t prevReady = readyIdx_.exchange(newReady, std::memory_order_acq_rel);

            if (prevReady & kNewDataFlag)
            {
                // The previous publish was never picked up by the reader - it just
                // got overwritten/discarded. Render is falling behind.
                droppedSnapshotCount_.fetch_add(1, std::memory_order_relaxed);
            }

            // Reclaim whatever buffer was previously "ready" (reader hasn't picked it up,
            // or it's the old read buffer the reader handed back) - it's free to overwrite.
            writeIdx_ = prevReady & kIndexMask;
        }

        // --- Reader side (Render thread) -----------------------------------

        // Call once per render frame. Returns the latest available snapshot.
        // Never blocks - if nothing new was published, returns the same buffer as last time.
        const T& AcquireReadBuffer()
        {
            const uint8_t cur = readyIdx_.load(std::memory_order_acquire);
            if (cur & kNewDataFlag)
            {
                // There IS fresh data - swap it in, hand our old read buffer back as free.
                const uint8_t newReady = readIdx_; // no flag - this becomes a "free" slot
                const uint8_t prev = readyIdx_.exchange(newReady, std::memory_order_acq_rel);
                readIdx_ = prev & kIndexMask;
            }
            else
            {
                // Nothing new since last frame - we're about to render a duplicate.
                staleFrameCount_.fetch_add(1, std::memory_order_relaxed);
            }
            return buffers_[readIdx_];
        }

        // --- Telemetry getters (safe to call from any thread) -----------------

        // How many published snapshots were overwritten before Render ever saw them.
        uint32_t GetDroppedSnapshotCount() const
        {
            return droppedSnapshotCount_.load(std::memory_order_relaxed);
        }

        // How many times Render re-rendered the same snapshot (no new data was ready).
        uint32_t GetStaleFrameCount() const
        {
            return staleFrameCount_.load(std::memory_order_relaxed);
        }

        // Reset both counters - handy to call once a second for a rolling stat.
        void ResetTelemetry()
        {
            droppedSnapshotCount_.store(0, std::memory_order_relaxed);
            staleFrameCount_.store(0, std::memory_order_relaxed);
        }

    private:
        static constexpr uint8_t kIndexMask  = 0b011;
        static constexpr uint8_t kNewDataFlag = 0b100;

        std::array<T, 3> buffers_{};

        // --- Telemetry (optional, zero-cost-ish - just two atomic counters) ---
        // How many times Publish() overwrote a snapshot the reader never picked up
        // (i.e. Render fell behind Main by at least one frame).
        std::atomic<uint32_t> droppedSnapshotCount_{0};
        // How many times AcquireReadBuffer() found no new data and re-returned
        // the same buffer (i.e. Render outpaced Main / rendered a duplicate frame).
        std::atomic<uint32_t> staleFrameCount_{0};

        // Only touched by the writer thread - no atomic needed.
        uint8_t writeIdx_ = 0;
        // Only touched by the reader thread - no atomic needed.
        uint8_t readIdx_  = 1;

        // Shared hand-off point between threads. Pad to avoid false sharing
        // with whatever sits next to this object in memory.
        alignas(std::hardware_destructive_interference_size)
            std::atomic<uint8_t> readyIdx_{2}; // starts with no NEW_DATA_FLAG - nothing published yet
    };
}

#endif //PLUENGINE_TRIPLEBUFFER_H

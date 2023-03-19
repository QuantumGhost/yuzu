// SPDX-FileCopyrightText: Copyright 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <new>
#include <version>

#include "common/polyfill_thread.h"

namespace Common {

namespace detail {
constexpr size_t DefaultCapacity = 0x1000;
} // namespace detail

template <typename T, size_t Capacity = detail::DefaultCapacity>
class SPSCQueue {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of two.");

public:
    bool TryPush(T&& t) {
        return Push<PushMode::Try>(std::move(t));
    }

    template <typename... Args>
    bool TryEmplace(Args&&... args) {
        return Emplace<PushMode::Try>(std::forward<Args>(args)...);
    }

    void PushWait(T&& t) {
        Push<PushMode::Wait>(std::move(t));
    }

    template <typename... Args>
    void EmplaceWait(Args&&... args) {
        Emplace<PushMode::Wait>(std::forward<Args>(args)...);
    }

    void PushOverwrite(T&& t) {
        Push<PushMode::Overwrite>(std::move(t));
    }

    template <typename... Args>
    void EmplaceOverwrite(Args&&... args) {
        Emplace<PushMode::Overwrite>(std::forward<Args>(args)...);
    }

    bool TryPop(T& t) {
        return Pop<PopMode::Try>(t);
    }

    void PopWait(T& t) {
        Pop<PopMode::Wait>(t);
    }

    void PopWait(T& t, std::stop_token stop_token) {
        Pop<PopMode::WaitWithStopToken>(t, stop_token);
    }

    T PopWait() {
        T t;
        Pop<PopMode::Wait>(t);
        return t;
    }

    T PopWait(std::stop_token stop_token) {
        T t;
        Pop<PopMode::WaitWithStopToken>(t, stop_token);
        return t;
    }

    void Clear() {
        while (!Empty()) {
            Pop();
        }
    }

    bool Empty() const {
        return m_read_index.load() == m_write_index.load();
    }

    size_t Size() const {
        return m_write_index.load() - m_read_index.load();
    }

private:
    enum class PushMode {
        Try,
        Wait,
        Overwrite,
        Count,
    };

    enum class PopMode {
        Try,
        Wait,
        WaitWithStopToken,
        Count,
    };

    template <PushMode Mode>
    bool Push(T&& t) {
        const size_t write_index = m_write_index.load();

        if constexpr (Mode == PushMode::Try) {
            // Check if we have free slots to write to.
            if ((write_index - m_read_index.load()) == Capacity) {
                return false;
            }
        } else if constexpr (Mode == PushMode::Wait) {
            // Wait until we have free slots to write to.
            std::unique_lock lock{producer_cv_mutex};
            producer_cv.wait(lock, [this, write_index] {
                return (write_index - m_read_index.load()) < Capacity;
            });
        } else if constexpr (Mode == PushMode::Overwrite) {
            // Check if we have free slots to write to.
            if ((write_index - m_read_index.load()) == Capacity) {
                // If we don't, increment the read index. This is effectively a pop operation.
                ++m_read_index;
            }
        } else {
            static_assert(Mode < PushMode::Count, "Invalid PushMode.");
        }

        // Determine the position to write to.
        const size_t pos = write_index % Capacity;

        // Push into the queue.
        m_data[pos] = std::move(t);

        // Increment the write index.
        ++m_write_index;

        // Notify the consumer that we have pushed into the queue.
        std::scoped_lock lock{consumer_cv_mutex};
        consumer_cv.notify_one();

        return true;
    }

    template <PushMode Mode, typename... Args>
    bool Emplace(Args&&... args) {
        const size_t write_index = m_write_index.load();

        if constexpr (Mode == PushMode::Try) {
            // Check if we have free slots to write to.
            if ((write_index - m_read_index.load()) == Capacity) {
                return false;
            }
        } else if constexpr (Mode == PushMode::Wait) {
            // Wait until we have free slots to write to.
            std::unique_lock lock{producer_cv_mutex};
            producer_cv.wait(lock, [this, write_index] {
                return (write_index - m_read_index.load()) < Capacity;
            });
        } else if constexpr (Mode == PushMode::Overwrite) {
            // Check if we have free slots to write to.
            if ((write_index - m_read_index.load()) == Capacity) {
                // If we don't, increment the read index. This is effectively a pop operation.
                ++m_read_index;
            }
        } else {
            static_assert(Mode < PushMode::Count, "Invalid PushMode.");
        }

        // Determine the position to write to.
        const size_t pos = write_index % Capacity;

        // Emplace into the queue.
        std::construct_at(std::addressof(m_data[pos]), std::forward<Args>(args)...);

        // Increment the write index.
        ++m_write_index;

        // Notify the consumer that we have pushed into the queue.
        std::scoped_lock lock{consumer_cv_mutex};
        consumer_cv.notify_one();

        return true;
    }

    void Pop() {
        const size_t read_index = m_read_index.load();

        // Check if the queue is empty.
        if (read_index == m_write_index.load()) {
            return;
        }

        // Determine the position to read from.
        const size_t pos = read_index % Capacity;

        // Pop the data off the queue, deleting it.
        std::destroy_at(std::addressof(m_data[pos]));

        // Increment the read index.
        ++m_read_index;

        // Notify the producer that we have popped off the queue.
        std::unique_lock lock{producer_cv_mutex};
        producer_cv.notify_one();
    }

    template <PopMode Mode>
    bool Pop(T& t, [[maybe_unused]] std::stop_token stop_token = {}) {
        const size_t read_index = m_read_index.load();

        if constexpr (Mode == PopMode::Try) {
            // Check if the queue is empty.
            if (read_index == m_write_index.load()) {
                return false;
            }
        } else if constexpr (Mode == PopMode::Wait) {
            // Wait until the queue is not empty.
            std::unique_lock lock{consumer_cv_mutex};
            consumer_cv.wait(lock,
                             [this, read_index] { return read_index != m_write_index.load(); });
        } else if constexpr (Mode == PopMode::WaitWithStopToken) {
            // Wait until the queue is not empty.
            std::unique_lock lock{consumer_cv_mutex};
            Common::CondvarWait(consumer_cv, lock, stop_token,
                                [this, read_index] { return read_index != m_write_index.load(); });
        } else {
            static_assert(Mode < PopMode::Count, "Invalid PopMode.");
        }

        // Determine the position to read from.
        const size_t pos = read_index % Capacity;

        // Pop the data off the queue, moving it.
        t = std::move(m_data[pos]);

        // Increment the read index.
        ++m_read_index;

        // Notify the producer that we have popped off the queue.
        std::unique_lock lock{producer_cv_mutex};
        producer_cv.notify_one();

        return true;
    }

#ifdef __cpp_lib_hardware_interference_size
    alignas(std::hardware_destructive_interference_size) std::atomic_size_t m_read_index{0};
    alignas(std::hardware_destructive_interference_size) std::atomic_size_t m_write_index{0};
#else
    alignas(64) std::atomic_size_t m_read_index{0};
    alignas(64) std::atomic_size_t m_write_index{0};
#endif

    std::array<T, Capacity> m_data;

    std::condition_variable_any producer_cv;
    std::mutex producer_cv_mutex;
    std::condition_variable_any consumer_cv;
    std::mutex consumer_cv_mutex;
};

template <typename T, size_t Capacity = detail::DefaultCapacity>
class MPSCQueue {
public:
    bool TryPush(T&& t) {
        std::scoped_lock lock{write_mutex};
        return spsc_queue.TryPush(std::move(t));
    }

    template <typename... Args>
    bool TryEmplace(Args&&... args) {
        std::scoped_lock lock{write_mutex};
        return spsc_queue.TryEmplace(std::forward<Args>(args)...);
    }

    void PushWait(T&& t) {
        std::scoped_lock lock{write_mutex};
        spsc_queue.PushWait(std::move(t));
    }

    template <typename... Args>
    void EmplaceWait(Args&&... args) {
        std::scoped_lock lock{write_mutex};
        spsc_queue.EmplaceWait(std::forward<Args>(args)...);
    }

    void PushOverwrite(T&& t) {
        std::scoped_lock lock{write_mutex};
        spsc_queue.PushOverwrite(std::move(t));
    }

    template <typename... Args>
    void EmplaceOverwrite(Args&&... args) {
        std::scoped_lock lock{write_mutex};
        spsc_queue.EmplaceOverwrite(std::forward<Args>(args)...);
    }

    bool TryPop(T& t) {
        return spsc_queue.TryPop(t);
    }

    void PopWait(T& t) {
        spsc_queue.PopWait(t);
    }

    void PopWait(T& t, std::stop_token stop_token) {
        spsc_queue.PopWait(t, stop_token);
    }

    T PopWait() {
        return spsc_queue.PopWait();
    }

    T PopWait(std::stop_token stop_token) {
        return spsc_queue.PopWait(stop_token);
    }

    void Clear() {
        spsc_queue.Clear();
    }

    bool Empty() {
        return spsc_queue.Empty();
    }

    size_t Size() {
        return spsc_queue.Size();
    }

private:
    SPSCQueue<T, Capacity> spsc_queue;
    std::mutex write_mutex;
};

template <typename T, size_t Capacity = detail::DefaultCapacity>
class MPMCQueue {
public:
    bool TryPush(T&& t) {
        std::scoped_lock lock{write_mutex};
        return spsc_queue.TryPush(std::move(t));
    }

    template <typename... Args>
    bool TryEmplace(Args&&... args) {
        std::scoped_lock lock{write_mutex};
        return spsc_queue.TryEmplace(std::forward<Args>(args)...);
    }

    void PushWait(T&& t) {
        std::scoped_lock lock{write_mutex};
        spsc_queue.PushWait(std::move(t));
    }

    template <typename... Args>
    void EmplaceWait(Args&&... args) {
        std::scoped_lock lock{write_mutex};
        spsc_queue.EmplaceWait(std::forward<Args>(args)...);
    }

    void PushOverwrite(T&& t) {
        std::scoped_lock lock{write_mutex};
        spsc_queue.PushOverwrite(std::move(t));
    }

    template <typename... Args>
    void EmplaceOverwrite(Args&&... args) {
        std::scoped_lock lock{write_mutex};
        spsc_queue.EmplaceOverwrite(std::forward<Args>(args)...);
    }

    bool TryPop(T& t) {
        std::scoped_lock lock{read_mutex};
        return spsc_queue.TryPop(t);
    }

    void PopWait(T& t) {
        std::scoped_lock lock{read_mutex};
        spsc_queue.PopWait(t);
    }

    void PopWait(T& t, std::stop_token stop_token) {
        std::scoped_lock lock{read_mutex};
        spsc_queue.PopWait(t, stop_token);
    }

    T PopWait() {
        std::scoped_lock lock{read_mutex};
        return spsc_queue.PopWait();
    }

    T PopWait(std::stop_token stop_token) {
        std::scoped_lock lock{read_mutex};
        return spsc_queue.PopWait(stop_token);
    }

    void Clear() {
        std::scoped_lock lock{read_mutex};
        spsc_queue.Clear();
    }

    bool Empty() {
        std::scoped_lock lock{read_mutex};
        return spsc_queue.Empty();
    }

    size_t Size() {
        std::scoped_lock lock{read_mutex};
        return spsc_queue.Size();
    }

private:
    SPSCQueue<T, Capacity> spsc_queue;
    std::mutex write_mutex;
    std::mutex read_mutex;
};

} // namespace Common

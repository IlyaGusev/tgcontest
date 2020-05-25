#pragma once

#include <memory>

template<class T>
class THotState {
public:
    THotState() = default;

    std::shared_ptr<T> AtomicGet() const {
        return std::atomic_load(&StatePtr);
    }

    void AtomicSet(std::shared_ptr<T> newStatePtr) {
        std::atomic_store(&StatePtr, newStatePtr);
    }

private:
    std::shared_ptr<T> StatePtr;
};

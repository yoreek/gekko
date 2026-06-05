#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace ewfm {

constexpr size_t kStateMachineStackDepth = 6;

inline bool timeReached(uint32_t now, uint32_t deadline) {
    return static_cast<int32_t>(now - deadline) >= 0;
}

template <typename State> class StateMachine {
public:
    explicit StateMachine(State initial) : state_(initial), previous_(initial) {}

    State state() const {
        return state_;
    }
    State previousState() const {
        return previous_;
    }
    uint32_t enteredAt() const {
        return enteredAt_;
    }
    bool isUpdated() const {
        return updated_;
    }
    bool isPaused() const {
        return paused_;
    }

    bool is(State state) const {
        return state_ == state;
    }

    void transitionTo(State next, uint32_t now) {
        updated_ = false;
        if (state_ == next) {
            return;
        }
        previous_ = state_;
        state_ = next;
        enteredAt_ = now;
        updated_ = true;
    }

    bool elapsed(uint32_t now, uint32_t intervalMs) const {
        return static_cast<uint32_t>(now - enteredAt_) >= intervalMs;
    }
    bool isTimeout(uint32_t now, uint32_t timeoutMs) const {
        return elapsed(now, timeoutMs);
    }
    void resetTimer(uint32_t now) {
        enteredAt_ = now;
        updated_ = true;
    }

    void pause() {
        paused_ = true;
    }
    void restart() {
        paused_ = false;
    }

    bool push(State state) {
        if (stackPos_ >= stack_.size()) {
            return false;
        }
        stack_[stackPos_++] = state;
        return true;
    }

    bool pushCurrent() {
        return push(state_);
    }

    bool pop(State& state) {
        if (stackPos_ == 0) {
            return false;
        }
        state = stack_[--stackPos_];
        return true;
    }

    bool returnToPopped(uint32_t now) {
        State next = state_;
        if (!pop(next)) {
            return false;
        }
        transitionTo(next, now);
        return true;
    }

private:
    State state_;
    State previous_;
    uint32_t enteredAt_{0};
    bool updated_{true};
    bool paused_{false};
    std::array<State, kStateMachineStackDepth> stack_{};
    size_t stackPos_{0};
};

} // namespace ewfm

#define EWFM_SM_GOTO(machine, nextState, now)                                                                                              \
    do {                                                                                                                                   \
        (machine).transitionTo((nextState), (now));                                                                                        \
        return;                                                                                                                            \
    } while (0)

#define EWFM_SM_RESET_TIMER(machine, now)                                                                                                  \
    do {                                                                                                                                   \
        (machine).resetTimer((now));                                                                                                       \
        return;                                                                                                                            \
    } while (0)

#define EWFM_SM_TIMEOUT(machine, now, timeoutMs) ((machine).isTimeout((now), (timeoutMs)))

#define EWFM_SM_TIME_REACHED(now, deadline) (::ewfm::timeReached((now), (deadline)))

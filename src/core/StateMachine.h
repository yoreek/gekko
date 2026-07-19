#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace ewfm {

constexpr size_t kStateMachineStackDepth = 6;

inline bool timeReached(uint32_t now, uint32_t deadline) {
    return static_cast<int32_t>(now - deadline) >= 0;
}

class StateMachine {
public:
    using State = void;
    using PState = State (StateMachine::*)();

    explicit StateMachine(PState current) : initial_(current), current_(current), previous_(current) {}
    virtual ~StateMachine() = default;

    virtual void tick(uint32_t uptime) {
        uptime_ = uptime;
        if (paused_) {
            return;
        }

        const PState oldState = current_;
        (this->*current_)();
        isStateUpdated_ = (oldState != current_);
    }

    void setState(const PState state) {
        previous_ = current_;
        current_ = state;
        stateUpdated_ = uptime_;
        isStateUpdated_ = (previous_ != current_);
    }

    void setState(const PState state, uint32_t uptime) {
        uptime_ = uptime;
        setState(state);
    }

    void transitionTo(const PState state, uint32_t uptime) {
        setState(state, uptime);
    }

    void reset(uint32_t uptime) {
        uptime_ = uptime;
        delay_ = 0;
        current_ = initial_;
        previous_ = initial_;
        stackPos_ = 0;
        paused_ = false;
        isStateUpdated_ = true;
        stateUpdated_ = uptime;
    }

    [[nodiscard]] inline PState getState() const {
        return current_;
    }
    [[nodiscard]] inline PState getPrevState() const {
        return previous_;
    }
    inline void setPrevState() {
        setState(previous_);
    }

    bool pushState(const PState state) {
        if (stackPos_ < kStateMachineStackDepth) {
            stack_[stackPos_++] = state;
            return true;
        }
        return false;
    }
    inline void pushState() {
        (void)pushState(current_);
    }

    PState popState() {
        if (stackPos_ > 0) {
            return stack_[--stackPos_];
        }
        return nullptr;
    }

    bool returnToPopped(uint32_t uptime) {
        PState next = popState();
        if (next == nullptr) {
            return false;
        }
        setState(next, uptime);
        return true;
    }

    [[nodiscard]] bool isStateUpdated() const {
        return isStateUpdated_;
    }
    [[nodiscard]] bool isPaused() const {
        return paused_;
    }
    [[nodiscard]] bool is(const PState state) const {
        return current_ == state;
    }
    [[nodiscard]] uint32_t stateUpdated() const {
        return stateUpdated_;
    }
    [[nodiscard]] uint32_t uptime() const {
        return uptime_;
    }
    [[nodiscard]] bool isTimeout(uint32_t timeoutMs) const {
        return static_cast<uint32_t>(uptime_ - stateUpdated_) > timeoutMs;
    }
    [[nodiscard]] bool elapsed(uint32_t uptime, uint32_t intervalMs) const {
        return static_cast<uint32_t>(uptime - stateUpdated_) >= intervalMs;
    }

    virtual void pause() {
        paused_ = true;
    }
    virtual void restart() {
        paused_ = false;
    }

    State DELAY();

protected:
    uint32_t uptime_{0};
    uint32_t delay_{0};
    PState initial_;
    PState current_;
    PState previous_;
    PState stack_[kStateMachineStackDepth]{};
    uint16_t stackPos_{0};
    bool paused_{false};
    bool isStateUpdated_{true};
    uint32_t stateUpdated_{0};
};

inline StateMachine::State StateMachine::DELAY() {
    if (isTimeout(delay_)) {
        setState(popState());
        return;
    }
}

} // namespace ewfm

#define SM_CLASS StateMachine
#define SM_STATE(s) void SM_CLASS::s()
#define SM_PTR(s) static_cast<PState>(&SM_CLASS::s)
#define SM_NEXT(s) setState(SM_PTR(s))
#define SM_GOTO(s)                                                                                                                         \
    do {                                                                                                                                   \
        setState(SM_PTR(s));                                                                                                               \
        return;                                                                                                                            \
    } while (0)
#define SM_CALL(s, r)                                                                                                                      \
    do {                                                                                                                                   \
        pushState(SM_PTR(r));                                                                                                              \
        SM_GOTO(s);                                                                                                                        \
    } while (0)
#define SM_CALL2(s, r1, r2)                                                                                                                \
    do {                                                                                                                                   \
        pushState(SM_PTR(r1));                                                                                                             \
        pushState(SM_PTR(r2));                                                                                                             \
        SM_GOTO(s);                                                                                                                        \
    } while (0)
#define SM_RETURN()                                                                                                                        \
    do {                                                                                                                                   \
        setState(popState());                                                                                                              \
        return;                                                                                                                            \
    } while (0)
#define SM_RETURN_SUCCESS()                                                                                                                \
    do {                                                                                                                                   \
        popState();                                                                                                                        \
        setState(popState());                                                                                                              \
        return;                                                                                                                            \
    } while (0)
#define SM_RETURN_FAIL()                                                                                                                   \
    do {                                                                                                                                   \
        setState(popState());                                                                                                              \
        popState();                                                                                                                        \
        return;                                                                                                                            \
    } while (0)
#define SM_TIMED_GOTO(s, t)                                                                                                                \
    do {                                                                                                                                   \
        delay_ = (t);                                                                                                                      \
        SM_CALL(DELAY, s);                                                                                                                 \
    } while (0)
#define EWFM_SM_TIME_REACHED(now, deadline) (::ewfm::timeReached((now), (deadline)))

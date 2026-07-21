#include "backtest/strategy/moving_average_cross.hpp"

#include <stdexcept>

namespace bt {

MovingAverageCross::MovingAverageCross(std::size_t fast_window, std::size_t slow_window)
    : fast_(fast_window), slow_(slow_window) {
    if (fast_window == 0 || slow_window == 0) {
        throw std::invalid_argument("MovingAverageCross: windows must be > 0");
    }
    if (fast_window >= slow_window) {
        throw std::invalid_argument("MovingAverageCross: fast window must be < slow window");
    }
}

std::optional<SignalEvent> MovingAverageCross::on_bar(const Bar& bar) {
    const double close = bar.close;
    window_.push_back(close);
    fast_sum_ += close;
    slow_sum_ += close;

    // Slide the fast window: drop the close that just fell out of the last
    // `fast_` elements.
    if (window_.size() > fast_) {
        fast_sum_ -= window_[window_.size() - fast_ - 1];
    }
    // Cap the deque at the slow window, dropping the oldest close.
    if (window_.size() > slow_) {
        slow_sum_ -= window_.front();
        window_.pop_front();
    }

    if (window_.size() < slow_) {
        return std::nullopt;  // not enough history to compare the two averages yet
    }

    const double fast_ma = fast_sum_ / static_cast<double>(fast_);
    const double slow_ma = slow_sum_ / static_cast<double>(slow_);
    const int sign = (fast_ma > slow_ma) ? 1 : (fast_ma < slow_ma) ? -1 : 0;

    std::optional<SignalEvent> signal;
    // A signal only fires on a genuine flip between above and below.
    if (sign != 0 && prev_sign_ != 0 && sign != prev_sign_) {
        const SignalType type = (sign > 0) ? SignalType::Long : SignalType::Short;
        signal = SignalEvent{bar.timestamp, type, 1.0};
    }
    if (sign != 0) {
        prev_sign_ = sign;
    }
    return signal;
}

void MovingAverageCross::reset() {
    window_.clear();
    fast_sum_ = 0.0;
    slow_sum_ = 0.0;
    prev_sign_ = 0;
}

std::string MovingAverageCross::name() const {
    return "MA Cross (" + std::to_string(fast_) + "/" + std::to_string(slow_) + ")";
}

}  // namespace bt

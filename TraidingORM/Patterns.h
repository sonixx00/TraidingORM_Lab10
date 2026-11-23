#pragma once

#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <functional>
#include <map>
#include <utility>
#include <algorithm>
#include <cmath>

// ========================================
// VARIANT 15: STOCK EXCHANGE APPLICATIONS
// ========================================

// 1. STATE

enum class ApplicationStatus {
    Created,
    Verified,
    Executed,
    Rejected
};

class ApplicationState {
private:
    ApplicationStatus currentStatus = ApplicationStatus::Created;

    std::vector<ApplicationStatus> history{
        ApplicationStatus::Created
    };

public:
    ApplicationStatus getStatus() const {
        return currentStatus;
    }

    const std::vector<ApplicationStatus>& getHistory() const {
        return history;
    }

    void changeStatus(ApplicationStatus next) {
        bool allowed = false;

        switch (currentStatus) {
        case ApplicationStatus::Created:
            allowed =
                next == ApplicationStatus::Verified ||
                next == ApplicationStatus::Rejected;
            break;

        case ApplicationStatus::Verified:
            allowed =
                next == ApplicationStatus::Executed ||
                next == ApplicationStatus::Rejected;
            break;

        case ApplicationStatus::Executed:
        case ApplicationStatus::Rejected:
            allowed = false;
            break;
        }

        if (!allowed) {
            throw std::logic_error(
                "Invalid application status transition"
            );
        }

        currentStatus = next;
        history.push_back(next);
    }
};

// ========================================
// 2. STRATEGY
// ========================================

struct ApplicationParameters {
    double volume;
    double urgency;
    double waitingTime;
};

class IPriorityStrategy {
public:
    virtual ~IPriorityStrategy() = default;

    virtual double calculate(
        const ApplicationParameters& parameters
    ) const = 0;
};

// Strategy 1: Maximum of normalized parameters

class MaximumNormalizedStrategy : public IPriorityStrategy {
public:
    double calculate(
        const ApplicationParameters& p
    ) const override {
        return std::max({
            p.volume / 1000.0,
            p.urgency / 10.0,
            p.waitingTime / 60.0
            });
    }
};

// Strategy 2: Weighted sum

class WeightedSumStrategy : public IPriorityStrategy {
public:
    double calculate(
        const ApplicationParameters& p
    ) const override {
        return p.volume * 0.4 +
            p.urgency * 0.35 +
            p.waitingTime * 0.25;
    }
};

// Strategy 3: Normalized weighted sum

class NormalizedWeightedSumStrategy : public IPriorityStrategy {
public:
    double calculate(
        const ApplicationParameters& p
    ) const override {
        return (p.volume / 1000.0) * 0.4 +
            (p.urgency / 10.0) * 0.35 +
            (p.waitingTime / 60.0) * 0.25;
    }
};

class PriorityCalculator {
private:
    std::unique_ptr<IPriorityStrategy> strategy;

public:
    explicit PriorityCalculator(
        std::unique_ptr<IPriorityStrategy> initial
    ) : strategy(std::move(initial)) {
        if (!strategy) {
            throw std::invalid_argument("Strategy is null");
        }
    }

    void setStrategy(
        std::unique_ptr<IPriorityStrategy> next
    ) {
        if (!next) {
            throw std::invalid_argument("Strategy is null");
        }

        strategy = std::move(next);
    }

    double calculate(
        const ApplicationParameters& parameters
    ) const {
        if (parameters.volume < 0 ||
            parameters.urgency < 0 ||
            parameters.waitingTime < 0) {
            throw std::invalid_argument(
                "Parameters cannot be negative"
            );
        }

        return strategy->calculate(parameters);
    }
};

// ========================================
// 3. OBSERVER
// ========================================

enum class ApplicationEventType {
    Registered,
    StatusChanged
};
struct ApplicationEvent {
    ApplicationEventType type;
    int applicationId;
    std::string message;
};

class ApplicationEventPublisher {
private:
    using Callback =
        std::function<void(const ApplicationEvent&)>;

    struct Subscriber {
        ApplicationEventType type;
        Callback callback;
    };

    std::map<int, Subscriber> subscribers;
    int nextId = 1;

public:
    int subscribe(
        ApplicationEventType type,
        Callback callback
    ) {
        if (!callback) {
            throw std::invalid_argument("Empty callback");
        }

        int id = nextId++;

        subscribers.emplace(
            id,
            Subscriber{ type, std::move(callback) }
        );

        return id;
    }

    void unsubscribe(int id) {
        subscribers.erase(id);
    }

    void notify(const ApplicationEvent& event) {
        auto snapshot = subscribers;

        for (const auto& [id, subscriber] : snapshot) {
            if (subscribers.contains(id) &&
                subscriber.type == event.type) {
                subscriber.callback(event);
            }
        }
    }
};

// Subscriber 1: Trader journal

class TraderJournal {
public:
    std::vector<std::string> entries;

    void onEvent(const ApplicationEvent& event) {
        entries.push_back(
            "Application #" +
            std::to_string(event.applicationId) +
            ": " + event.message
        );
    }
};

// Subscriber 2: Application statistics

class ApplicationStatistics {
public:
    int registeredCount = 0;
    int statusChangedCount = 0;

    void onEvent(const ApplicationEvent& event) {
        if (event.type == ApplicationEventType::Registered) {
            ++registeredCount;
        }
        else if (
            event.type == ApplicationEventType::StatusChanged
            ) {
            ++statusChangedCount;
        }
    }
};

// ========================================
// 4. FACTORY METHOD
// ========================================

class PriorityStrategyFactory {
public:
    virtual ~PriorityStrategyFactory() = default;

    virtual std::unique_ptr<IPriorityStrategy>
        createStrategy() const = 0;
};

class MaximumPriorityFactory : public PriorityStrategyFactory {
public:
    std::unique_ptr<IPriorityStrategy>
        createStrategy() const override {
        return std::make_unique<MaximumNormalizedStrategy>();
    }
};

class WeightedPriorityFactory : public PriorityStrategyFactory {
public:
    std::unique_ptr<IPriorityStrategy>
        createStrategy() const override {
        return std::make_unique<WeightedSumStrategy>();
    }
};

class NormalizedPriorityFactory : public PriorityStrategyFactory {
public:
    std::unique_ptr<IPriorityStrategy>
        createStrategy() const override {
        return std::make_unique<NormalizedWeightedSumStrategy>();
    }
};
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <sqlite3.h>
#include <sqlite_orm/sqlite_orm.h>
#include "Patterns.h"

using namespace std;
using namespace sqlite_orm;

// ===============================
// 1. DATA MODELS
// ===============================

struct Product {
    int id;
    string name;
    double price;
    int quantity;
};

struct Order {
    int id;
    int productId;
    int quantity;
    string orderDate;
};

// ===============================
// 2. ORM DATABASE
// ===============================
struct ApplicationStatusHistory {
    int id;
    int applicationId;
    string oldStatus;
    string newStatus;
    string changedAt;
};
auto createStorage(const string& filename) {
    return make_storage(filename,
        make_table("Products",
            make_column("id", &Product::id,
                primary_key().autoincrement()),
            make_column("name", &Product::name),
            make_column("price", &Product::price),
            make_column("quantity", &Product::quantity)
        ),
        make_table("Orders",
            make_column("id", &Order::id,
                primary_key().autoincrement()),
            make_column("productId", &Order::productId),
            make_column("quantity", &Order::quantity),
            make_column("orderDate", &Order::orderDate),
            foreign_key(&Order::productId)
            .references(&Product::id)
        ),
        make_table("ApplicationStatusHistory",
            make_column("id", &ApplicationStatusHistory::id,
                primary_key().autoincrement()),
            make_column("applicationId",
                &ApplicationStatusHistory::applicationId),
            make_column("oldStatus",
                &ApplicationStatusHistory::oldStatus),
            make_column("newStatus",
                &ApplicationStatusHistory::newStatus),
            make_column("changedAt",
                &ApplicationStatusHistory::changedAt)
        )
    );
}

using Storage = decltype(createStorage("trading.db"));

// ===============================
// 3. REPOSITORY INTERFACE
// ===============================

class IProductRepository {
public:
    virtual int add(const Product& product) = 0;
    virtual vector<Product> getAll() = 0;
    virtual Product getById(int id) = 0;
    virtual void update(const Product& product) = 0;
    virtual void remove(int id) = 0;

    virtual ~IProductRepository() = default;
};

// ===============================
// 4. PRODUCT REPOSITORY
// ===============================

class ProductRepository : public IProductRepository {
private:
    Storage& storage;

public:
    explicit ProductRepository(Storage& db)
        : storage(db) {
    }

    int add(const Product& product) override {
        return storage.insert(product);
    }

    vector<Product> getAll() override {
        return storage.get_all<Product>();
    }

    Product getById(int id) override {
        return storage.get<Product>(id);
    }

    void update(const Product& product) override {
        storage.update(product);
    }

    void remove(int id) override {
        storage.remove<Product>(id);
    }
};

// ===============================
// 5. ORDER DATA ACCESS
// ===============================

class OrderRepository {
private:
    Storage& storage;

public:
    explicit OrderRepository(Storage& db)
        : storage(db) {
    }

    int add(const Order& order) {
        return storage.insert(order);
    }

    vector<Order> getAll() {
        return storage.get_all<Order>();
    }
};

// ===============================
// 6. OUTPUT FUNCTIONS
// ===============================

void printProducts(IProductRepository& repository) {
    cout << "\n=== PRODUCTS ===\n";

    for (const auto& p : repository.getAll()) {
        cout << "ID: " << p.id
            << " | Name: " << p.name
            << " | Price: " << p.price
            << " | Quantity: " << p.quantity
            << endl;
    }
}

void printOrders(OrderRepository& repository) {
    cout << "\n=== ORDERS ===\n";

    for (const auto& o : repository.getAll()) {
        cout << "ID: " << o.id
            << " | Product ID: " << o.productId
            << " | Quantity: " << o.quantity
            << " | Date: " << o.orderDate
            << endl;
    }
}

// ===============================
// 7. SQLITE C API
// ===============================

void demonstrateRawSQLite(const string& filename) {
    cout << "\n=== SQLITE C API ===\n";

    sqlite3* db = nullptr;

    if (sqlite3_open(filename.c_str(), &db) != SQLITE_OK) {
        cout << "Cannot open database\n";

        if (db) {
            sqlite3_close(db);
        }

        return;
    }

    const char* sql =
        "SELECT COUNT(*) FROM Products;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(
        db, sql, -1, &stmt, nullptr) == SQLITE_OK) {

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            cout << "Products count (C API): "
                << sqlite3_column_int(stmt, 0)
                << endl;
        }
    }
    else {
        cout << "SQL preparation error\n";
    }

    if (stmt) {
        sqlite3_finalize(stmt);
    }

    sqlite3_close(db);
}

// ===============================
// 8. MAIN PROGRAM
// ===============================

int main() {
    try {
        // Use a separate demo database.
        // The previous trading.db is not modified.
        const string dbName = "trading_lab8.db";

        auto storage = createStorage(dbName);
        storage.on_open = [](sqlite3* db) {
            sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
            };
        storage.sync_schema();
    // ===== LAB 10: VARIANT 15 =====
    cout << "\n=== LAB 10: STOCK EXCHANGE APPLICATIONS ===\n";

    // 1. STATE
    ApplicationState applicationState;
    const int applicationId = 1;

    auto saveStatusChange = [&](ApplicationStatus next) {
        ApplicationStatus previous = applicationState.getStatus();

        storage.transaction([&]() {
            applicationState.changeStatus(next);

            auto statusName = [](ApplicationStatus status) {
                switch (status) {
                case ApplicationStatus::Created: return string("Created");
                case ApplicationStatus::Verified: return string("Verified");
                case ApplicationStatus::Executed: return string("Executed");
                case ApplicationStatus::Rejected: return string("Rejected");
                }
                return string("Unknown");
                };

            storage.insert(ApplicationStatusHistory{
                -1,
                applicationId,
                statusName(previous),
                statusName(next),
                "2026-11-23"
                });

            return true;
            });
        };

    saveStatusChange(ApplicationStatus::Verified);
    saveStatusChange(ApplicationStatus::Executed);
    auto savedHistory = storage.get_all<ApplicationStatusHistory>();

    cout << "\n=== SQLITE STATUS HISTORY ===\n";

    for (const auto& record : savedHistory) {
        cout << "Application #" << record.applicationId
            << ": " << record.oldStatus
            << " -> " << record.newStatus
            << " | Date: " << record.changedAt << endl;
    }

    cout << "Total history records: "
        << savedHistory.size() << endl;
    cout << "STATE: valid transitions = "
        << applicationState.getHistory().size() - 1 << endl;

    // Check invalid transition
    try {
        applicationState.changeStatus(ApplicationStatus::Rejected);
    }
    catch (const std::logic_error& e) {
        cout << "STATE: invalid transition blocked" << endl;
        // ===== LAB 10: PATTERN TESTS =====
        cout << "\n=== LAB 10: PATTERN TESTS ===\n";

        int passed = 0;
        int failed = 0;

        auto check = [&](bool condition, const string& name) {
            cout << (condition ? "[PASS] " : "[FAIL] ") << name << endl;
            if (condition) ++passed;
            else ++failed;
            };

        // STATE: valid transitions
        ApplicationState testState;
        testState.changeStatus(ApplicationStatus::Verified);
        testState.changeStatus(ApplicationStatus::Executed);

        check(
            testState.getStatus() == ApplicationStatus::Executed &&
            testState.getHistory().size() == 3,
            "State: valid transitions"
        );

        // STATE: rejected application
        ApplicationState rejectedState;
        rejectedState.changeStatus(ApplicationStatus::Rejected);

        check(
            rejectedState.getStatus() == ApplicationStatus::Rejected,
            "State: application rejected"
        );

        // STATE: invalid transition
        bool invalidBlocked = false;
        try {
            rejectedState.changeStatus(ApplicationStatus::Executed);
        }
        catch (const std::logic_error&) {
            invalidBlocked = true;
        }

        check(invalidBlocked, "State: invalid transition blocked");

        // STRATEGY: three algorithms
        ApplicationParameters testParams{ 500, 8, 30 };

        MaximumNormalizedStrategy maxStrategy;
        WeightedSumStrategy weightedStrategy;
        NormalizedWeightedSumStrategy normalizedStrategy;

        check(
            std::abs(maxStrategy.calculate(testParams) - 0.8) < 0.000001,
            "Strategy: maximum normalized"
        );

        check(
            std::abs(weightedStrategy.calculate(testParams) - 210.3) < 0.000001,
            "Strategy: weighted sum"
        );

        check(
            std::abs(normalizedStrategy.calculate(testParams) - 0.605) < 0.000001,
            "Strategy: normalized weighted sum"
        );

        // OBSERVER: subscribe and unsubscribe
        ApplicationEventPublisher testPublisher;
        int received = 0;

        int subscriptionId = testPublisher.subscribe(
            ApplicationEventType::Registered,
            [&](const ApplicationEvent&) {
                ++received;
            }
        );

        testPublisher.notify({
            ApplicationEventType::Registered, 2, "Registered"
            });

        testPublisher.unsubscribe(subscriptionId);

        testPublisher.notify({
            ApplicationEventType::Registered, 2, "Registered again"
            });

        check(received == 1, "Observer: subscribe/unsubscribe");

        // FACTORY METHOD
        NormalizedPriorityFactory testFactory;
        auto createdStrategy = testFactory.createStrategy();

        check(
            std::abs(createdStrategy->calculate(testParams) - 0.605) < 0.000001,
            "Factory Method: strategy creation"
        );

        cout << "\nTEST RESULTS: "
            << passed << " passed, "
            << failed << " failed\n";
    }

    // 2. STRATEGY
    ApplicationParameters parameters{ 500, 8, 30 };

    PriorityCalculator calculator(
        std::make_unique<MaximumNormalizedStrategy>()
    );

    cout << "STRATEGY (maximum normalized): "
        << calculator.calculate(parameters) << endl;

    calculator.setStrategy(
        std::make_unique<WeightedSumStrategy>()
    );

    cout << "STRATEGY (weighted sum): "
        << calculator.calculate(parameters) << endl;

    calculator.setStrategy(
        std::make_unique<NormalizedWeightedSumStrategy>()
    );

    cout << "STRATEGY (normalized weighted sum): "
        << calculator.calculate(parameters) << endl;

    // 3. OBSERVER
    ApplicationEventPublisher publisher;
    TraderJournal journal;
    ApplicationStatistics statistics;

    int journalRegisteredId = publisher.subscribe(
        ApplicationEventType::Registered,
        [&](const ApplicationEvent& event) {
            journal.onEvent(event);
        }
    );

    int journalStatusId = publisher.subscribe(
        ApplicationEventType::StatusChanged,
        [&](const ApplicationEvent& event) {
            journal.onEvent(event);
        }
    );

    int statisticsRegisteredId = publisher.subscribe(
        ApplicationEventType::Registered,
        [&](const ApplicationEvent& event) {
            statistics.onEvent(event);
        }
    );

    int statisticsStatusId = publisher.subscribe(
        ApplicationEventType::StatusChanged,
        [&](const ApplicationEvent& event) {
            statistics.onEvent(event);
        }
    );

    publisher.notify({
        ApplicationEventType::Registered,
        1,
        "Application registered"
        });

    publisher.notify({
        ApplicationEventType::StatusChanged,
        1,
        "Application status changed"
        });

    cout << "OBSERVER: journal entries = "
        << journal.entries.size() << endl;

    cout << "OBSERVER: registered = "
        << statistics.registeredCount << endl;

    cout << "OBSERVER: status changes = "
        << statistics.statusChangedCount << endl;

    publisher.unsubscribe(journalRegisteredId);
    publisher.unsubscribe(journalStatusId);
    publisher.unsubscribe(statisticsRegisteredId);
    publisher.unsubscribe(statisticsStatusId);

    // 4. FACTORY METHOD
    NormalizedPriorityFactory factory;
    auto strategy = factory.createStrategy();

    cout << "FACTORY METHOD: priority = "
        << strategy->calculate(parameters) << endl;

    cout << "=== PATTERN DEMONSTRATION COMPLETED ===\n\n";
    

        // Enable foreign key checks.


        ProductRepository productRepo(storage);
        OrderRepository orderRepo(storage);

        IProductRepository& repository = productRepo;

        // ===============================
        // 9. INITIAL DATA
        // ===============================

        cout << "=== DATABASE INITIALIZATION ===\n";

        // Clear old demo records.
        storage.remove_all<Order>();
        storage.remove_all<Product>();

        vector<Product> products = {
            {-1, "Laptop", 25000, 5},
            {-1, "Mouse", 500, 20},
            {-1, "Keyboard", 1200, 10},
            {-1, "Monitor", 7000, 8},
            {-1, "Printer", 4500, 6}
        };

        vector<int> productIds;

        for (const auto& p : products) {
            productIds.push_back(repository.add(p));
        }

        // Add 15 related orders.
        for (int i = 0; i < 15; i++) {
            Order order{
                -1,
                productIds[i % 5],
                i + 1,
                "2026-11-16"
            };

            orderRepo.add(order);
        }

        cout << "5 products added successfully.\n";
        cout << "15 orders added successfully.\n";

        printProducts(repository);
        printOrders(orderRepo);

        // ===============================
        // 10. CRUD OPERATIONS
        // ===============================

        cout << "\n=== CRUD OPERATIONS ===\n";

        // CREATE
        int testId = repository.add(
            { -1, "Test Product", 100, 3 }
        );

        cout << "CREATE: Product added, ID = "
            << testId << endl;

        // READ
        Product testProduct = repository.getById(testId);

        cout << "READ: "
            << testProduct.name << endl;

        // UPDATE
        testProduct.price = 150;
        repository.update(testProduct);

        cout << "UPDATE: New price = "
            << repository.getById(testId).price
            << endl;

        // DELETE
        repository.remove(testId);

        cout << "DELETE: Test product removed\n";

        // ===============================
        // 11. FILTERING
        // ===============================

        cout << "\n=== FILTER: PRICE > 1000 ===\n";

        auto filtered = storage.get_all<Product>(
            where(c(&Product::price) > 1000.0)
        );

        for (const auto& p : filtered) {
            cout << p.name
                << " | Price: " << p.price
                << endl;
        }

        // ===============================
        // 12. SORTING AND LIMIT
        // ===============================

        cout << "\n=== SORT BY PRICE DESC, LIMIT 3 ===\n";

        auto sorted = storage.get_all<Product>(
            order_by(&Product::price).desc(),
            limit(3)
        );

        for (const auto& p : sorted) {
            cout << p.name
                << " | Price: " << p.price
                << endl;
        }

        // ===============================
        // 13. AGGREGATION
        // ===============================

        cout << "\n=== AGGREGATION ===\n";

        cout << "Products count: "
            << storage.count<Product>()
            << endl;

        auto totalQuantity = storage.sum(&Product::quantity);

        if (totalQuantity) {
            cout << "Total quantity: "
                << *totalQuantity << endl;
        }

        // ===============================
        // 14. GROUPING
        // ===============================

        cout << "\n=== GROUP BY PRODUCT ===\n";

        auto grouped = storage.select(
            columns(
                &Order::productId,
                count(&Order::id)
            ),
            group_by(&Order::productId)
        );

        for (const auto& row : grouped) {
            cout << "Product ID: "
                << get<0>(row)
                << " | Orders: "
                << get<1>(row)
                << endl;
        }

        // ===============================
        // 15. JOIN
        // ===============================

        cout << "\n=== JOIN PRODUCTS AND ORDERS ===\n";

        auto joined = storage.select(
            columns(
                &Product::name,
                &Order::id,
                &Order::quantity,
                &Order::orderDate
            ),
            inner_join<Order>(
                on(c(&Product::id) == &Order::productId)
            )
        );

        for (const auto& row : joined) {
            cout << "Product: " << get<0>(row)
                << " | Order ID: " << get<1>(row)
                << " | Quantity: " << get<2>(row)
                << " | Date: " << get<3>(row)
                << endl;
        }

        // ===============================
        // 16. SUCCESSFUL TRANSACTION
        // ===============================

        cout << "\n=== TRANSACTION COMMIT ===\n";

        int firstId = productIds[0];

        int before =
            repository.getById(firstId).quantity;

        bool committed = storage.transaction([&]() {
            Product p = repository.getById(firstId);

            p.quantity -= 1;
            repository.update(p);

            orderRepo.add({
                -1,
                firstId,
                1,
                "2026-11-16"
                });

            return true;
            });

        int after =
            repository.getById(firstId).quantity;

        cout << "Before: " << before << endl;
        cout << "After: " << after << endl;
        cout << "COMMIT: "
            << (committed ? "SUCCESS" : "FAILED")
            << endl;

        // ===============================
        // 17. ROLLBACK TRANSACTION
        // ===============================

        cout << "\n=== TRANSACTION ROLLBACK ===\n";

        before =
            repository.getById(firstId).quantity;

        try {
            storage.transaction([&]() {
                Product p = repository.getById(firstId);

                p.quantity -= 1;
                repository.update(p);

                // Invalid foreign key causes an error.
                orderRepo.add({
                    -1,
                    999999,
                    1,
                    "2026-11-16"
                    });

                return true;
                });
        }
        catch (const exception& e) {
            cout << "Expected error: "
                << e.what() << endl;
        }

        after =
            repository.getById(firstId).quantity;

        cout << "Before: " << before << endl;
        cout << "After: " << after << endl;

        if (before == after) {
            cout << "ROLLBACK SUCCESSFUL\n";
        }
        else {
            cout << "ROLLBACK CHECK FAILED\n";
        }

        // ===============================
        // 18. WITHOUT TRANSACTION
        // ===============================

        cout << "\n=== WITHOUT TRANSACTION ===\n";

        before =
            repository.getById(firstId).quantity;

        Product p = repository.getById(firstId);

        p.quantity -= 1;
        repository.update(p);

        try {
            orderRepo.add({
                -1,
                999999,
                1,
                "2026-11-16"
                });
        }
        catch (const exception& e) {
            cout << "Expected error: "
                << e.what() << endl;
        }

        after =
            repository.getById(firstId).quantity;

        cout << "Before: " << before << endl;
        cout << "After: " << after << endl;

        cout << "Without transaction, first change remains.\n";

        // ===============================
        // 19. RAW API COMPARISON
        // ===============================

        demonstrateRawSQLite(dbName);

        cout << "\n=== ORM API ===\n";

        cout << "Products count (ORM): "
            << storage.count<Product>()
            << endl;

        // ===============================
        // 20. FINAL RESULTS
        // ===============================

        cout << "\n=== FINAL DATABASE STATE ===\n";

        printProducts(repository);

        cout << "\nTotal products: "
            << storage.count<Product>()
            << endl;

        cout << "Total orders: "
            << storage.count<Order>()
            << endl;

        cout << "\nLABORATORY WORK COMPLETED\n";

        return 0;
    }
    catch (const exception& e) {
        cerr << "\nERROR: " << e.what() << endl;
        return 1;
    }
}
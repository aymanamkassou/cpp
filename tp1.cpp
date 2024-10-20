#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstdio>  // For file handling
#define MAX_ACCOUNTS 50
#pragma warning(disable : 4996)

// --- Currency Class --- //
// Handles currency conversions and holds the currency code.
class Currency {
    std::string code;            // Currency code (e.g., USD, EUR)
    double conversionRateToBase; // Conversion rate to base currency (e.g., USD)

public:
    Currency(const std::string& code, double rate) : code(code), conversionRateToBase(rate) {}

    double convertToBase(double amount) const {
        return amount * conversionRateToBase;
    }

    double convertFromBase(double amount) const {
        return amount / conversionRateToBase;
    }

    const std::string& getCode() const {
        return code;
    }

    void saveToFile(FILE* file) const {
        fprintf(file, "%s %.2f\n", code.c_str(), conversionRateToBase);
    }

    static Currency loadFromFile(FILE* file) {
        char currencyCode[4];
        double rate;
        fscanf(file, "%s %lf", currencyCode, &rate);
        return Currency(std::string(currencyCode), rate);
    }
};

// --- TransactionValidator Class --- //
// Responsible for validating transactions.
class TransactionValidator {
public:
    void validateDeposit(double amount) const {
        if (amount <= 0) throw std::invalid_argument("Deposit amount must be positive.");
    }

    void validateWithdraw(double balance, double amount) const {
        if (amount <= 0) throw std::invalid_argument("Withdraw amount must be positive.");
        if (balance < amount) throw std::invalid_argument("Insufficient funds.");
    }

    void validateTransfer(double amount) const {
        if (amount <= 0) throw std::invalid_argument("Transfer amount must be positive.");
    }
};

// --- Account Class --- //
// Manages multiple currencies and transactions.
class Account {
    std::string owner;
    int id;
    std::vector<double> balances;            // Balances for each currency.
    std::vector<Currency> currencies;        // Currencies associated with the account.
    TransactionValidator validator;

public:
    Account(int id, const std::string& owner) : id(id), owner(owner) {}

    // Adds a currency to the account.
    void addCurrency(const Currency& currency) {
        currencies.push_back(currency);
        balances.push_back(0.0);
    }

    // Deposits money into a specific currency.
    void deposit(double amount, const std::string& currencyCode) {
        validator.validateDeposit(amount);
        for (size_t i = 0; i < currencies.size(); ++i) {
            if (currencies[i].getCode() == currencyCode) {
                balances[i] += currencies[i].convertToBase(amount);
                return;
            }
        }
        throw std::invalid_argument("Currency not found in account.");
    }

    // Withdraws money from a specific currency.
    void withdraw(double amount, const std::string& currencyCode) {
        for (size_t i = 0; i < currencies.size(); ++i) {
            if (currencies[i].getCode() == currencyCode) {
                double baseAmount = currencies[i].convertToBase(amount);
                validator.validateWithdraw(balances[i], baseAmount);
                balances[i] -= baseAmount;
                return;
            }
        }
        throw std::invalid_argument("Currency not found in account.");
    }

    // Transfers money between accounts, converting currencies as needed.
    void transfer(Account& toAccount, double amount, const std::string& fromCurrency, const std::string& toCurrency) {
        withdraw(amount, fromCurrency);
        toAccount.deposit(amount, toCurrency);
    }

    // Display the account's balances in all currencies.
    void display() const {
        std::cout << "Account ID: " << id << " | Owner: " << owner << std::endl;
        for (size_t i = 0; i < currencies.size(); ++i) {
            std::cout << "Balance in " << currencies[i].getCode() << ": "
                << currencies[i].convertFromBase(balances[i]) << std::endl;
        }
    }

    // Save account information and balances to a file.
    void saveToFile(FILE* file) const {
        fprintf(file, "%d %s %zu\n", id, owner.c_str(), currencies.size());
        for (size_t i = 0; i < currencies.size(); ++i) {
            currencies[i].saveToFile(file);
            fprintf(file, "%.2f\n", balances[i]);
        }
    }

    // Load account information and balances from a file.
    static Account loadFromFile(FILE* file) {
        int accountId;
        char ownerBuffer[100];
        size_t currencyCount;

        fscanf(file, "%d %s %zu", &accountId, ownerBuffer, &currencyCount);

        Account account(accountId, std::string(ownerBuffer));
        for (size_t i = 0; i < currencyCount; ++i) {
            Currency currency = Currency::loadFromFile(file);
            double balance;
            fscanf(file, "%lf", &balance);
            account.addCurrency(currency);
            account.balances.back() = balance;
        }
        return account;
    }
};

// --- AccountManager Class --- //
// Manages multiple accounts and handles file I/O.
class AccountManager {
    Account* accounts[MAX_ACCOUNTS];  // Use array for accounts
    int accountCount;

public:
    AccountManager() : accountCount(0) {}

    // Adds a new account.
    void addAccount(Account* account) {
        if (accountCount >= MAX_ACCOUNTS) throw std::overflow_error("Maximum accounts reached.");
        accounts[accountCount++] = account;
    }

    // Display all accounts.
    void displayAllAccounts() const {
        for (int i = 0; i < accountCount; ++i) {
            accounts[i]->display();
        }
    }

    // Save all account data to a file.
    void saveAccountsToFile(const std::string& filename) const {
        FILE* file = fopen(filename.c_str(), "w");
        if (!file) throw std::runtime_error("Unable to open file for writing.");

        for (int i = 0; i < accountCount; ++i) {
            accounts[i]->saveToFile(file);
        }

        fclose(file);
    }

    // Load all account data from a file.
    void loadAccountsFromFile(const std::string& filename) {
        FILE* file = fopen(filename.c_str(), "r");
        if (!file) throw std::runtime_error("Unable to open file for reading.");

        while (!feof(file)) {
            Account account = Account::loadFromFile(file);
            addAccount(new Account(account));
        }

        fclose(file);
    }
};

int main() {
    // Define currencies.
    Currency usd("USD", 1.0);   // Base currency.
    Currency eur("EUR", 1.1);   // 1 EUR = 1.1 USD.
    Currency gbp("GBP", 1.3);   // 1 GBP = 1.3 USD.

    // Create the Account Manager.
    AccountManager manager;

    // Create two accounts.
    Account* account1 = new Account(1, "Alice");
    Account* account2 = new Account(2, "Bob");

    // Add currencies to accounts.
    account1->addCurrency(usd);
    account1->addCurrency(eur);
    account2->addCurrency(usd);
    account2->addCurrency(gbp);

    // Add accounts to the manager.
    manager.addAccount(account1);
    manager.addAccount(account2);

    // Perform transactions.
    account1->deposit(1000, "USD");
    account1->withdraw(100, "EUR");
    account1->transfer(*account2, 200, "USD", "GBP");

    // Display all accounts.
    manager.displayAllAccounts();

    // Save accounts to file.
    manager.saveAccountsToFile("accounts.txt");

    // Load accounts from file (clearing the manager first).
    AccountManager newManager;
    newManager.loadAccountsFromFile("accounts.txt");

    // Display loaded accounts.
    newManager.displayAllAccounts();

    return 0;
}

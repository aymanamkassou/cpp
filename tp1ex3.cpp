#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstdio>  
#define MAX_ACCOUNTS 50


class currency {
    std::string code;           
    double conversion_rate_to_base; 

public:
    currency(const std::string& code, double rate) : code(code), conversion_rate_to_base(rate) {}

    double convert_to_base(double amount) const {
        return amount * conversion_rate_to_base;
    }

    double convert_from_base(double amount) const {
        return amount / conversion_rate_to_base;
    }


    void save_to_file(FILE* file) const {
        fprintf(file, "%s %.2f\n", code.c_str(), conversion_rate_to_base);
    }

    static currency load_from_file(FILE* file) {
        char currency_code[4];
        double rate;
        fscanf(file, "%s %lf", currency_code, &rate);
        return currency(std::string(currency_code), rate);
    }

    friend class account;
};


class transaction_validator {
public:
    void validate_deposit(double amount) const {
        if (amount <= 0) return;
    }

    void validate_withdraw(double balance, double amount) const {
        if (amount <= 0) return;
        if (balance < amount) return;
    }

    void validate_transfer(double amount) const {
        if (amount <= 0) return;
    }
};


class account {
protected:
    std::string owner;
    int id;
    std::vector<double> balances;            
    std::vector<currency> currencies;        
    transaction_validator validator;

public:
    account(int id, const std::string& owner) : id(id), owner(owner) {}

    void add_currency(const currency& currency) {
        currencies.push_back(currency);
        balances.push_back(0.0);
    }


    void deposit(double amount, const std::string& currency_code) {
        validator.validate_deposit(amount);
        for (int i = 0; i < currencies.size(); i++) {
            if (currencies[i].code == currency_code) {
                balances[i] += currencies[i].convert_to_base(amount);
                return;
            }
        }
    }


    void withdraw(double amount, const std::string& currency_code) {
        for (int i = 0; i < currencies.size(); i++) {
            if (currencies[i].code == currency_code) {
                double base_amount = currencies[i].convert_to_base(amount);
                validator.validate_withdraw(balances[i], base_amount);
                balances[i] -= base_amount;
                return;
            }
        }
    }

 
    void transfer(account& to_account, double amount, const std::string& from_currency, const std::string& to_currency) {
        validator.validate_transfer(amount);
        withdraw(amount, from_currency);
        to_account.deposit(amount, to_currency);
    }

    
    void display() const {
        std::cout << "Account ID: " << id << " | Owner: " << owner << std::endl;
        for (int i = 0; i < currencies.size(); i++) {
            std::cout << "Balance in " << currencies[i].code << ": "
                << currencies[i].convert_from_base(balances[i]) << std::endl;
        }
    }

 
    void save_to_file(FILE* file) const {
        fprintf(file, "%d %s %zu\n", id, owner.c_str(), currencies.size());
        for (int i = 0; i < currencies.size(); i++) {
            currencies[i].save_to_file(file);
            fprintf(file, "%.2f\n", balances[i]);
        }
    }

    
    static account load_from_file(FILE* file) {
        int account_id;
        char owner_buffer[100];
        int currency_count;

        fscanf(file, "%d %s %zu", &account_id, owner_buffer, &currency_count);

        account account(account_id, std::string(owner_buffer));
        for (int i = 0; i < currency_count; ++i) {
            currency currency = currency::load_from_file(file);
            double balance;
            fscanf(file, "%lf", &balance);
            account.add_currency(currency);
            account.balances.back() = balance;
        }
        return account;
    }
};

class savings_account : public account {
    double interest_rate;
public:
    savings_account(int id, const std::string& owner, double rate) : account(id,owner) {}

};

class account_manager {
    account* accounts[MAX_ACCOUNTS];
    int account_count;

public:
    account_manager() : account_count(0) {}


    void add_account(account* account) {
        if (account_count >= MAX_ACCOUNTS)
            return;
        accounts[account_count++] = account;
    }

  
    void display_all_accounts() const {
        for (int i = 0; i < account_count; ++i) {
            accounts[i]->display();
        }
    }

   
    void save_accounts_to_file(const std::string& filename) const {
        FILE* file = fopen(filename.c_str(), "w");
        if (!file)
            exit(0);

        for (int i = 0; i < account_count; i++) {
            accounts[i]->save_to_file(file);
        }

        fclose(file);
    }

    void load_accounts_from_file(const std::string& filename) {
        FILE* file = fopen(filename.c_str(), "r");
        if (!file)
            exit(0);
        while (!feof(file)) {
            account account = account::load_from_file(file);
            add_account(new account(account));
        }

        fclose(file);
    }
};

int main() {
   
    currency usd("USD", 1.0);   
    currency eur("EUR", 1.1);   
    currency mad("MAD", 0.1);   

    
    account_manager manager;

   
    account* account1 = new account(1, "Ali");
    account* account2 = new account(2, "Jamal");

    
    account1->add_currency(usd);
    account1->add_currency(eur);
    account2->add_currency(usd);
    account2->add_currency(mad);

   
    manager.add_account(account1);
    manager.add_account(account2);

   
    account1->deposit(1000, "USD");
    account1->withdraw(100, "EUR");
    account1->transfer(*account2, 200, "USD", "MAD");

    
    manager.display_all_accounts();

   
    manager.save_accounts_to_file("accounts.txt");

    
    account_manager new_manager;
    new_manager.load_accounts_from_file("accounts.txt");

   
    new_manager.display_all_accounts();

    return 0;
}

//compiled with CDT v1.6.2

#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/time.hpp>
#include <eosio/singleton.hpp>

#include <string>

namespace eosiosystem {
   class system_contract;
}

namespace eosio {

   using std::string;

   
   class [[eosio::contract("cpustakeceos")]] token : public contract {
      public:
         using contract::contract;

         
         [[eosio::action]]
         void create( const name&   issuer,
                      const asset&  maximum_supply);
         
         [[eosio::action]]
         void issue( const name& to, const asset& quantity, const string& memo );

         
         [[eosio::action]]
         void retire( const asset& quantity, const string& memo );

         
         [[eosio::action]]
         void transfer( const name&    from,
                        const name&    to,
                        const asset&   quantity,
                        const string&  memo );
         
         [[eosio::action]]
         void open( const name& owner, const symbol& symbol, const name& ram_payer );

        
         [[eosio::action]]
         void close( const name& owner, const symbol& symbol );

         [[eosio::action]]
         void claim( const name& user);


         [[eosio::on_notify("eosiotoken::transfer")]]
         void deposit(name from, name to, eosio::asset quantity, std::string memo);

         [[eosio::on_notify("cpumintofeos::transfer")]]
         void reward(name from, name to, eosio::asset quantity, std::string memo);

         static asset get_supply( const name& token_contract_account, const symbol_code& sym_code )
         {
            stats statstable( token_contract_account, sym_code.raw() );
            const auto& st = statstable.get( sym_code.raw() );
            return st.supply;
         }

         static asset get_balance( const name& token_contract_account, const name& owner, const symbol_code& sym_code )
         {
            accounts accountstable( token_contract_account, owner.value );
            const auto& ac = accountstable.get( sym_code.raw() );
            return ac.balance;
         }


         asset get_rex_balance() {
         //gets rex balance from eosio table of contract
               rex_balance_table rex_balance("eosio"_n, get_self().value);
               auto rex_it = rex_balance.find(get_self().value);
               check(rex_it != rex_balance.end(), "User does not have a REX balance");
               return rex_it->rex_balance;
         }

         void add_deposit(name user, asset deposit){
         //save initial rex amount of user into table
            deposit_table deposits(_self, _self.value);
            auto itr = deposits.find(user.value);

            if(itr == deposits.end()) {
               deposits.emplace(_self, [&](auto& dep) {
                  dep.account = user;
                  dep.deposit = deposit;
                  dep.deposittime = current_time_point();  // Set to current time
                  dep.claimable = asset(0, symbol("ECPU", 8));

               });
            }       
            else {
               deposits.modify(itr, _self, [&](auto& dep) {
                  dep.deposit += deposit;
                  dep.deposittime = current_time_point();  // Set to current time
               });
            }
         }

  // Helper function to get the initial EOS deposit of a user
         asset get_deposit(name user) {
            
            deposit_table deposits(_self, _self.value);
            auto itr = deposits.find(user.value);
            check(itr != deposits.end(), "No deposit found for the user");
            return itr->deposit;
         }

  // Helper function to get the deposit time of a user's deposit
         
         
         uint64_t get_deposit_time(name user) {
            
            deposit_table deposits(_self, _self.value);
            auto itr = deposits.find(user.value);
            check(itr != deposits.end(), "No deposit found for the user");
            return itr->deposittime.utc_seconds;
         }

         void distribute_reward(asset ecpu_in) {

            deposit_table deposits(_self, _self.value);

            // Calculate the total EOS deposited
            asset total_eos_deposited = asset(0, symbol("EOS", 4));
            for (auto& deposit : deposits) {
               total_eos_deposited += deposit.deposit;
            }

            check(total_eos_deposited.amount > 0, "No EOS deposits found");

            // Distribute the EPCU reward based on each user's deposit proportion
            for (auto& deposit : deposits) {
               double user_share = (double)deposit.deposit.amount / (double)total_eos_deposited.amount;
               asset user_reward = asset(user_share * ecpu_in.amount, ecpu_in.symbol);

            // Update the claimable asset for each user
               deposits.modify(deposit, _self, [&](auto& record) {
                  record.claimable += user_reward;
               });
            }
         }

         asset get_rex_for_eos(asset eos_amount) {

         //intent is to, when user withdraws CEOS, to calculate the needed REX to be sold to give back to the user           
            check(eos_amount.symbol == symbol("EOS", 4), "Only EOS amounts are accepted");

            rex_pool_s rex_pool_table("eosio"_n, "eosio"_n.value);
            auto rex_pool = rex_pool_table.get();

            // Calculate the current rate of REX
            double current_rex_rate = static_cast<double>(rex_pool.total_rex.amount) / rex_pool.total_lendable.amount;

            // Calculate the amount of REX that corresponds to the given EOS amount
            int64_t rex_amount = static_cast<int64_t>(eos_amount.amount * current_rex_rate);

            return asset(rex_amount, symbol("REX", 4));
         }

         asset get_eos_for_rex(asset rex_amount) {
            check(rex_amount.symbol == symbol("REX", 4), "Only REX amounts are accepted");

            rex_pool_s rex_pool_table("eosio"_n, "eosio"_n.value);
            auto rex_pool = rex_pool_table.get();

            // Calculate the current rate of REX (EOS per REX)
            double current_rex_rate = static_cast<double>(rex_pool.total_rex.amount) / rex_pool.total_lendable.amount;

            // Calculate the amount of EOS that corresponds to the given REX amount
            // The EOS amount is the REX amount divided by the REX rate
            int64_t eos_amount = static_cast<int64_t>(rex_amount.amount / current_rex_rate);

            return asset(eos_amount, symbol("EOS", 4));
         }

         void check_and_send_excess_eos() {
            require_auth(_self);

            // Step 1: Check the contract's own REX balance
            rex_balance_table rex_balance("eosio"_n, _self.value);
            auto rex_it = rex_balance.find(_self.value);
            check(rex_it != rex_balance.end(), "Contract does not have a REX balance");

            // Step 2: Get the equivalent EOS value of the REX
            asset eos_equivalent = get_eos_for_rex(rex_it->rex_balance);

            // Step 3: Get the total supply of CEOS tokens
            stats statstable("ceostoken"_n, symbol_code("CEOS").raw()); // Replace 'ceostoken' with your token contract
            auto existing = statstable.find(symbol_code("CEOS").raw());
            check(existing != statstable.end(), "CEOS token does not exist");
            asset ceos_supply = existing->supply;

            // Step 4: Calculate the excess EOS and send to ecpulpholder
            if (eos_equivalent > ceos_supply) {
            
               action(permission_level{_self, "active"_n}, name{"eosio.token"}, "transfer"_n, 
               std::make_tuple(get_self(), name{"ecpulpholder"}, (ceos_supply-eos_equivalent), string("deposit"))).send();
            
            }

         }

         void place_rex_order(name user, asset eos_amount) {
         // Ensure the asset is REX
            check(eos_amount.symbol == symbol("EOS", 4), "Must specify expected EOS amount");

            rex_order_table orders(get_self(), get_self().value);

            // Clear existing order (if any)
            auto order_itr = orders.begin();
            check(order_itr == orders.end(), "An existing order is already placed");

            // Place new order
            orders.emplace(get_self(), [&](auto& order) {
               order.username = user;
               order.eos_amount = eos_amount;
            });
         }

         void delete_rex_order() {
            rex_order_table orders(get_self(), get_self().value);

            // Check if there's an order to delete
            auto order_itr = orders.begin();
            if (order_itr != orders.end()) {
               // Delete the order
               orders.erase(order_itr);
            }
         }

         name get_rex_order_user() {
            rex_order_table orders(get_self(), get_self().value);
            auto order_itr = orders.begin();

            check(order_itr != orders.end(), "No REX orders found");
            return order_itr->username;
         }

         asset get_rex_order_amount() {
            rex_order_table orders(get_self(), get_self().value);
            auto order_itr = orders.begin();

            check(order_itr != orders.end(), "No REX orders found");
            return order_itr->eos_amount;
         }

         asset get_claimable(name user) {
            deposit_table deposits(get_self(), get_self().value);
            auto itr = deposits.find(user.value);
            check(itr != deposits.end(), "User record not found");

            return itr->claimable;
         }

         void remove_claimable(name user) {
            deposit_table deposits(get_self(), get_self().value);
            auto itr = deposits.find(user.value);
            check(itr != deposits.end(), "User record not found");

            deposits.modify(itr, get_self(), [&](auto& record) {
               check(record.deposit.amount >= record.claimable.amount, "Insufficient deposit to remove claimable amount");
               record.deposit -= record.claimable;
               record.claimable = asset(0, record.claimable.symbol);
            });
         }

         bool is_five_days_passed(name user) {
           
            deposit_table deposits(get_self(), get_self().value);
            auto deposit_itr = deposits.find(user.value);
            check(deposit_itr != deposits.end(), "User record not found");

            // Current time
            auto current_time = current_time_point();

            // Time five days ago
            auto five_days_ago = current_time - eosio::days(5);

            // Check if the deposittime is earlier than five days ago
            return deposit_itr->deposittime < five_days_ago;
         }

         using create_action = eosio::action_wrapper<"create"_n, &token::create>;
         using issue_action = eosio::action_wrapper<"issue"_n, &token::issue>;
         using retire_action = eosio::action_wrapper<"retire"_n, &token::retire>;
         using transfer_action = eosio::action_wrapper<"transfer"_n, &token::transfer>;
         using open_action = eosio::action_wrapper<"open"_n, &token::open>;
         using close_action = eosio::action_wrapper<"close"_n, &token::close>;
      private:

         struct [[eosio::table]] user_deposit {
            name   account;       // Account name of the user
            asset  deposit;    // The amount of EOS deposited
            time_point_sec deposittime; // The time when the deposit was made

            asset claimable; //ECPU amount claimable

            // Primary key for the table to be indexed by user account
            uint64_t primary_key() const { return account.value; }

            // Secondary index by deposit time
            uint64_t by_deposit_time() const { return deposittime.utc_seconds; }
         };

         
         struct [[eosio::table]] account {
            asset    balance;

            uint64_t primary_key()const { return balance.symbol.code().raw(); }
         };

         struct [[eosio::table]] currency_stats {
            asset    supply;
            asset    max_supply;
            name     issuer;

            uint64_t primary_key()const { return supply.symbol.code().raw(); }
         };

         // The rex_pool struct mirrors the struct found in the EOSIO system contract for REX
         struct [[eosio::table, eosio::contract("eosio")]] rex_pool {
            asset    total_lendable;   // Total EOS lent to REX
            asset    total_rex;        // Total REX
         };

         struct [[eosio::table, eosio::contract("eosio")]] rex_balance {
            asset    vote_stake;    // EOS staked for voting
            asset    rex_balance;   // REX balance

            uint64_t primary_key()const { return vote_stake.symbol.code().raw(); }
         };

         // Table to store REX sell orders
         struct [[eosio::table]] rex_order {
            name     username;    // Account name of the user
            asset    eos_amount;  // Epected EOS for amount of REX to be sold

            // Primary key for the table to be indexed by username
            uint64_t primary_key() const { return username.value; }
         };

         typedef eosio::multi_index< "accounts"_n, account > accounts;
         typedef eosio::multi_index< "stat"_n, currency_stats > stats;

         typedef eosio::multi_index<"deposits"_n, user_deposit,
         indexed_by<"bytime"_n, const_mem_fun<user_deposit, uint64_t, &user_deposit::by_deposit_time>>
         > deposit_table;
         
         typedef eosio::singleton<"rexpool"_n, rex_pool> rex_pool_s;

         typedef eosio::multi_index<"rexbalance"_n, rex_balance> rex_balance_table;

          typedef eosio::multi_index<"rexorders"_n, rex_order> rex_order_table;

         void sub_balance( const name& owner, const asset& value );
         void add_balance( const name& owner, const asset& value, const name& ram_payer );
   };

}
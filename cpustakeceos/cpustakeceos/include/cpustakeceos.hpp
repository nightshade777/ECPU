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

         void set_initial_rex(name user, asset rexbal){
         //save initial rex amount of user into table
            deposit_table deposits(_self, _self.value);
            auto itr = deposits.find(user.value);

            if(itr == deposits.end()) {
               deposits.emplace(_self, [&](auto& dep) {
                  dep.account = user;
                  dep.initialrex = rexbal;
                  // Initialize other fields as necessary, e.g., eosdeposit, deposittime
               });
            }       
            else {
               deposits.modify(itr, _self, [&](auto& dep) {
                  dep.initialrex = rexbal;
               });
            }
         }

         void modify_rex(name user, asset rexbal, bool add) {
            deposit_table deposits(_self, _self.value);
            auto itr = deposits.find(user.value);

            if(itr == deposits.end()) {
               // If entry does not exist, use set_initial_rex
               set_initial_rex(user, rexbal);
            }       
            else {
               // Modify existing entry
               deposits.modify(itr, _self, [&](auto& dep) {
                  if(add) {
                  dep.initialrex += rexbal;
                  } 
                  else {
                  dep.initialrex -= rexbal;
                  }
               });
            }
         }

  // Helper function to get the initial amount of REX corresponding to a user's deposit
         asset get_stored_rex(name user) {
         //gets rex from deposits from internal table
            deposit_table deposits(_self, _self.value);
            auto itr = deposits.find(user.value);
            check(itr != deposits.end(), "No deposit found for the user");
            return itr->initialrex;
         }

  // Helper function to get the initial EOS deposit of a user
         asset get_initial_deposit(name user) {
            
            deposit_table deposits(_self, _self.value);
            auto itr = deposits.find(user.value);
            check(itr != deposits.end(), "No deposit found for the user");
            return itr->eosdeposit;
         }

  // Helper function to get the deposit time of a user's deposit
         
         void set_deposit_time(name user, uint64_t deposit_time){
            
            deposit_table deposits(_self, _self.value);
            auto itr = deposits.find(user.value);

            if(itr == deposits.end()) {
               deposits.emplace(_self, [&](auto& dep) {
                  dep.account = user;
                  dep.deposittime = time_point_sec(deposit_time);
            // Initialize other fields as necessary
               });
            } 
            else {
               deposits.modify(itr, _self, [&](auto& dep) {
                  dep.deposittime = time_point_sec(deposit_time);
               });
            }
         }
         
         uint64_t get_deposit_time(name user) {
            
            deposit_table deposits(_self, _self.value);
            auto itr = deposits.find(user.value);
            check(itr != deposits.end(), "No deposit found for the user");
            return itr->deposittime.utc_seconds;
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
            asset  eosdeposit;    // The amount of EOS deposited
            asset  initialrex;    // The initial amount of REX corresponding to the EOS deposit
            time_point_sec deposittime; // The time when the deposit was made

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

         typedef eosio::multi_index< "accounts"_n, account > accounts;
         typedef eosio::multi_index< "stat"_n, currency_stats > stats;

         typedef eosio::multi_index<"deposits"_n, user_deposit,
         indexed_by<"bytime"_n, const_mem_fun<user_deposit, uint64_t, &user_deposit::by_deposit_time>>
         > deposit_table;
         
         typedef eosio::singleton<"rexpool"_n, rex_pool> rex_pool_s;

         typedef eosio::multi_index<"rexbalance"_n, rex_balance> rex_balance_table;

         void sub_balance( const name& owner, const asset& value );
         void add_balance( const name& owner, const asset& value, const name& ram_payer );
   };

}
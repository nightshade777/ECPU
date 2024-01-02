//compiled with CDT v1.6.2

#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>

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

         asset get_rex_delta(){

         //convert or draw from system table, perhaps use ramtoken style...
         //perhaps no calculations needed and can find exact rex issuance?

         }
         asset get_initial_rex(){

         }

         asset get_initial_deposit(name user){

         }

         uint64_t get_deposit_time(name user){

         }



         using create_action = eosio::action_wrapper<"create"_n, &token::create>;
         using issue_action = eosio::action_wrapper<"issue"_n, &token::issue>;
         using retire_action = eosio::action_wrapper<"retire"_n, &token::retire>;
         using transfer_action = eosio::action_wrapper<"transfer"_n, &token::transfer>;
         using open_action = eosio::action_wrapper<"open"_n, &token::open>;
         using close_action = eosio::action_wrapper<"close"_n, &token::close>;
      private:
         
         //table for user desposits,
         //asset eosdeposit
         //asset initialrex
         //uint deposittime
         
         
         
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

         typedef eosio::multi_index< "accounts"_n, account > accounts;
         typedef eosio::multi_index< "stat"_n, currency_stats > stats;

         void sub_balance( const name& owner, const asset& value );
         void add_balance( const name& owner, const asset& value, const name& ram_payer );
   };

}
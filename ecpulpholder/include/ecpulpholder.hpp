//compiled with CDT v1.6.2

#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>

#include <string>

namespace eosiosystem {
   class system_contract;
}

namespace eosio {

   using std::string;

   
   class [[eosio::contract("ecpulpholder")]] token : public contract {
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
         void instapowerup( const name& contract, name receiver, asset powerupamt );

         [[eosio::action]]
         void setproxy( const name& contract, name proxy, name sender);

         [[eosio::action]]
         void setpool(asset resevoir, asset rex_queue, asset eospool);

         [[eosio::on_notify("eosio.token::transfer")]]
         void deposit(name from, name to, eosio::asset quantity, std::string memo);

         [[eosio::on_notify("cpumintofeos::stake")]] 
         void setstake(name account, asset value, bool selfdelegate);

         [[eosio::on_notify("cpumintofeos::delegate")]] 
         void setdelegate(name account, asset receiver, asset value);

         void set_resevoir( const asset& quantity){ //update resevoir in stat table

                  auto sym = asset(0, symbol("ECPU", 4)).symbol;
                  stats statstable( get_self(), sym.code().raw() );
                  auto existing = statstable.find( sym.code().raw() );
                  check( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
                  const auto& st = *existing;
                  require_auth( st.issuer );
                  check( quantity.is_valid(), "invalid quantity" );
                  check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
                  statstable.modify( st, same_payer, [&]( auto& s ) {
                        s.resevoir = quantity;
                  });
                  
         }

         name get_proxy(name contract)
         {// get name of proxy from table to vote for
                   proxies to_proxy( get_self(), get_self().value );
                   const auto& to = to_proxy.get( contract.value );
                   return to.proxy;

         }
         name get_eossender(name contract)
         {//get name of contract that sends voting rewards
                   proxies to_proxy( get_self(), get_self().value );
                   const auto& to = to_proxy.get( contract.value );
                   return to.eossender;
         }

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

         static asset get_ecpustake(const name& token_contract_account, const symbol_code& sym_code) // build struct from cpumintofeos account and pull total ecpu staked from table
         {
            stats statstable( token_contract_account, sym_code.raw() );
            const auto& st = statstable.get( sym_code.raw() );
            return st.totalstake;
         }

         asset get_resevoir( name contract )
         {
            poolstats pstatstable( get_self(), get_self().value );
            const auto& to = pstatstable.get( contract.value );
            return to.resevoir;
         }
         asset get_rex_queue( name contract )
         {
            poolstats pstatstable( get_self(), get_self().value );
            const auto& to = pstatstable.get( contract.value );
            return to.rex_queue;
         }
         int get_last_deposit( name contract )
         {
            poolstats pstatstable( get_self(), get_self().value );
            const auto& to = pstatstable.get( contract.value );
            return to.lastdeposit;
         }
         asset get_eos_pool( name contract )
         {
            poolstats pstatstable( get_self(), get_self().value );
            const auto& to = pstatstable.get( contract.value );
            return to.eospool;
         }

         using create_action = eosio::action_wrapper<"create"_n, &token::create>;
         using issue_action = eosio::action_wrapper<"issue"_n, &token::issue>;
         using retire_action = eosio::action_wrapper<"retire"_n, &token::retire>;
         using transfer_action = eosio::action_wrapper<"transfer"_n, &token::transfer>;
         using open_action = eosio::action_wrapper<"open"_n, &token::open>;
         using close_action = eosio::action_wrapper<"close"_n, &token::close>;
      
      private:
         struct [[eosio::table]] account {
            asset    balance;

            uint64_t primary_key()const { return balance.symbol.code().raw(); }
         };

         struct [[eosio::table]] currency_stats {
           
            asset    supply;
            asset    max_supply;
            name     issuer;
            int      prevmine;// var not used natively, utilized in order to pull from two different token contracts without requiring two structs
            int      creationtime;//see above
            asset    totalstake;//see above
            asset    resevoir;//eos from voter rewards reserved for unstakers for 24 hours, need to stay liquid in case unstaked ECPU becomes staked and delegated
            asset    rex_queue;//eos from mining which will be queued to enter rex every hour
            int      last_deposit;//last deposit of rex queue 
            uint64_t primary_key()const { return supply.symbol.code().raw(); }
            
         };

            struct [[eosio::table]] pool_stats {
           
            name     contract;
            asset    resevoir;//eos from voter rewards reserved for unstakers for 24 hours, need to stay liquid in case unstaked ECPU becomes staked and delegated
            asset    rex_queue;//eos from mining which will be queued to enter rex every hour
            int      lastdeposit;//last deposit of above mining income rex queue 
            asset    eospool; //total initial eos entered into rex


            auto primary_key() const {return contract.value;}
            
         };


         struct [[eosio::table]] proxy {
           name contract;
           name proxy;
           name eossender;
           
           auto primary_key() const {return contract.value;}
         };

         typedef eosio::multi_index< "accounts"_n, account > accounts;
         typedef eosio::multi_index< "stat"_n, currency_stats > stats;
         typedef eosio::multi_index< "proxytable"_n, proxy > proxies;
         typedef eosio::multi_index< "poolstat"_n, pool_stats > poolstats;

         void sub_balance( const name& owner, const asset& value );
         void add_balance( const name& owner, const asset& value, const name& ram_payer );
   };

}
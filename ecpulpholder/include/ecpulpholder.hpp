//compiled with CDT v1.6.2

#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/system.hpp>

using namespace eosio;

   using std::string;

   
   CONTRACT ecpulpholder: public contract {
      
      public:
         using contract::contract;

         [[eosio::action]]
         void setproxy(name proxy, name sender);

         [[eosio::action]]
         void setpool(asset resevoir, asset eospool, asset lastpay);

         [[eosio::on_notify("eosio.token::transfer")]]
         void deposit(name from, name to, eosio::asset quantity, std::string memo);

         [[eosio::on_notify("cpumintofeos::delegate")]] 
         void setdelegate(name account, asset receiver, asset value);


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

         asset get_resevoir()
         {
            poolstats pstatstable( get_self(), get_self().value );
            const auto& to = pstatstable.get( get_self().value );
            return to.resevoir;
         }

         int get_last_rexqueue()
         {
            poolstats pstatstable( get_self(), get_self().value );
            const auto& to = pstatstable.get( get_self().value );
            return to.lastdeposit;
         }

         asset get_eos_pool()
         {
            poolstats pstatstable( get_self(), get_self().value );
            const auto& to = pstatstable.get( get_self().value );
            return to.eospool;
         }

          asset get_last_daily_pay()
         {
            poolstats pstatstable( get_self(), get_self().value );
            const auto& to = pstatstable.get( get_self().value );
            return to.lastdailypay;
         }

         void add_resevoir(asset input){

             poolstats pstatstable( get_self(), get_self().value );
             auto to = pstatstable.find( get_self().value );

             pstatstable.modify( to, get_self(), [&]( auto& a ) {
       
               a.resevoir = a.resevoir + input;
               
            });

         }

         void add_to_eospool(asset input){

             poolstats pstatstable( get_self(), get_self().value );
             auto to = pstatstable.find( get_self().value );

             pstatstable.modify( to, get_self(), [&]( auto& a ) {
       
               a.eospool = a.eospool + input;
            });

         }

         void clear_resevoir(){

             poolstats pstatstable( get_self(), get_self().value );
             auto to = pstatstable.find( get_self().value );

             pstatstable.modify( to, get_self(), [&]( auto& a ) {
       
               a.resevoir = asset(0, symbol("EOS", 4));
               
            });

         }

         void set_last_daily_pay(asset input){

             poolstats pstatstable( get_self(), get_self().value );
             auto to = pstatstable.find( get_self().value );

             pstatstable.modify( to, get_self(), [&]( auto& a ) {
       
               a.lastdailypay = input;
               
            });

         }
      private:
         struct [[eosio::table]] account {
            asset    balance;
            uint64_t primary_key()const { return balance.symbol.code().raw(); }
         };

         struct [[eosio::table]] currency_stats {
           
            asset    supply;
            asset    max_supply;
            name     issuer;
            uint32_t  prevmine;
            uint32_t  creationtime;
            uint32_t  lastdeposit;//time of last deposit of above mining income rex queue 
            asset    totalstake;
            asset    totaldelegate;
            
            uint64_t primary_key()const { return supply.symbol.code().raw(); }
         };

         struct [[eosio::table]] pool_stats {
           
            name     contract;
            asset    resevoir;//eos from voter rewards reserved for unstakers for 24 hours, need to stay liquid in case unstaked ECPU becomes staked and delegated
            int      lastdeposit;//time of last deposit of above mining income rex queue 
            asset    eospool; //total initial eos entered into rex

            asset    lastdailypay;//amount sent to contract daily


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

         
   };


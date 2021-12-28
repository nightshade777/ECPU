#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/system.hpp>

#include <string>
#include <math.h>


namespace eosiosystem {
   class system_contract;
}

namespace eosio {

   using std::string;

   
   class [[eosio::contract("cpumintofeos")]] token : public contract {
      public:
         using contract::contract;


         
         
         //standard token contract actions -----------------------------------------------
         
         [[eosio::action]]
         void create( const name&   issuer,
                      const asset&  maximum_supply);
        
         [[eosio::action]]
         void issue( const name& to, const asset& quantity, const string& memo );

        
         [[eosio::action]]
         void transfer( const name&    from,
                        const name&    to,
                        const asset&   quantity,
                        const string&  memo );
        
         [[eosio::action]]
         void open( const name& owner, const symbol& symbol, const name& ram_payer );


         [[eosio::action]]
         void open2(name user);

         [[eosio::action]]
         void open3(name user, name recipient);

         
         [[eosio::action]]
         void close( const name& owner, const symbol& symbol );
          
         
         [[eosio::action]]
         void stake (name account, asset value, bool selfdelegate);
         
         [[eosio::action]]
         void unstake (name account, asset value, bool selfdelegate);

         [[eosio::action]]
         void delegate (name account, name receiver, asset value);
         
         
         [[eosio::action]]
         void undelegate (name account, name receiver, asset value);


         [[eosio::action]]
         void retire( const asset& quantity, const string& memo );


         [[eosio::action]] 
         void destroytoken(asset token);


         [[eosio::action]] 
         void destroyacc(std::string symbol, name account);

         [[eosio::action]] 
         void minereceipt( name user);

         // mining------------------------------------------------------
         
        [[eosio::on_notify("eosio.token::transfer")]]
         void claim(name from, name to, eosio::asset quantity, std::string memo);
         
         
         static asset get_supply( const name& token_contract_account, const symbol_code& sym_code )
         {
            stats statstable( token_contract_account, sym_code.raw() );
            const auto& st = statstable.get( sym_code.raw() );
            return st.supply;
         }

         uint32_t get_prevmine( const name& token_contract_account, const symbol_code& sym_code )
         {
            stats statstable( token_contract_account, sym_code.raw() );
            const auto& st = statstable.get( sym_code.raw() );
            return st.prevmine;
         }

         uint32_t get_ctime( const name& token_contract_account, const symbol_code& sym_code )
         {
            stats statstable( token_contract_account, sym_code.raw() );
            const auto& st = statstable.get( sym_code.raw() );
            return st.creationtime;
         }
         
         static asset get_balance( const name& token_contract_account, const name& owner, const symbol_code& sym_code )
         {
            accounts accountstable( token_contract_account, owner.value );
            const auto& ac = accountstable.get( sym_code.raw() );
            return ac.balance;
         }

         static asset get_balance_eos( const name& token_contract_account, const name& owner, const symbol_code& sym_code )
         {
            accountseos accountstable( token_contract_account, owner.value );
            const auto& ac = accountstable.get( sym_code.raw() );
            return ac.balance;
         }
         
         static asset get_stored_balance( const name& token_contract_account, const name& owner, const symbol_code& sym_code )
         {
            accounts accountstable( token_contract_account, owner.value );
            const auto& ac = accountstable.get( sym_code.raw() );
            return ac.storebalance;
         }
         
         static asset get_cpupower_balance( const name& token_contract_account, const name& owner, const symbol_code& sym_code )
         {
            accounts accountstable( token_contract_account, owner.value );
            const auto& ac = accountstable.get( sym_code.raw() );
            return ac.cpupower;
         }

         
         int get_stake_fraction(name user){

            asset supply  = get_supply(get_self(), symbol_code("CPU"));
            asset cpu_staked = get_cpupower_balance(get_self(), user, symbol_code("CPU"));

            int stakefraction = ((cpu_staked.amount * 1000000) / supply.amount);

            return stakefraction;

         }

         bool check_cpu_balance(name user ){
            
            
            asset assetname = asset(0, symbol("CPU", 4));

            auto sym_code = assetname.symbol.code();

            accounts accountstable( name{"cpumintofeos"}, user.value );
            
            auto existing = accountstable.find(sym_code.raw());
            
            if (existing == accountstable.end()){

                return false;

            }
            
            return true;
         }
              

    void updatestake(asset quantity){

        auto sym = quantity.symbol;
        stats statstable( get_self(), sym.code().raw() );
        auto existing = statstable.find( sym.code().raw() );
        const auto& st = *existing;
        statstable.modify( st, same_payer, [&]( auto& s ) {
            s.totalstake += quantity;
        });
}

   void updatedelegate(asset quantity){

        auto sym = quantity.symbol;
        stats statstable( get_self(), sym.code().raw() );
        auto existing = statstable.find( sym.code().raw() );
        const auto& st = *existing;
        statstable.modify( st, same_payer, [&]( auto& s ) {
            s.totaldelegate += quantity;
        });
}

    uint32_t get_last_deposit()
         {
            asset quantity = asset(0, symbol("ECPU", 4));
            auto sym = quantity.symbol;
      
            stats statstable( get_self(), sym.code().raw() );
            const auto& to = statstable.get( sym.code().raw() );
            return to.lastdeposit;
            
    
    }

      void mine(name from){


        uint32_t elapsedpm = current_time_point().sec_since_epoch() - get_prevmine(get_self(), symbol_code("ECPU"));
        
        asset balance = get_balance(get_self(),get_self(), symbol_code("ECPU"));

    
        if (elapsedpm > 180){ //if 3 minutes have passed since last mining event, issue a 1 new mining reward, dont issue for dead spaces
      
            int rewardcount =  elapsedpm / 180;
            asset issue =  rewardcount * asset(10000, symbol("CPU", 4));

            action(permission_level{_self, "active"_n}, "cpumintofeos"_n, "issue"_n, 
            std::make_tuple(get_self(), issue, std::string("issue new ECPU rewards"))).send();

            balance = balance + issue;

            stats statstable( get_self(), symbol_code("ECPU").raw() );
            auto existing = statstable.find( symbol_code("ECPU").raw() );
            check( existing != statstable.end(), "token with symbol does not exist" );
            const auto& st = *existing;

            statstable.modify( st, same_payer, [&]( auto& s ) {
                s.prevmine = current_time_point().sec_since_epoch();
            });

          }
            std::string minemessage = "ECPU mining Reward";

            balance = balance/10000;
      
        if (balance.amount > 0){
            action(permission_level{_self, "active"_n}, "cpumintofeos"_n, "transfer"_n, 
            std::make_tuple(get_self(), from, balance, minemessage)).send();
        }

}
         using create_action = eosio::action_wrapper<"create"_n, &token::create>;
         using issue_action = eosio::action_wrapper<"issue"_n, &token::issue>;
         using retire_action = eosio::action_wrapper<"retire"_n, &token::retire>;
         using transfer_action = eosio::action_wrapper<"transfer"_n, &token::transfer>;
         using open_action = eosio::action_wrapper<"open"_n, &token::open>;
         using close_action = eosio::action_wrapper<"close"_n, &token::close>;
      
         struct [[eosio::table]] account {
            asset    balance;
            asset    storebalance; //transfer blocker var, total ecpu locked
            asset    cpupower; //ecpu staked to bal (includes staked from others)
            asset    unstaking;
            uint32_t unstake_time;

            uint64_t primary_key()const { return balance.symbol.code().raw(); }
         };

         struct [[eosio::table]] accounteos {
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


         struct [[eosio::table]] delegatecpu {
            
            name     recipient;
            asset    cpupower;
            uint32_t  delegatetime;
            

            uint64_t primary_key()const { return recipient.value; } 
         };


         typedef eosio::multi_index< "accounts"_n, account > accounts;
         typedef eosio::multi_index< "accounts"_n, accounteos > accountseos;
         typedef eosio::multi_index< "delegates"_n, delegatecpu > delegates;
         typedef eosio::multi_index< "stat"_n, currency_stats > stats;
        


         void sub_balance( const name& owner, const asset& value );
         void add_balance( const name& owner, const asset& value, const name& ram_payer );
   };
   
} /// namespace eosio 



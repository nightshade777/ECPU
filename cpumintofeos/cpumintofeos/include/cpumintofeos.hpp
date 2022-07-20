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
         void delegate (name account, name receiver, asset value);
         
         
         [[eosio::action]]
         void undelegate (name account, name receiver, asset value);


         [[eosio::action]]
         void retire( const asset& quantity, const string& memo );


         [[eosio::action]] 
         void destroytoken(asset token);


         [[eosio::action]] 
         void destroyacc(asset token, name account, name delegaterow);

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
         
         /**static asset get_delpower_balance( const name& token_contract_account, const name& owner, const symbol_code& sym_code )
         {
            accounts accountstable( token_contract_account, owner.value );
            const auto& ac = accountstable.get( sym_code.raw() );
            return ac.delegatepwr;
         }**/
         
         static asset get_cpupower_balance( const name& token_contract_account, const name& owner, const symbol_code& sym_code )
         {
            accounts accountstable( token_contract_account, owner.value );
            const auto& ac = accountstable.get( sym_code.raw() );
            return ac.cpupower;
         }

         bool check_cpu_balance(name user ){
            
            
            asset assetname = asset(0, symbol("ECPU", 8));

            auto sym_code = assetname.symbol.code();

            accounts accountstable( name{"cpumintofeos"}, user.value );
            
            auto existing = accountstable.find(sym_code.raw());
            
            if (existing == accountstable.end()){

                return false;

            }
            
            return true;
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

    uint32_t get_last_deposit(){
            
            asset quantity = asset(0, symbol("ECPU", 8));
            auto sym = quantity.symbol;
       
            stats statstable( get_self(), sym.code().raw() );
           
            const auto& to = statstable.get( sym.code().raw() );
             
            return to.lastdeposit;
            
    
    }

      void mine(name from){


        uint32_t elapsedpm = current_time_point().sec_since_epoch() - get_prevmine(get_self(), symbol_code("ECPU"));
        
        asset balance = get_balance(get_self(),get_self(), symbol_code("ECPU"));

    
        if (elapsedpm > 180){ //if 3 minutes have passed since last mining event, issue a 1 new mining reward, round down to whole number rewards
      
            int rewardcount =  elapsedpm / 180;
            asset issue =  rewardcount * asset(100000000, symbol("ECPU", 8));

            action(permission_level{_self, "active"_n}, "cpumintofeos"_n, "issue"_n, 
            std::make_tuple(get_self(), issue, std::string("Issue ECPU Inflation"))).send();

            balance = balance + issue;

            stats statstable( get_self(), symbol_code("ECPU").raw() );
            auto existing = statstable.find( symbol_code("ECPU").raw() );
            const auto& st = *existing;

            statstable.modify( st, same_payer, [&]( auto& s ) {
                s.prevmine = current_time_point().sec_since_epoch();
            });

          }
            std::string minemessage = "ECPU Mining Rewards";

            balance = balance/10000;
      
        if (balance.amount > 0){
            action(permission_level{_self, "active"_n}, "cpumintofeos"_n, "transfer"_n, 
            std::make_tuple(get_self(), from, balance, minemessage)).send();
        }

}

        
        
        
        
        
        
        asset get_non_liquid_ecpu(name account){
        
         
           // if delgatee's undelegating ECPU != 0, then check if 24 hours have passed
           //if 24 hours have passed, modify entry to zero
           //if 24 hours have not passed, add to illiquid balance
          
           update_delegating(account); //checks if undelegating period has been passed and removes undelegate amounts which have been cleared
 
           asset non_liquid_ecpu =  asset(0, symbol("ECPU", 8)); //initialization

           accounts from_acnts( get_self(), account.value );
           auto to = from_acnts.find( non_liquid_ecpu.symbol.code().raw() );
           
           non_liquid_ecpu += (to->delegated); // add delegated CPU
           
           delegates delegatetable(get_self(),account.value);

           auto it = delegatetable.begin();

           if (it ==  delegatetable.end()){

               return asset(0, symbol("ECPU", 8));
           }
           for(it= delegatetable.begin(); it != delegatetable.end(); it++){

               if((it->undelegatingecpu).amount != 0){
               
                     non_liquid_ecpu = non_liquid_ecpu + (it->undelegatingecpu); //add ECPU still undelegating

               }

           }
           return non_liquid_ecpu;
        }

        void update_delegating(name account){//iterates through all delegates which this account has delegated to and 
                                             //updates any undelegating vars which have already cleared to zero
                                             //sums all cleared undelegating and removes from var in accounts table
                                             

   
           uint32_t time_now = current_time_point().sec_since_epoch();
           uint32_t one_day  = 60*60*24;

           asset undelegateclear = asset(0, symbol("ECPU", 8)); //sum of undelegting amount to be cleared

           

           delegates delegatetable(get_self(),account.value);

           auto it = delegatetable.begin();

           if (it ==  delegatetable.end()){

               return;
           }


           for(it = delegatetable.begin(); it != delegatetable.end(); it++){

               if((it->undelegatingecpu).amount != 0){
               
   
                     if((it->undelegatetime) < (time_now - one_day)){

                        delegatetable.modify(it, same_payer, [&]( auto& a ){
                              
                              undelegateclear += a.undelegatingecpu;
                              a.undelegatingecpu =  asset(0, symbol("ECPU", 8));

                        });
                     }

               }

           }
           accounts from_acnts( _self, account.value );
           auto to = from_acnts.find( undelegateclear.symbol.code().raw() );
   
           from_acnts.modify( to, same_payer, [&]( auto& a ) {
               a.undelegating -= undelegateclear;
           });

        }

         using create_action = eosio::action_wrapper<"create"_n, &token::create>;
         using issue_action = eosio::action_wrapper<"issue"_n, &token::issue>;
         using retire_action = eosio::action_wrapper<"retire"_n, &token::retire>;
         using transfer_action = eosio::action_wrapper<"transfer"_n, &token::transfer>;
         using open_action = eosio::action_wrapper<"open"_n, &token::open>;
         using close_action = eosio::action_wrapper<"close"_n, &token::close>;
      
         struct [[eosio::table]] account {
            asset    balance;
            asset    delegated;  //balance of tokens able to be delegated, max is the number of tokens staked, converted to cpupower upon delegation
                                   //(can be thought of as amount of staked tokens which have not been delegated yet)
            asset    undelegating; //ECPU in process of clearing undelegation
            asset    cpupower;     //ECPU tokens delegated to bal (includes delegated from others)
                                   //(can be thought of as the sum of all ECPU delegated to this accoount)
           
           

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
            uint32_t  prevmine;//time of most recent mining action
            uint32_t  creationtime; //time of token creation
            uint32_t  lastdeposit;//time of last deposit of above mining income rex queue 
            asset    totaldelegate; //total amount of ECPU delegated

            uint64_t primary_key()const { return supply.symbol.code().raw(); }
         };

         //scope is the account executing the delegate action, this table gives a list of all delegatees with respective d_ECPU from delegator
         struct [[eosio::table]] delegatecpu {
            
            name      recipient; //receiver of degated ECPU (cpupower)
            asset     cpupower; // amount of ECPU delegated
            uint32_t  delegatetime; // time of MOST RECENT delegation

            asset     undelegatingecpu;//ECPU being undelegated, will be non-zero even after clearing period until the delegator executes a transfer/delegate/undelegate action to refresh
            uint32_t  undelegatetime;// time of MOST RECENT delegation
            

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



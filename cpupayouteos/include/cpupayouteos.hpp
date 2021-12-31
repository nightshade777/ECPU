#pragma once
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/system.hpp>


using namespace eosio;



CONTRACT cpupayouteos : public contract {
  public:
    using contract::contract;

    //initialize system-----------------------------
    [[eosio::action]] 
    void initialize(name contract);

    [[eosio::action]]
    void resetround(name contract);

    [[eosio::action]] 
    void intdelegatee(name user);

    //listen to mining 
    [[eosio::on_notify("cpumintofeos::minereceipt")]] void cpupowerup(name miner);
    
    //listen to stake / unstake
    [[eosio::on_notify("cpumintofeos::delegate")]] void setdelegate(name account, name receiver, asset value);
    [[eosio::on_notify("cpumintofeos::undelegate")]] void setundelgate(name account, name receiver, asset value);
    
    
    TABLE delegatee { 
      
      name      delegatee;
      asset     ecpudelegated;
      uint32_t  delegatetime;
      uint32_t  lastpaytime;

      
      auto primary_key() const { return delegatee.value; }
    };
    typedef multi_index<name("delegatee"), delegatee> delegatees;



    TABLE delstakestat {
      name       contract;
      asset      totaldelstaked;
     
      
      auto primary_key() const { return contract.value; }
    };
    typedef multi_index<name("delstakestat"), delstakestat> delstake_stat;



    TABLE queuetable { 
      
      name       contract;
      name       currentpayee;
   uint32_t       lastpaytime;
      asset      remainingpay_ecpu; //the remaaining amount of eos corresponding to delegated ECPU to be paid ut during curren round
      asset      startpay_ecpu;//the amount of eos to be paid out at the start of the round
      asset      stakestart; //the amount of ecpu staked at start of the active round
    uint32_t      payoutstarttime; //the time of the start if the round
      
      
      
      auto primary_key() const { return contract.value; }

    };
    





     struct [[eosio::table]] account {
            
            asset    balance;
            asset    storebalance; //balance of staked tokens, this is a transfer blocker variable representing total ecpu locked
            asset    delegatepwr;  //balance of staked tokens able to be delegated, max is the number of tokens staked, converted to cpupower upon delegation
                                   //(can be thought of as amount of staked tokens which have not been delegated yet)
            asset    cpupower;     //ecpu staked to bal (includes staked from others)
                                   //(can be thought of as the sum of all ECPU delegated to this accoount)
            asset    unstaking;
            uint32_t unstake_time;
            

            uint64_t primary_key()const { return balance.symbol.code().raw(); }
         };
  
        

        
        struct [[eosio::table]] accountg {
            asset    balance;
            
            

            uint64_t primary_key()const { return balance.symbol.code().raw(); }
         };
  
        

        struct [[eosio::table]] currency_stats {
            asset    supply;
            asset    max_supply;
            name     issuer;
            uint32_t  prevmine;
            uint32_t  creationtime;
            uint32_t  lastdeposit;
            asset    totalstake;
            asset    totaldelegate;

            uint64_t primary_key()const { return supply.symbol.code().raw(); }
         };

         typedef eosio::multi_index<"queuetable"_n, queuetable> queue_table;
         typedef eosio::multi_index< "accounts"_n, account > accounts;
         typedef eosio::multi_index< "accounts"_n, accountg > accountsg; 
         typedef eosio::multi_index< "stat"_n, currency_stats > stats;


static asset get_balance_eos( const name& token_contract_account, const name& owner, const symbol_code& sym_code )
      {
          accountsg accountstable( token_contract_account, owner.value );
          const auto& ac = accountstable.get( sym_code.raw() );
          return ac.balance;
      }

static asset get_ecpu_delstake(const name& token_contract_account, const symbol_code& sym_code) // build struct from cpumintofeos account and pull total ecpu staked from table
      {
          stats statstable( token_contract_account, sym_code.raw() );
          const auto& st = statstable.get( sym_code.raw() );
          return st.totaldelegate;
      }
//get cpu power delegated to a user
static asset get_cpu_del_balance( const name& token_contract_account, const name& owner, const symbol_code& sym_code )
      {
          accounts accountstable( token_contract_account, owner.value );
          const auto& ac = accountstable.get( sym_code.raw() );
          return ac.cpupower;
      }

 name get_current_payee(){

        name payee; 
        queue_table queuetable(get_self(), get_self().value);
        auto existing = queuetable.find(get_self().value);
        payee = existing->currentpayee;
        return payee;

    }

    void set_next_round(){ //execute this function after each ECPU mining action to prepare each iteration

        //iterate to next payee
        //see if 12 hours have passed, if so restart starting stake and round time
       

        set_next_payee(); // a & b & d accomplished here 


        if (current_time_point().sec_since_epoch() > (get_payout_start_time()+ (60*60*12))){ 
                
                // reset starting stake, starting times, starting payout pool
                set_ecpu_rpsp(); //restarts remaining pay and starting pay, gets current eos balance to set startpay
                set_start_stake();
                return;

        }
        return;
}

  void update_delegatee(name user, asset value){//update when delegating or undelegating, asset value can be negative

    delegatees delegatee(get_self(), get_self().value);
    auto existing = delegatee.find(user.value);
    const auto& st = *existing;

    if (existing  == delegatee.end()){

            delegatee.emplace( get_self(), [&]( auto& s ) {
                s.lastpaytime = current_time_point().sec_since_epoch();
                s.delegatee = user;
                s.ecpudelegated = value;
                s.delegatetime = current_time_point().sec_since_epoch();
                s.lastpaytime = current_time_point().sec_since_epoch();
            });

    }
    
    else{

        const auto& st = *existing;

        delegatee.modify( st, same_payer, [&]( auto& s ) {
                
                s.delegatetime = current_time_point().sec_since_epoch();
                s.lastpaytime = current_time_point().sec_since_epoch();
                s.ecpudelegated = s.ecpudelegated + value; 
        });
    }

    asset ecpudelegated = get_cpu_del_balance(name{"cpumintofeos"}, user, symbol_code("ECPU") );

    if (ecpudelegated.amount == (0)){
        
        delegatee.erase(existing);
    }
    
  }

  void update_last_paid(name user){ //update last pay time when receiving CPU/NET rental from mining activity

    delegatees delegatee(get_self(), get_self().value);
    auto existing = delegatee.find(user.value);
    const auto& st = *existing;

    delegatee.modify( st, same_payer, [&]( auto& s ) {
        
            s.lastpaytime = current_time_point().sec_since_epoch();
           
    });
}

  uint32_t get_user_last_paid(name user){

    uint32_t lastpaid;

    delegatees delegatee(get_self(), get_self().value);
    auto existing = delegatee.find(user.value);

    lastpaid = existing->lastpaytime;

    return lastpaid;


  }






//----------------------------------------------------------------------------------------------------------------


    name get_next_payee(){

        name nextpayee;
        
        name current_payee = get_current_payee();

        

        delegatees delegatee(get_self(), get_self().value);
        auto existing = delegatee.find(current_payee.value);

        
        if (existing == delegatee.end()){

                existing = delegatee.begin();
                return existing->delegatee;
        }


        existing++;

        if (existing == delegatee.end()){

            existing = delegatee.begin();
        }

        nextpayee = existing->delegatee;

        

        return nextpayee;
    }

    void set_next_payee(){


        name nextpayee = get_next_payee();

        queue_table queuetable(get_self(), get_self().value);
        auto existing = queuetable.find(get_self().value);
        check( existing != queuetable.end(), "contract table not deployed" );
        const auto& st = *existing;

        queuetable.modify( st, same_payer, [&]( auto& s ) {
                   
                    s.currentpayee = nextpayee;

        });

        


    }

    bool is_payee_last(){

        name nextpayee;
        
        name current_payee = get_current_payee();

        delegatees delegatee(get_self(), get_self().value);
        auto existing = delegatee.find(current_payee.value);
        existing++;

        if (existing == delegatee.end()){

            return true;
        }

        else{

            return false;
        }





    }


//------------------ remaining pay getters --------------------------------------------------------//

   
    asset get_remaining_pay_ecpu(){

        asset rp; 

        queue_table queuetable(get_self(), get_self().value);
        auto existing = queuetable.find(get_self().value);

        rp = existing->remainingpay_ecpu;

        return rp;


    }
//----------------------------------------startingpay gettters --------------------------//
    
    asset get_starting_pay_ecpu(){

        asset sp; 

        queue_table queuetable(get_self(), get_self().value);
        auto existing = queuetable.find(get_self().value);

        sp = existing->startpay_ecpu;

        return sp;

//-------------------------------------------------------------------------------------------------------------
    }

    asset get_stake_start(){

        asset stakestart; 

        queue_table queuetable(get_self(), get_self().value);
        auto existing = queuetable.find(get_self().value);

        stakestart = existing->stakestart;

        return stakestart;


    }


    uint32_t get_payout_start_time(){

        uint32_t pst; 

        queue_table queuetable(get_self(), get_self().value);
        auto existing = queuetable.find(get_self().value);

        pst = existing->payoutstarttime;

        return pst;




    }


    asset get_userstake(name user){

        asset stake;

        delegatees delegatee(get_self(), get_self().value);
        auto existing = delegatee.find(user.value);

        stake = existing->ecpudelegated;

        return stake;

    }


    uint32_t get_user_delegatetime(name user){

        uint32_t delegatetime;

        delegatees delegatee(get_self(), get_self().value);
        auto existing = delegatee.find(user.value);

        delegatetime = existing->delegatetime;

        return delegatetime;

    }

    


    void sendasset(name user, asset quantity, const std::string& memo){


      std::string sym =  quantity.symbol.code().to_string();

      if (quantity.amount <= 0){

          return;
      }
  
      
     if (sym == "ECPU"){
        
          action(permission_level{_self, "active"_n}, "cpumintofeos"_n, "transfer"_n, 
          std::make_tuple(get_self(), user, quantity, std::string(memo))).send();

    }

  }





  // SETTERS


  void update_queue(asset quantity){

        std::string tokensym = quantity.symbol.code().to_string();

        queue_table queuetable(get_self(), get_self().value);
        auto existing = queuetable.find(get_self().value);
        check( existing != queuetable.end(), "contract table not deployed" );
        const auto& st = *existing;

        if (tokensym == "ECPU"){

                queuetable.modify( st, same_payer, [&]( auto& s ) {
                    s.remainingpay_ecpu += quantity;
                });

        }



  }

  void update_global_stake(){

   
    delstake_stat globalstake(get_self(),get_self().value);
    auto existing2 = globalstake.find(get_self().value);
    const auto& st2 = *existing2;


    globalstake.modify( st2, same_payer, [&]( auto& s ) {

            s.totaldelstaked = get_ecpu_delstake(name{"cpumintofeos"},asset(0, symbol("ECPU", 8)).symbol.code());//find total stake of ECPU

    });

  }


  asset get_current_stake(){

        /**
        ecpustake_stat globalstake(get_self(),get_self().value);
        auto existing2 = globalstake.find(get_self().value);
        asset ccs = existing2->clubstaked;
        return ccs;
        **/
        
        asset ccs = asset(0, symbol("ECPU", 8));
        
        
        delegatees delegatee(get_self(), get_self().value);
        for (auto it = delegatee.begin(); it != delegatee.end(); it++){

            ccs = ccs + (it->ecpudelegated);

        }

        return ccs;


  }

  void set_start_stake(){

        
        queue_table queuetable(get_self(), get_self().value);
        auto existing = queuetable.find(get_self().value);
        check( existing != queuetable.end(), "contract table not deployed" );
        const auto& st = *existing;

        queuetable.modify( st, same_payer, [&]( auto& s ) {
                    
                    s.stakestart = get_current_stake();
                    s.payoutstarttime = current_time_point().sec_since_epoch();
                
                });


  }
  
  void set_ecpu_rpsp(){

        queue_table queuetable(get_self(), get_self().value);
        auto existing = queuetable.find(get_self().value);
        check( existing != queuetable.end(), "contract table not deployed" );
        const auto& st = *existing;

        asset currentbal = asset(0, symbol(symbol_code("EOS"),4));
        auto sym = currentbal.symbol.code();
        currentbal = get_balance_eos(name{"eosio.token"}, get_self(), sym);

      
        asset sp = currentbal/2;

                queuetable.modify( st, same_payer, [&]( auto& s ) {
                    s.startpay_ecpu = sp;
                    s.remainingpay_ecpu = asset(0, symbol(symbol_code("EOS"),4));
                });

  }
};
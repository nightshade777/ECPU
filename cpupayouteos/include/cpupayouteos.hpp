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
    void initialize(name contract, asset clubstaked);

    [[eosio::action]]
    void resetround(name contract);

    [[eosio::action]] 
    void intdelegatee(name user);

    //listen to mining 
    [[eosio::on_notify("cpumintofeos::minereceipt")]] void cpupowerup(name user);
    
    //listen to stake / unstake
    [[eosio::on_notify("cpumintofeos::delegate")]] void setdelegate(name account, asset value);
    [[eosio::on_notify("cpumintofeos::undelegate")]] void setundelgate(name account, asset value);
    
    
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
    typedef multi_index<name("queuetable"), queuetable> queue_table;





     struct [[eosio::table]] account {
            asset    balance;
            asset    storebalance;
            asset    cpupower;
            asset    unstaking;
            uint32_t  unstake_time;
            

            uint64_t primary_key()const { return balance.symbol.code().raw(); }
         };
  
        typedef eosio::multi_index< "accounts"_n, account > accounts;

        
        struct [[eosio::table]] accountg {
            asset    balance;
            
            

            uint64_t primary_key()const { return balance.symbol.code().raw(); }
         };
  
        typedef eosio::multi_index< "accounts"_n, accountg > accountsg;

        struct [[eosio::table]] currency_stats {
            asset    supply;
            asset    max_supply;
            name     issuer;
            uint32_t      prevmine;
            uint32_t      creationtime;
            asset    totalstake;
            asset    totaldelegate;

            uint64_t primary_key()const { return supply.symbol.code().raw(); }
         };


static asset get_balance( const name& token_contract_account, const name& owner, const symbol_code& sym_code )
      {
          accounts accountstable( token_contract_account, owner.value );
          const auto& ac = accountstable.get( sym_code.raw() );
          return ac.balance;
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

  void update_last_paid(name user){

    delegatees delegatee(get_self(), get_self().value);
    auto existing = delegatee.find(user.value);
    const auto& st = *existing;

    delegatee.emplace( get_self(), [&]( auto& s ) {
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

long double get_paying_ratio(){

    name user = get_current_payee();
    asset stake = get_userstake(user);
    asset ss = get_stake_start();


    long double payout_frac = ((double(stake.amount*1000)) / (double(ss.amount)));

       //check((payout_frac > 0), "error payout fraction is zero");

     return payout_frac;
     }



//get cpu power delegated to a user
        static asset get_cpu_del_balance( const name& token_contract_account, const name& owner, const symbol_code& sym_code )
         {
            accounts accountstable( token_contract_account, owner.value );
            const auto& ac = accountstable.get( sym_code.raw() );
            return ac.cpupower;
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

  
  
  
  void update_global_stake(name user, asset stakechange){

   
    //1 find ecpu stored

    asset cpudelegated = get_userstake(user);


    delegatees delegatee(get_self(), get_self().value);
    auto existing = delegatee.find(user.value);
    const auto& st = *existing;


    delstake_stat globalstake(get_self(),get_self().value);
    auto existing2 = globalstake.find(get_self().value);
    const auto& st2 = *existing2;


    if (existing == delegatee.end()){ //CASE 3 CONSIDERED: DIDNT HAVE DELEGATED STAKED BUT NOW DOES

        delegatee.emplace( get_self(), [&]( auto& s ) {

                        s.delegatee = user;
                        s.ecpudelegated = cpudelegated;
                        s.delegatetime = current_time_point().sec_since_epoch();
                        s.lastpaytime = current_time_point().sec_since_epoch();

                  });
        globalstake.modify( st2, same_payer, [&]( auto& s ) {

                        s.totaldelstaked = get_current_stake();

                  });
    
    }

    else { //already STAKED, STAKED EVEN MORE

        delegatee.modify( st, same_payer, [&]( auto& s ) {

                        s.ecpudelegated = s.ecpudelegated + stakechange;
                        s.delegatetime = current_time_point().sec_since_epoch();
                        s.lastpaytime = current_time_point().sec_since_epoch();

                  });
        globalstake.modify( st2, same_payer, [&]( auto& s ) {

                        s.totaldelstaked = get_current_stake();

                  });


    }




  }

  void update_global_unstake(name user, asset stakechange){

    //CASE1 person with less than 777k unstakes, do nothing -> nothing
    //CASE 2 person with 777k unstakes and still has > 777k -> modify stake club entry, modify global stake
    //CASE 3 person with 777k unstakes and no longer has 777k ->delete stake club entry, modify global stake
    //snapshot 1, internal stake club table (status and stake before the change) 
    //snapshot 2 ecpu storedbalance table (real current stake value)

   
    //4 case four, person with 777k already staked who wasnt on list, unstakes some



    delegatees delegatee(get_self(), get_self().value);
    auto existing = delegatee.find(user.value);
    const auto& st = *existing;

    delstake_stat globalstake(get_self(),get_self().value);
    auto existing2 = globalstake.find(get_self().value);
    const auto& st2 = *existing2;

    queue_table queuetable(get_self(), get_self().value);
    auto existing3 = queuetable.find(get_self().value);
    check( existing3 != queuetable.end(), "contract table not deployed" );
    const auto& st3 = *existing3;

    name nextpayer;

    if (existing3->currentpayee == user){

            existing++;
            nextpayer = existing->delegatee;
            queuetable.modify( st3, same_payer, [&]( auto& s ) {

                        s.currentpayee = nextpayer;

                  });

        }

    if (existing == delegatee.end()){

        return; //CASE 1 ADDRESSED HERE

    }

    asset ecpudelegated = get_cpu_del_balance(name{"cpumintofeos"}, user, symbol_code("ECPU") );

    if (ecpudelegated.amount == (0)){
        
        delegatee.erase(existing);
        
        globalstake.modify( st2, same_payer, [&]( auto& s ) {

                        s.totaldelstaked = get_current_stake();

                  });
    }

    else{

        delegatee.modify( st, same_payer, [&]( auto& s ) {

                        s.ecpudelegated = s.ecpudelegated - stakechange;
                        s.delegatetime = current_time_point().sec_since_epoch();
                    });

        globalstake.modify( st2, same_payer, [&]( auto& s ) {

                        s.totaldelstaked = get_current_stake();

                  });
    }
  }

  asset get_current_stake(){

        /**
        ecpustake_stat globalstake(get_self(),get_self().value);
        auto existing2 = globalstake.find(get_self().value);
        asset ccs = existing2->clubstaked;
        return ccs;
        **/
        
        asset ccs = asset(0, symbol("ECPU", 4));
        
        
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

        asset currentbal = asset(0.0000, symbol(symbol_code("EOS"),4));
        auto sym = currentbal.symbol.code();
        currentbal = get_balance(name{"eosio.token"}, get_self(), sym);

      
        asset sp = currentbal/2;

                queuetable.modify( st, same_payer, [&]( auto& s ) {
                    s.startpay_ecpu = sp;
                    s.remainingpay_ecpu = asset(0.0000, symbol(symbol_code("EOS"),4));
                });

  }
};
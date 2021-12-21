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

    //modify loading ratio

    [[eosio::action]] 
    void modifylr(name user, int loadingratio);

    //listen to mining 
    [[eosio::on_notify("cpumintofeos::minereceipt")]] void cpupowerup(name user);
    
    //listen to stake / unstake
    [[eosio::on_notify("cpumintofeos::delegate")]] void setdelegate(name account, asset value);
    [[eosio::on_notify("cpumintofeos::undelegate")]] void setundelgate(name account, asset value);
    
    [[eosio::on_notify("ecpulpholder::instapowerup")]] void instapowerup( const name& contract, name receiver, asset powerupamt );
    
    TABLE delegatee { 
      
      name      delegatee;
      asset     ecpustaked;
      uint32_t  delegatetime;
      uint32_t  lastpaytime;

      
      auto primary_key() const { return delegatee.value; }
    };
    typedef multi_index<name("delegatee"), delegatee> delegatees;



    TABLE stakestat {
      name       contract;
      asset      totalstaked;
     
      
      auto primary_key() const { return contract.value; }
    };
    typedef multi_index<name("stakestat"), stakestat> stake_stat;



    TABLE queuetable { 
      
      name       contract;
      name       currentpayee;
      int        lastpaytime;
      asset      remainingpay_ecpu;
      asset      startpay_ecpu;
      int        loadingratio; //determines the rate at which divs are loaded from the queue to the active round
      asset      stakestart; //the amount of ecpu staked at start of the active round
      int        payoutstarttime; //the time of the start if the rounf 
      
      
      
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
            int      prevmine;
            int      creationtime;
            asset    totalstake;
            asset    totaldelegate;

            uint64_t primary_key()const { return supply.symbol.code().raw(); }
         };


 name get_current_payee(){

        name payee; 

        queue_table queuetable(get_self(), get_self().value);
        auto existing = queuetable.find(get_self().value);

        payee = existing->currentpayee;

        return payee;

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

        stake = existing->ecpustaked;

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


  uint32_t get_loading_ratio(){

        uint32_t lr; 

        queue_table queuetable(get_self(), get_self().value);
        auto existing = queuetable.find(get_self().value);

        lr = existing->loadingratio;

        return lr;

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


    stake_stat globalstake(get_self(),get_self().value);
    auto existing2 = globalstake.find(get_self().value);
    const auto& st2 = *existing2;


    if (existing == delegatee.end()){ //CASE 3 CONSIDERED: DIDNT HAVE STAKED BUT NOW DOES

        delegatee.emplace( get_self(), [&]( auto& s ) {

                        s.delegatee = user;
                        s.ecpustaked = cpudelegated;
                        s.delegatetime = current_time_point().sec_since_epoch();

                  });
        globalstake.modify( st2, same_payer, [&]( auto& s ) {

                        s.totalstaked = get_current_stake();

                  });
    
    }

    else { //already STAKED, STAKED EVEN MORE

        delegatee.modify( st, same_payer, [&]( auto& s ) {

                        s.ecpustaked = s.ecpustaked + stakechange;
                        s.delegatetime = current_time_point().sec_since_epoch();

                  });
        globalstake.modify( st2, same_payer, [&]( auto& s ) {

                        s.totalstaked = get_current_stake();

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

    stake_stat globalstake(get_self(),get_self().value);
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

    asset ecpustaked = get_cpu_del_balance(name{"cpumintofeos"}, user, symbol_code("ECPU") );

    if (ecpustaked.amount/10000 < (777000)){
        
        delegatee.erase(existing);
        
        globalstake.modify( st2, same_payer, [&]( auto& s ) {

                        s.totalstaked = get_current_stake();

                  });
        
    }

    else{

        delegatee.modify( st, same_payer, [&]( auto& s ) {

                        s.ecpustaked = s.ecpustaked - stakechange;
                        s.delegatetime = current_time_point().sec_since_epoch();
                    });

        globalstake.modify( st2, same_payer, [&]( auto& s ) {

                        s.totalstaked = get_current_stake();

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
        
        asset ccs = asset(0, symbol("ecpu", 4));
        
        
        delegatees delegatee(get_self(), get_self().value);
        for (auto it = delegatee.begin(); it != delegatee.end(); it++){

            ccs = ccs + (it->ecpustaked);

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


        int lr = get_loading_ratio();
        asset sp = lr*get_remaining_pay_ecpu()/1000;

                queuetable.modify( st, same_payer, [&]( auto& s ) {
                    s.startpay_ecpu = sp;
                    s.remainingpay_ecpu = s.remainingpay_ecpu - s.startpay_ecpu;
                });

  }


void set_next_round(){

//iterate to next 
        //a next payee from local stake table
        //b set current payee to the payee you just retrieved
        //c check if payee was last payee on the table, if so, begin new payment round
        //d. current payee set to the start of the staking table
        //e. queue gets 0.1% moved to active round, so move 0.1% of loading ratio from remaining pay to starting pay for all 3 assets
        //f. global stake at start needs to get updated in queue table
        //g.set new payout start time


        bool restart_req = is_payee_last(); // c, new payment round is required if true

        set_next_payee(); // a & b & d accomplished here 

        if (restart_req == false){

            return;

        }

        //c: new round required, new round code here and below:

        //e: 
      
        set_ecpu_rpsp();
     

        //f&g: 
        set_start_stake();
      



}

//BELOW FROM EOSIO.SYSTEM.HPP PERTAINING TO POWERUP SYSTEM
//NEEDED IN ORDER TO FIND SPOT PRICE OF CPU

 static constexpr int64_t powerup_frac = 1'000'000'000'000'000ll;  // 1.0 = 10^15 WARNING CONVERTED FROM INLINE TO STATC TO AVOID ERROR
 static constexpr uint32_t seconds_per_day       = 24 * 3600;

struct powerup_state_resource {
      static constexpr double   default_exponent   = 2.0;                  // Exponent of 2.0 means that the price to reserve a
                                                                           //    tiny amount of resources increases linearly
                                                                           //    with utilization.
      static constexpr uint32_t default_decay_secs = 1 * seconds_per_day;  // 1 day; if 100% of bandwidth resources are in a
                                                                           //    single loan, then, assuming no further powerup usage,
                                                                           //    1 day after it expires the adjusted utilization
                                                                           //    will be at approximately 37% and after 3 days
                                                                           //    the adjusted utilization will be less than 5%.

      uint8_t        version                 = 0;
      int64_t        weight                  = 0;                  // resource market weight. calculated; varies over time.
                                                                   //    1 represents the same amount of resources as 1
                                                                   //    satoshi of SYS staked.
      int64_t        weight_ratio            = 0;                  // resource market weight ratio:
                                                                   //    assumed_stake_weight / (assumed_stake_weight + weight).
                                                                   //    calculated; varies over time. 1x = 10^15. 0.01x = 10^13.
      int64_t        assumed_stake_weight    = 0;                  // Assumed stake weight for ratio calculations.
      int64_t        initial_weight_ratio    = powerup_frac;        // Initial weight_ratio used for linear shrinkage.
      int64_t        target_weight_ratio     = powerup_frac / 100;  // Linearly shrink the weight_ratio to this amount.
      time_point_sec initial_timestamp       = {};                 // When weight_ratio shrinkage started
      time_point_sec target_timestamp        = {};                 // Stop automatic weight_ratio shrinkage at this time. Once this
                                                                   //    time hits, weight_ratio will be target_weight_ratio.
      double         exponent                = default_exponent;   // Exponent of resource price curve.
      uint32_t       decay_secs              = default_decay_secs; // Number of seconds for the gap between adjusted resource
                                                                   //    utilization and instantaneous utilization to shrink by 63%.
      asset          min_price               = {};                 // Fee needed to reserve the entire resource market weight at
                                                                   //    the minimum price (defaults to 0).
      asset          max_price               = {};                 // Fee needed to reserve the entire resource market weight at
                                                                   //    the maximum price.
      int64_t        utilization             = 0;                  // Instantaneous resource utilization. This is the current
                                                                   //    amount sold. utilization <= weight.
      int64_t        adjusted_utilization    = 0;                  // Adjusted resource utilization. This is >= utilization and
                                                                   //    <= weight. It grows instantly but decays exponentially.
      time_point_sec utilization_timestamp   = {};                 // When adjusted_utilization was last updated
   };

   struct [[eosio::table("powup.state"),eosio::contract("eosio.system")]] powerup_state {
      static constexpr uint32_t default_powerup_days = 30; // 30 day resource powerups

      uint8_t                    version           = 0;
      powerup_state_resource     net               = {};                     // NET market state
      powerup_state_resource     cpu               = {};                     // CPU market state
      uint32_t                   powerup_days      = default_powerup_days;   // `powerup` `days` argument must match this.
      asset                      min_powerup_fee   = {};                     // fees below this amount are rejected

      uint64_t primary_key()const { return 0; }
   };



 

};
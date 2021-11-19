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
    void intstaker(name user);

    //modify loading ratio

    [[eosio::action]] 
    void modifylr(name user, int loadingratio);

    //listen to mining 
    [[eosio::on_notify("sovdexrelays::minereceipt")]] void paydiv(name user, eosio::asset sovburned, asset minepool);

    //listen to transfers to get paid out
    [[eosio::on_notify("fakexautforu::transfer")]] void insertgold(name from, name to, asset quantity, std::string memo);
    [[eosio::on_notify("btc.ptokens::transfer")]] void insertbtc(name from, name to, asset quantity, std::string memo);
    [[eosio::on_notify("svxmintofeos::transfer")]] void insertsvx(name from, name to, asset quantity, std::string memo);
    
    //listen to stake / unstake
    [[eosio::on_notify("svxmintofeos::stake")]] void setstake(name account, asset value);
    [[eosio::on_notify("svxmintofeos::unstake")]] void setunstake(name account, asset value);
    
    
    
    TABLE staketable { 
      
      name      staker;
      asset     svxstaked;
      uint32_t  staketime;
      
      auto primary_key() const { return staker.value; }
    };
    typedef multi_index<name("staketable"), staketable> stake_table;



    TABLE svxstakestat {
      name       contract;
      asset      clubstaked;
     
      
      auto primary_key() const { return contract.value; }
    };
    typedef multi_index<name("svxstakestat"), svxstakestat> svxstake_stat;



    TABLE queuetable { 
      
      name       contract;
      
      //asset      currenttoken; // current token being paid out 
      name       currentpayee;

      asset      remainingpay_gold; // divs loaded in queue
      asset      remainingpay_btc;
      asset      remainingpay_svx;

      asset      startpay_gold; // amount of divs at the start of the round, should be calc from LR*queue
      asset      startpay_btc;
      asset      startpay_svx;

      int        loadingratio; //determines the rate at which divs are loaded from the queue to the active round

      asset      clubstakestart; //the amount of SVX staked in 777 club at the start of the active round
      
      int        payoutstarttime; //the time of the start if the rounf 
      
      
      
      auto primary_key() const { return contract.value; }

    };
    typedef multi_index<name("queuetable"), queuetable> queue_table;





     struct [[eosio::table]] account {
            asset    balance;
            asset    storebalance;
            asset    svxpower;
            asset    unstaking;
            uint32_t  unstake_time;
            

            uint64_t primary_key()const { return balance.symbol.code().raw(); }
         };
  
        typedef eosio::multi_index< "accounts"_n, account > accounts;

        
        static asset get_stored_balance( const name& token_contract_account, const name& owner, const symbol_code& sym_code )
         {
            accounts accountstable( token_contract_account, owner.value );
            const auto& ac = accountstable.get( sym_code.raw() );
            return ac.storebalance;
        }



        struct [[eosio::table]] accountg {
            asset    balance;
            
            

            uint64_t primary_key()const { return balance.symbol.code().raw(); }
         };
  
        typedef eosio::multi_index< "accounts"_n, accountg > accountsg;

//----------------------------------------------------------------------------------------------------------------

    name get_current_payee(){

        name payee; 

        queue_table queuetable(get_self(), get_self().value);
        auto existing = queuetable.find(get_self().value);

        payee = existing->currentpayee;

        return payee;

    }

    name get_next_payee(){

        name nextpayee;
        
        name current_payee = get_current_payee();

        

        stake_table staketable(get_self(), get_self().value);
        auto existing = staketable.find(current_payee.value);

        
        if (existing == staketable.end()){

                existing = staketable.begin();
                return existing->staker;
        }


        existing++;

        if (existing == staketable.end()){

            existing = staketable.begin();
        }

        nextpayee = existing->staker;

        

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

        stake_table staketable(get_self(), get_self().value);
        auto existing = staketable.find(current_payee.value);
        existing++;

        if (existing == staketable.end()){

            return true;
        }

        else{

            return false;
        }





    }


//------------------ remaining pay getters --------------------------------------------------------//

    asset get_remaining_pay_gold(){

        asset rp; 

        queue_table queuetable(get_self(), get_self().value);
        auto existing = queuetable.find(get_self().value);

        rp = existing->remainingpay_gold;

        return rp;


    }
    asset get_remaining_pay_btc(){

        asset rp; 

        queue_table queuetable(get_self(), get_self().value);
        auto existing = queuetable.find(get_self().value);

        rp = existing->remainingpay_btc;

        return rp;


    }
    asset get_remaining_pay_svx(){

        asset rp; 

        queue_table queuetable(get_self(), get_self().value);
        auto existing = queuetable.find(get_self().value);

        rp = existing->remainingpay_svx;

        return rp;


    }
//----------------------------------------startingpay gettters --------------------------//
    asset get_starting_pay_gold(){

        asset sp; 

        queue_table queuetable(get_self(), get_self().value);
        auto existing = queuetable.find(get_self().value);

        sp = existing->startpay_gold;

        return sp;


    }
    asset get_starting_pay_btc(){

        asset sp; 

        queue_table queuetable(get_self(), get_self().value);
        auto existing = queuetable.find(get_self().value);

        sp = existing->startpay_btc;

        return sp;


    }
    asset get_starting_pay_svx(){

        asset sp; 

        queue_table queuetable(get_self(), get_self().value);
        auto existing = queuetable.find(get_self().value);

        sp = existing->startpay_svx;

        return sp;

//-------------------------------------------------------------------------------------------------------------
    }

    asset get_club_stake_start(){

        asset css; 

        queue_table queuetable(get_self(), get_self().value);
        auto existing = queuetable.find(get_self().value);

        css = existing->clubstakestart;

        return css;


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

        stake_table staketable(get_self(), get_self().value);
        auto existing = staketable.find(user.value);

        stake = existing->svxstaked;

        return stake;

    }


    uint32_t get_userstaketime(name user){

        uint32_t staketime;

        stake_table staketable(get_self(), get_self().value);
        auto existing = staketable.find(user.value);

        staketime = existing->staketime;

        return staketime;

    }

    long double get_paying_ratio(){

        name user = get_current_payee();
        asset stake = get_userstake(user);
        asset css = get_club_stake_start();


       long double payout_frac = ((double(stake.amount*1000)) / (double(css.amount)));

       //check((payout_frac > 0), "error payout fraction is zero");

        return payout_frac;
    }


    void sendasset(name user, asset quantity, const std::string& memo){


      std::string sym =  quantity.symbol.code().to_string();

      if (quantity.amount <= 0){

          return;
      }


     if (sym == "PBTC"){
        
          action(permission_level{_self, "active"_n}, "btc.ptokens"_n, "transfer"_n, 
          std::make_tuple(get_self(), user, quantity, std::string(memo))).send();
        
      }

      if (sym == "XAUT"){
        
          action(permission_level{_self, "active"_n}, "fakexautforu"_n, "transfer"_n, 
          std::make_tuple(get_self(), user, quantity, std::string(memo))).send();
        
      }
  
      
     if (sym == "SVX"){
        
          action(permission_level{_self, "active"_n}, "svxmintofeos"_n, "transfer"_n, 
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

        if (tokensym == "PBTC"){


                queuetable.modify( st, same_payer, [&]( auto& s ) {
                    s.remainingpay_btc += quantity;
                });

        }

        if (tokensym == "XAUT"){


                queuetable.modify( st, same_payer, [&]( auto& s ) {
                    s.remainingpay_gold += quantity;
                });

        }

        if (tokensym == "SVX"){


                queuetable.modify( st, same_payer, [&]( auto& s ) {
                    s.remainingpay_svx += quantity;
                });

        }



  }

  void update_global_stake(name user, asset stakechange){

   //1 case one, didnt have 777k, staked and still doesnt have 777k - edit nothing 
   //2 case two, had 777k, staked and now has even more - modify global, modify club table
   //3 case three didnt have 777k, but now has 777k - modify global stake, emplace club stake table

   
   //4 case four, person with 777k already staked who wasnt on list, stakes more

   //snapshot 1, internal stake club table (status and stake before the change) 
   //snapshot 2 svx storedbalance table (real current stake value)
   
   
    //1 find svx stored

    asset stored = get_stored_balance(name{"svxmintofeos"}, user, symbol_code("SVX") );

    if (stored.amount/10000 < (777000)){  //CASE1 CONSIDERED: NOW DOESNT HAVE 777K STAKED
        
        return;

    }

    stake_table staketable(get_self(), get_self().value);
    auto existing = staketable.find(user.value);
    const auto& st = *existing;


    svxstake_stat globalstake(get_self(),get_self().value);
    auto existing2 = globalstake.find(get_self().value);
    const auto& st2 = *existing2;


    if (existing == staketable.end()){ //CASE 3 CONSIDERED: DIDNT HAVE 777K STAKED BUT NOW DOES

        staketable.emplace( get_self(), [&]( auto& s ) {

                        s.staker = user;
                        s.svxstaked = stored;
                        s.staketime = current_time_point().sec_since_epoch();

                  });
        globalstake.modify( st2, same_payer, [&]( auto& s ) {

                        s.clubstaked = get_current_club_stake();

                  });
    
    }

    else { //CASE3 CONSIDERED: HAS 777K STAKED, STAKED EVEN MORE

        staketable.modify( st, same_payer, [&]( auto& s ) {

                        s.svxstaked = s.svxstaked + stakechange;
                        s.staketime = current_time_point().sec_since_epoch();

                  });
        globalstake.modify( st2, same_payer, [&]( auto& s ) {

                        s.clubstaked = get_current_club_stake();

                  });


    }




  }

  void update_global_unstake(name user, asset stakechange){

    //CASE1 person with less than 777k unstakes, do nothing -> nothing
    //CASE 2 person with 777k unstakes and still has > 777k -> modify stake club entry, modify global stake
    //CASE 3 person with 777k unstakes and no longer has 777k ->delete stake club entry, modify global stake
    //snapshot 1, internal stake club table (status and stake before the change) 
    //snapshot 2 svx storedbalance table (real current stake value)

   
    //4 case four, person with 777k already staked who wasnt on list, unstakes some



    stake_table staketable(get_self(), get_self().value);
    auto existing = staketable.find(user.value);
    const auto& st = *existing;

    svxstake_stat globalstake(get_self(),get_self().value);
    auto existing2 = globalstake.find(get_self().value);
    const auto& st2 = *existing2;

    queue_table queuetable(get_self(), get_self().value);
    auto existing3 = queuetable.find(get_self().value);
    check( existing3 != queuetable.end(), "contract table not deployed" );
    const auto& st3 = *existing3;

    name nextpayer;

    if (existing3->currentpayee == user){

            existing++;
            nextpayer = existing->staker;
            queuetable.modify( st3, same_payer, [&]( auto& s ) {

                        s.currentpayee = nextpayer;

                  });


        }






    if (existing == staketable.end()){

        return; //CASE 1 ADDRESSED HERE

    }

    asset stored = get_stored_balance(name{"svxmintofeos"}, user, symbol_code("SVX") );

    if (stored.amount/10000 < (777000)){
        
        staketable.erase(existing);
        
        globalstake.modify( st2, same_payer, [&]( auto& s ) {

                        s.clubstaked = get_current_club_stake();

                  });
        
    }

    else{

        staketable.modify( st, same_payer, [&]( auto& s ) {

                        s.svxstaked = s.svxstaked - stakechange;
                        s.staketime = current_time_point().sec_since_epoch();
                    });

        globalstake.modify( st2, same_payer, [&]( auto& s ) {

                        s.clubstaked = get_current_club_stake();

                  });



    }

        


  }

  asset get_current_club_stake(){

        /**
        svxstake_stat globalstake(get_self(),get_self().value);
        auto existing2 = globalstake.find(get_self().value);
        asset ccs = existing2->clubstaked;
        return ccs;
        **/
        
        asset ccs = asset(0, symbol("SVX", 4));
        
        
        stake_table staketable(get_self(), get_self().value);
        for (auto it = staketable.begin(); it != staketable.end(); it++){

            ccs = ccs + (it->svxstaked);

        }

        return ccs;


  }

  void set_start_stake(){

        
        queue_table queuetable(get_self(), get_self().value);
        auto existing = queuetable.find(get_self().value);
        check( existing != queuetable.end(), "contract table not deployed" );
        const auto& st = *existing;

        queuetable.modify( st, same_payer, [&]( auto& s ) {
                    
                    s.clubstakestart = get_current_club_stake();
                    s.payoutstarttime = current_time_point().sec_since_epoch();
                
                });


  }

  void set_gold_rpsp(){

        
        queue_table queuetable(get_self(), get_self().value);
        auto existing = queuetable.find(get_self().value);
        check( existing != queuetable.end(), "contract table not deployed" );
        const auto& st = *existing;


        int lr = get_loading_ratio();
        asset sp = lr*get_remaining_pay_gold()/1000;

                queuetable.modify( st, same_payer, [&]( auto& s ) {
                    s.startpay_gold = sp;
                    s.remainingpay_gold = s.remainingpay_gold - s.startpay_gold;
                });

}

  void set_svx_rpsp(){

        queue_table queuetable(get_self(), get_self().value);
        auto existing = queuetable.find(get_self().value);
        check( existing != queuetable.end(), "contract table not deployed" );
        const auto& st = *existing;


        int lr = get_loading_ratio();
        asset sp = lr*get_remaining_pay_svx()/1000;

                queuetable.modify( st, same_payer, [&]( auto& s ) {
                    s.startpay_svx = sp;
                    s.remainingpay_svx = s.remainingpay_svx - s.startpay_svx;
                });

  }

  void set_btc_rpsp(){

        queue_table queuetable(get_self(), get_self().value);
        auto existing = queuetable.find(get_self().value);
        check( existing != queuetable.end(), "contract table not deployed" );
        const auto& st = *existing;


        int lr = get_loading_ratio();
        asset sp = lr*get_remaining_pay_btc()/1000;

                queuetable.modify( st, same_payer, [&]( auto& s ) {
                    s.startpay_btc = sp;
                    s.remainingpay_btc = s.remainingpay_btc - s.startpay_btc;
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
        set_btc_rpsp();
        set_svx_rpsp();
        set_gold_rpsp();

        //f&g: 
        set_start_stake();
      



}


bool check_pbtc_balance(name miner ){
            
            
            asset assetname = asset(0, symbol("PBTC", 8));

            
            auto sym_code = assetname.symbol.code();

            accountsg accountstable( name{"btc.ptokens"}, miner.value );
            
            auto existing = accountstable.find(sym_code.raw());
            
            if (existing == accountstable.end()){

                return false;

            }
            //check(existing != accountstable.end(),"Must have a balance for this asset before buying. Please use open action to initialize ram");
            
            return true;
         }

 

};
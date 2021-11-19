#include <cpupayouteos.hpp>


[[eosio::action]] 
void cpupayouteos::initialize(name contract, asset clubstaked){

      require_auth(get_self());

      queue_table queuetable(get_self(), get_self().value);
      auto existing = queuetable.find(get_self().value);
      
      queuetable.emplace( get_self(), [&]( auto& s ) {
      
            s.contract = name{"cpupayouteos"};
     
            s.currentpayee = name{"therealgavin"};

            s.remainingpay_gold = asset(0, symbol("XAUT", 4)); 
            s.remainingpay_btc = asset(0, symbol("PBTC", 8)); 
            s.remainingpay_svx = asset(0, symbol("SVX", 4)); 

            s.startpay_gold = asset(0, symbol("XAUT", 4)); 
            s.startpay_btc = asset(0, symbol("PBTC", 8)); 
            s.startpay_svx = asset(0, symbol("SVX", 4));

            s.loadingratio = 1; 
            s.clubstakestart = clubstaked; 
            s.payoutstarttime = current_time_point().sec_since_epoch();
                  
      });

      svxstake_stat globalstake(get_self(),get_self().value);
      auto existing2 = globalstake.find(get_self().value);
      
      globalstake.emplace( get_self(), [&]( auto& s ) {

            s.contract = name{"cpupayouteos"};
            s.clubstaked = asset(0, symbol("SVX", 4));

      });

}

[[eosio::action]]
void cpupayouteos::resetround(name contract){

      require_auth(get_self());

     

      stake_table staketable(get_self(), get_self().value);
      auto existing2 = staketable.begin();
      name firstpayee =  existing2->staker;

      

      queue_table queuetable(get_self(), get_self().value);
      auto existing = queuetable.find(get_self().value);

      const auto& st = *existing;

      queuetable.modify( st, same_payer, [&]( auto& s ) {
                   
            s.currentpayee = firstpayee;
            s.clubstakestart = get_current_club_stake(); 
            s.payoutstarttime = current_time_point().sec_since_epoch();

            //s.remainingpay_btc = asset(0, symbol("PBTC", 8)); 
            //s.startpay_btc = asset(0, symbol("PBTC", 8)); 

      });

      set_svx_rpsp();
      set_btc_rpsp();
      set_gold_rpsp();



}

[[eosio::action]] 
void cpupayouteos::intstaker(name user){

      require_auth(get_self());

      
      asset stored = get_stored_balance(name{"svxmintofeos"}, user, symbol_code("SVX") );

      check(((stored.amount / 10000) >= 777000),"user does not have enough staked to be in 777 club");

      stake_table staketable(get_self(), get_self().value);
      auto existing = staketable.find(user.value);
      const auto& st = *existing;



      
      

      if (existing == staketable.end()){

      
                  staketable.emplace( get_self(), [&]( auto& s ) {

                        s.staker = user;
                        s.svxstaked = stored;
                        s.staketime = current_time_point().sec_since_epoch();

                  });
      }

      else {

                  staketable.modify( st, same_payer, [&]( auto& s ) {

                        s.staker = user;
                        s.svxstaked = stored;
                        s.staketime = current_time_point().sec_since_epoch();

                  });


      }


      svxstake_stat globalstake(get_self(),get_self().value);
      auto existing2 = globalstake.find(get_self().value);
      const auto& st2 = *existing2;


      globalstake.modify( st2, same_payer, [&]( auto& s ) {

                        s.clubstaked = get_current_club_stake(); 

                  });







}


[[eosio::action]] 
void cpupayouteos::modifylr(name user, int loadingratio){

      require_auth(get_self());
      
      queue_table queuetable(get_self(), get_self().value);
      auto existing = queuetable.find(get_self().value);
      const auto& st = *existing;

      queuetable.modify( st, same_payer, [&]( auto& s ) {
            s.loadingratio = loadingratio;
      });




}


 [[eosio::on_notify("sovdexrelays::minereceipt")]] void cpupayouteos::paydiv(name user, asset sovburned, asset minepool){

    

    //1 get current payee
    name payee = get_current_payee();
    //2 see if payee stake time is after start of payout round
    uint32_t userstaketime = get_userstaketime(payee);
    uint32_t roundstarttime = get_payout_start_time();

    if (userstaketime > roundstarttime){
      
         set_next_round();
         return;

      }
    

    
    //3 find current payout amount with payout fraction for user

    
    
    //---------Gold Payout --------------------//
    
    long double payratio = get_paying_ratio();

    asset startpay_gold = get_starting_pay_gold();
    asset divpayment_gold = payratio*startpay_gold;
    asset minepayment_gold  = divpayment_gold;

    
    
   

    //check(1!=1,"code got here cpp 187");

    sendasset(payee, divpayment_gold, "Gold Payout 777 Club Member");
    sendasset(user, minepayment_gold, "Gold Payout for Mining");

   
   //-------------BTC Payout --------------------------//


    asset startpay_btc = get_starting_pay_btc();
    asset divpayment_btc = startpay_btc;
    divpayment_btc.amount = double(payratio*startpay_btc.amount)/1000/10000;
    asset minepayment_btc = divpayment_btc;
    minepayment_btc.amount = 0;
    divpayment_btc = divpayment_btc - minepayment_btc;

    

    sendasset(payee, divpayment_btc, "BTC Payout 777 Club Member");
    
    if (check_pbtc_balance(user) == true){

            sendasset(user, minepayment_btc, "BTC Payout for Mining");
    
    }


    //--------------SVX Payout (no Miner SVX) ----------------------//
      
      
    asset startpay_svx = get_starting_pay_svx();
    asset divpayment_svx = startpay_svx;
    divpayment_svx.amount = double(payratio*startpay_svx.amount)/1000;
    
    
    sendasset(payee, divpayment_svx, "SVX Payout 777 Club Member");


    set_next_round();

    
            
    
    
    
   


    


   

 }


[[eosio::on_notify("fakexautforu::transfer")]] void cpupayouteos::insertgold(name from, name to, asset quantity, std::string memo){
      if (from ==get_self() || to != get_self()){
            return;
      }
      update_queue(quantity);

}
[[eosio::on_notify("btc.ptokens::transfer")]] void cpupayouteos::insertbtc(name from, name to, asset quantity, std::string memo){
      if (from ==get_self() || to != get_self()){
            return;
      }
      update_queue(quantity);

}

[[eosio::on_notify("svxmintofeos::transfer")]] void cpupayouteos::insertsvx(name from, name to, asset quantity, std::string memo){
      if (from ==get_self() || to != get_self()){
            return;
      }
      update_queue(quantity);

}

[[eosio::on_notify("svxmintofeos::stake")]] void cpupayouteos::setstake(name account, asset value){

      update_global_stake(account, value);



}


[[eosio::on_notify("svxmintofeos::unstake")]] void cpupayouteos::setunstake(name account, asset value){

      update_global_unstake(account, value);

}
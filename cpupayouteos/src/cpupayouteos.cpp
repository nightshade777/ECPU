#include <cpupayouteos.hpp>


[[eosio::action]] 
void cpupayouteos::initialize(name contract, asset totalstaked){

      require_auth(get_self());

      queue_table queuetable(get_self(), get_self().value);
      auto existing = queuetable.find(get_self().value);
      
      queuetable.emplace( get_self(), [&]( auto& s ) {
      
            s.contract = name{"cpupayouteos"};
     
            s.currentpayee = name{"therealgavin"};

            
            s.remainingpay_ecpu = asset(0, symbol("ECPU", 4)); 
            s.startpay_ecpu = asset(0, symbol("ECPU", 4));

            s.loadingratio = 1; 
            s.clubstakestart = totalstaked; 
            s.payoutstarttime = current_time_point().sec_since_epoch();
                  
      });

      stake_stat globalstake(get_self(),get_self().value);
      auto existing2 = globalstake.find(get_self().value);
      
      globalstake.emplace( get_self(), [&]( auto& s ) {

            s.contract = name{"cpupayouteos"};
            s.totalstaked = asset(0, symbol("ECPU", 4));

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

      });

      set_ecpu_rpsp();

}

[[eosio::action]] 
void cpupayouteos::intstaker(name user){

      require_auth(get_self());

      
      asset stored = get_stored_balance(name{"cpumintofeos"}, user, symbol_code("ECPU") );



      stake_table staketable(get_self(), get_self().value);
      auto existing = staketable.find(user.value);
      const auto& st = *existing;



      
      

      if (existing == staketable.end()){

      
                  staketable.emplace( get_self(), [&]( auto& s ) {

                        s.staker = user;
                        s.ecpustaked = stored;
                        s.staketime = current_time_point().sec_since_epoch();

                  });
      }

      else {

                  staketable.modify( st, same_payer, [&]( auto& s ) {

                        s.staker = user;
                        s.ecpustaked = stored;
                        s.staketime = current_time_point().sec_since_epoch();

                  });


      }


      stake_stat globalstake(get_self(),get_self().value);
      auto existing2 = globalstake.find(get_self().value);
      const auto& st2 = *existing2;


      globalstake.modify( st2, same_payer, [&]( auto& s ) {

                        s.totalstaked = get_current_club_stake(); 

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


 [[eosio::on_notify("cpumintofeos::minereceipt")]] void cpupayouteos::payreward(name user, asset minepool){

    

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

   
    
    long double payratio = get_paying_ratio();



      
      
    asset startpay_ecpu = get_starting_pay_ecpu();
    asset divpayment_ecpu = startpay_ecpu;
    divpayment_ecpu.amount = double(payratio*startpay_ecpu.amount)/1000;
    
    
    sendasset(payee, divpayment_ecpu, "ECPU Payout stake payout");


    set_next_round();

    

 }


[[eosio::on_notify("cpumintofeos::delegate")]] void cpupayouteos::setdelegate(name account, asset value){

      update_global_stake(account, value);



}


[[eosio::on_notify("cpumintofeos::undelegate")]] void cpupayouteos::setundelgate(name account, asset value){

      update_global_unstake(account, value);

}
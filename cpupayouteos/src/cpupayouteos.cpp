#include <cpupayouteos.hpp>


[[eosio::action]] 
void cpupayouteos::initialize(name contract){

//TO DO delete tables if existing and re-emplace

      require_auth(get_self());

      queue_table queuetable(get_self(), get_self().value);
      auto existing = queuetable.find(get_self().value);

      //if (existing !=queuetable.end()){

        //   queuetable.erase(existing);

      //}

      if(existing == queuetable.end()){
      
            queuetable.emplace( get_self(), [&]( auto& s ) {
      
                  s.contract = name{"cpupayouteos"};
                  s.currentpayee = name{"ddctesterxcr"};
                  s.remainingpay_ecpu = asset(0, symbol("EOS", 4)); 
                  s.startpay_ecpu = get_balance_eos(name{"eosio.token"}, name{"cpupayouteos"}, symbol_code("EOS")); 
                  s.stakestart = get_ecpu_delstake(name{"cpumintofeos"},asset(0, symbol("ECPU", 8)).symbol.code());
                  s.payoutstarttime = current_time_point().sec_since_epoch();
                  
            });

      }
      else {
            queuetable.modify( existing, get_self(), [&]( auto& s ){

                  s.contract = name{"cpupayouteos"};
                  s.currentpayee = name{"ddctesterxcr"};
                  s.remainingpay_ecpu = asset(0, symbol("EOS", 4)); 
                  s.startpay_ecpu = get_balance_eos(name{"eosio.token"}, name{"cpupayouteos"}, symbol_code("EOS")); 
                  s.stakestart = get_ecpu_delstake(name{"cpumintofeos"},asset(0, symbol("ECPU", 8)).symbol.code());
                  s.payoutstarttime = current_time_point().sec_since_epoch();

            });
      }
       


      delstake_stat globalstake(get_self(),get_self().value);
      auto existing2 = globalstake.find(get_self().value);

      if(existing2 == globalstake.end()){
      
            globalstake.emplace( get_self(), [&]( auto& s ) {

                  s.contract = name{"cpupayouteos"};
                  s.totaldelstaked = get_ecpu_delstake(name{"cpumintofeos"},asset(0, symbol("ECPU", 8)).symbol.code());//find total delegated stake of ECPU

            });
      }
      else {
            
            globalstake.modify( existing2, get_self(), [&]( auto& s ){

                  s.contract = name{"cpupayouteos"};
                  s.totaldelstaked = get_ecpu_delstake(name{"cpumintofeos"},asset(0, symbol("ECPU", 8)).symbol.code());//find total delegated stake of ECPU

            });

      }

}



[[eosio::action]]
void cpupayouteos::resetround(name contract){

      require_auth(get_self());

     

      delegatees delegatee(get_self(), get_self().value);
      auto existing2 = delegatee.begin();
      name firstpayee =  existing2->delegatee;

      

      queue_table queuetable(get_self(), get_self().value);
      auto existing = queuetable.find(get_self().value);

      const auto& st = *existing;

      queuetable.modify( st, same_payer, [&]( auto& s ) {
                   
            s.currentpayee = firstpayee;
            s.stakestart = get_ecpu_delstake(name{"cpumintofeos"},asset(0, symbol("ECPU", 8)).symbol.code());
            s.payoutstarttime = current_time_point().sec_since_epoch();

      });

      set_ecpu_rpsp();

}

[[eosio::action]] 
void cpupayouteos::intdelegatee(name user){

      require_auth(get_self());

      asset staked = get_cpu_del_balance(name{"cpumintofeos"}, user, symbol_code("ECPU") );

      delegatees delegatee(get_self(), get_self().value);
      auto existing = delegatee.find(user.value);
      const auto& st = *existing;

      //delegatee.erase(existing);
     
      if (existing == delegatee.end()){

      
                  delegatee.emplace( get_self(), [&]( auto& s ) {

                        s.delegatee = user;
                        s.ecpudelegated = staked;
                        s.delegatetime = current_time_point().sec_since_epoch();

                  });
      }

      else {

                  delegatee.modify( st, same_payer, [&]( auto& s ) {

                        s.delegatee = user;
                        s.ecpudelegated = staked;
                        s.delegatetime = current_time_point().sec_since_epoch();

                  });


      }


      delstake_stat globalstake(get_self(),get_self().value);
      auto existing2 = globalstake.find(get_self().value);
      const auto& st2 = *existing2;


      globalstake.modify( st2, same_payer, [&]( auto& s ) {

                        s.totaldelstaked = get_ecpu_delstake(name{"cpumintofeos"},asset(0, symbol("ECPU", 8)).symbol.code());//find delegated total stake of ECPU

                  });
                  
}





 [[eosio::on_notify("cpumintofeos::minereceipt")]] void cpupayouteos::cpupowerup(name miner){

    
     //check(1!=1, "code got here");

    uint32_t twelve_hours = 1;
     //
    //1 get current payee
    name payee = get_current_payee();
    //2 see if payee stake time is after start of payout round
    uint32_t userlastpaytime = get_user_last_paid(payee);
    uint32_t userlastdelegate = get_user_delegatetime(payee);
    
    //has 12 hours past?, if not go to next payee and end action
    if (userlastpaytime > (current_time_point().sec_since_epoch() - twelve_hours)){
         set_next_round(); //iterate to next payee and setup next round
         return;
     }
     else if (userlastdelegate > (current_time_point().sec_since_epoch() - twelve_hours)){
         set_next_round(); //iterate to next payee and setup next round
         return;
     }

     else { //set last paid time to now
  
         update_last_paid(payee);

     }
    //get ECPU delegated to payee
    asset payeebal = get_cpu_del_balance(name{"cpumintofeos"}, payee, symbol_code("ECPU") );
    //get total ECPU delegated in total
    asset totaldelstake = get_ecpu_delstake(name{"cpumintofeos"},asset(0, symbol("ECPU", 8)).symbol.code());
    //
    asset startpay_ecpu = get_starting_pay_ecpu();
    asset powerup = asset(0, symbol("EOS", 4)); //initialization

    powerup = startpay_ecpu*(double(payeebal.amount))/(double(totaldelstake.amount))/2;
    
    check(powerup.amount != 0, "powerup amount is zero");
    
    action(permission_level{_self, "active"_n}, "eosio.token"_n, "transfer"_n, 
          std::make_tuple(get_self(), name{"powerupcalc1"}, powerup, payee.to_string())).send();


    set_next_round();

    queue_table queuetable(get_self(), get_self().value);
    auto existing = queuetable.find(get_self().value);

    queuetable.modify( existing, get_self(), [&]( auto& s ){

            s.remainingpay_ecpu = s.remainingpay_ecpu - powerup;

    });

    

 }


[[eosio::on_notify("cpumintofeos::delegate")]] void cpupayouteos::setdelegate(name account, name receiver, asset value){

      update_global_stake();
      update_delegatee(receiver, value);



}
[[eosio::on_notify("cpumintofeos::undelegate")]] void cpupayouteos::setundelgate(name account, name receiver, asset value){

      update_global_stake();
      update_delegatee(receiver, -value);

}

[[eosio::on_notify("eosio.token::transfer")]] void cpupayouteos::resetround(name from, name to, eosio::asset quantity, std::string memo){

   if (to != get_self()) {

         return;
   }

   if (from != name{"ecpulpholder"}){

         return;
   }
   queue_table queuetable(get_self(), get_self().value);
   auto existing = queuetable.find(get_self().value);

   queuetable.modify( existing, get_self(), [&]( auto& s ){

                  
                  s.remainingpay_ecpu = get_balance_eos(name{"eosio.token"}, name{"cpupayouteos"}, symbol_code("EOS"));
                  s.startpay_ecpu = get_balance_eos(name{"eosio.token"}, name{"cpupayouteos"}, symbol_code("EOS")); 
                  s.payoutstarttime = current_time_point().sec_since_epoch();

   });



}


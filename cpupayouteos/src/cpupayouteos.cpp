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
                  s.totaldelstaked = get_ecpu_delstake(name{"cpumintofeos"},asset(0, symbol("ECPU", 8)).symbol.code());

            });
      }
      else {
            
            globalstake.modify( existing2, get_self(), [&]( auto& s ){

                  s.contract = name{"cpupayouteos"};
                  s.totaldelstaked = get_ecpu_delstake(name{"cpumintofeos"},asset(0, symbol("ECPU", 8)).symbol.code());

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
            s.stakestart = get_current_stake(); 
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

                        s.totaldelstaked = get_current_stake(); 

                  });
}





 [[eosio::on_notify("cpumintofeos::minereceipt")]] void cpupayouteos::cpupowerup(name miner){

    
     check(1!=1, "code got here");
    //1 get current payee
    name payee = get_current_payee();
    //2 see if payee stake time is after start of payout round
    uint32_t userlastpaytime = get_user_last_paid(payee);
    uint32_t userlastdelegate = get_user_delegatetime(payee);
    
    //has 12 hours past?, if not go to next payee and end action
    if (userlastpaytime < current_time_point().sec_since_epoch(); + 60*60*12){
         set_next_round(); //iterate to next payee and setup next round
         return;
     }
     else if (userlastdelegate < current_time_point().sec_since_epoch(); + 60*60*12){
         set_next_round(); //iterate to next payee and setup next round
         return;
     }

     else { //set last paid time to now
  
         update_last_paid(payee);

     }
    
     long double payratio = get_paying_ratio();

    
    //3 find current payout amount with payout fraction for user

    asset startpay_ecpu = get_starting_pay_ecpu();
    asset divpayment_ecpu = asset(0, symbol("EOS", 4)); //initialization
    divpayment_ecpu.amount = double(payratio*startpay_ecpu.amount)/100;
    
    
    action(permission_level{_self, "active"_n}, "eosio.token"_n, "transfer"_n, 
          std::make_tuple(get_self(), name{"powerupcalc1"}, divpayment_ecpu, payee.to_string())).send();


    set_next_round();

    

 }


[[eosio::on_notify("cpumintofeos::delegate")]] void cpupayouteos::setdelegate(name account, name receiver, asset value){

      update_global_stake(account, value);
      update_delegatee(receiver, value);



}
[[eosio::on_notify("cpumintofeos::undelegate")]] void cpupayouteos::setundelgate(name account, name receiver, asset value){

      update_global_stake(account, -value);
      update_delegatee(receiver, -value);

}


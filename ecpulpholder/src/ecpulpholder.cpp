//compiled with CDT v1.6.2

#include <ecpulpholder.hpp>

void ecpulpholder::setproxy(name proxy, name sender){
   
   require_auth(get_self());
   
   proxies to_proxy( get_self(), get_self().value );
   auto to = to_proxy.find( get_self().value );
   if( to == to_proxy.end() ) {
      to_proxy.emplace( get_self(), [&]( auto& a ){
        a.contract = get_self();
        a.proxy = proxy;
        a.eossender = sender;
      });
   } else {
      to_proxy.modify( to, get_self(), [&]( auto& a ) {
        a.proxy = proxy;
        a.eossender = sender;
      });
   }

}

void ecpulpholder::setpool(asset resevoir, asset eospool, asset lastpay){//initialization and manual reset

require_auth(get_self());
   
   poolstats pstatstable( get_self(), get_self().value );
   auto to = pstatstable.find( get_self().value );
   if( to == pstatstable.end() ) {
      pstatstable.emplace( get_self(), [&]( auto& a ){
        
        a.contract = get_self();
        a.resevoir = lastpay;
        a.eospool = eospool;
        a.lastdeposit = current_time_point().sec_since_epoch();
        a.lastdailypay =  lastpay;

      });
      name proxy = get_proxy(name{"ecpulpholder"});//get voter proxy account

      action(permission_level{_self, "active"_n}, "eosio"_n, "voteproducer"_n, 
               std::make_tuple(get_self(), proxy, name{""})).send();

      action(permission_level{_self, "active"_n}, "eosio"_n, "deposit"_n, 
               std::make_tuple(get_self(), eospool)).send();
   }
   else{

      pstatstable.modify( to, get_self(), [&]( auto& a ) {
       
        a.resevoir = resevoir;
        a.eospool = eospool;
        a.lastdeposit = current_time_point().sec_since_epoch();
        a.lastdailypay =  lastpay;
      
      });

      
      

   }
}

[[eosio::on_notify("cpumintofeos::delegate")]] 
void ecpulpholder::setdelegate(name account, name receiver, asset value){

     //if delegating in middle of round, send corresponding proportion from liquid resevoir and powerup account immediately

     
     asset powerup = asset(0, symbol("EOS", 4)); //initiaize powerup with eos asset, change amount in next step
     
     asset lastpay = get_last_daily_pay();
     
     asset ecpusupply;
     asset resevoir;

    
     ecpusupply = get_supply(name{"cpumintofeos"},asset(0, symbol("ECPU", 8)).symbol.code());
     ecpusupply = ecpusupply + asset(48000000000, symbol("ECPU", 8)); // find total supply of ECPU one day from now (adjustment needed or else newly minted ECPU being delegated can make resevoir go bankrupt)
     resevoir = get_resevoir();

     
         
     powerup = (lastpay*double(value.amount)/(double(ecpusupply.amount))/2);//get half of alloted ECPU for day and send

     check(powerup.amount !=0, "ERROR Delegated ECPU amount corresponding to CPU/NET Rental is 0.0000 EOS, try delegating more ECPU");
     check(powerup.amount >= 5, "Delegated ECPU amount corresponding to CPU/NET Rental must be greater than or equal to than 0.0005 EOS, try delegating more ECPU");
    
     add_resevoir(-powerup);// update resevoir by removing this individual powerup
    
     action(permission_level{_self, "active"_n}, "eosio.token"_n, "transfer"_n, 
           std::make_tuple(get_self(), name{"powerupcalc1"}, powerup,receiver.to_string())).send();
     
}

[[eosio::on_notify("eosio.token::transfer")]]
void ecpulpholder::deposit(name from, name to, eosio::asset quantity, std::string memo){
//two types of eos being received, mining income, and voting income
      //-mining incoming EOS sent to be locked in REX forever, sends once an hour from cpumintofeos
      //-staking incoming EOS used for powerups proportional to ECPU staked, the EOS corresponding to unstaked ECPU will be kept in resevoir
         //untill next payment from rewards proxy
   
   //standard cases to ignore below
   if (to != get_self()){
        return;
   }

   if (from == get_self()){
        return;
   }

   if (memo == "refill"){
        return;
   }
   
   name proxy = get_proxy(name{"ecpulpholder"});//get voter proxy account
   name proxy_sender = get_eossender(name{"ecpulpholder"});//get expected reward sending-account


   if(from == proxy_sender){//in the case of receiving voting rewards from the proxy's reward sending account
         
         //upon payment of vote rewards, place all current liquid eos (previous resevoir see below) into REX permanently 
         asset resevoir = get_resevoir();//get powerup pool reserved for undelegated ECPU

         action(permission_level{_self, "active"_n}, "eosio"_n, "voteproducer"_n, 
               std::make_tuple(get_self(), proxy, name{""})).send();

         if (resevoir.amount != 0){

                  action(permission_level{_self, "active"_n}, "eosio"_n, "deposit"_n, 
                     std::make_tuple(get_self(), resevoir)).send();
         }

         add_to_eospool(resevoir); // move resevoir to permanent pool 
         
         clear_resevoir(); //clear resevoir

         set_last_daily_pay(quantity); // set last received reward 

         // send stake/supply* received to iteration contract
         asset ecpusupply =get_supply(name{"cpumintofeos"},asset(0, symbol("ECPU", 8)).symbol.code()); //find total supply of ECPU 
         ecpusupply = ecpusupply + asset(48000000000, symbol("ECPU", 8)); // find total supply of ECPU one day from now (adjustment needed or else newly minted ECPU being delegated can make resevoir go bankrupt)
         asset ecpu_dstake = get_ecpu_delstake(name{"cpumintofeos"},asset(0, symbol("ECPU", 8)).symbol.code());//find total stake of ECPU
         
         
         asset powerup = quantity; //initialization 
         powerup = quantity*(double(ecpu_dstake.amount))/(double(ecpusupply.amount)); //find fraction of staked ECPU, unstaked ECPU allocation will be auto reinvested
         
         check(ecpu_dstake < ecpusupply, "error stake shall always be < or equal to supply");// sanity check
         check(powerup < quantity, "error powerup amount shall always be < than received quantity"); // sanity check
         
         resevoir = quantity - powerup; //liquid eos representing unstaked ECPU, will await in balance untill next cpu payment

         add_resevoir(resevoir);// update resevoir

         if (powerup.amount != 0){
        
               action(permission_level{_self, "active"_n}, "eosio.token"_n, "transfer"_n, 
                     std::make_tuple(get_self(), name{"cpupayouteos"}, powerup, std::string(""))).send();
         } 
      }
   
   else if(memo == ""){ //no memo is for permanent burn case, locked into REX forever

  
               
      action(permission_level{_self, "active"_n}, "eosio"_n, "voteproducer"_n, 
      std::make_tuple(get_self(), proxy, name{""})).send();

      //check(1!=1, "code got here  (line 153)");

      action(permission_level{_self, "active"_n}, "eosio"_n, "deposit"_n, 
      std::make_tuple(get_self(), quantity)).send();

      add_to_eospool(quantity);
         
   }
   
   else{ 

         check(1!=1,"memo not valid, send with no memo to lock into REX forver");
   

   return;
   }
}

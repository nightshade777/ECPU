//compiled with CDT v1.6.2

#include <ecpulpholder.hpp>

void ecpulpholder::setproxy( const name& contract, name proxy, name sender){
   
   require_auth(get_self());
   
   proxies to_proxy( get_self(), get_self().value );
   auto to = to_proxy.find( contract.value );
   if( to == to_proxy.end() ) {
      to_proxy.emplace( get_self(), [&]( auto& a ){
        a.contract = contract;
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


void ecpulpholder::setpool( asset resevoir, asset rex_queue, asset eospool, asset lastpay){//initialization and manual reset


require_auth(get_self());
   
   poolstats pstatstable( get_self(), get_self().value );
   auto to = pstatstable.find( get_self().value );
   if( to == pstatstable.end() ) {
      pstatstable.emplace( get_self(), [&]( auto& a ){
        
        a.resevoir = asset(0, symbol("EOS", 4));
        a.rex_queue = asset(0, symbol("EOS", 4));
        a.eospool = asset(0, symbol("EOS", 4));
        a.lastdeposit = current_time_point().sec_since_epoch();

        a.lastdailypay =  asset(0, symbol("EOS", 4));

      });
   }
   else{

      pstatstable.modify( to, get_self(), [&]( auto& a ) {
       
        a.resevoir = resevoir;
        a.rex_queue = rex_queue;
        a.eospool = eospool;
        a.lastdeposit = current_time_point().sec_since_epoch();
        a.lastdailypay =  lastpay;
      
      });
   }
}

[[eosio::on_notify("cpumintofeos::delegate")]] 
void ecpulpholder::setdelegate(name account, asset receiver, asset value){

     //if delegating in middle of round, send corresponding proportion from liquid resevoir and powerup account immediately

     asset powerup = asset(0, symbol("EOS", 4)); //initiaize powerup with eos asset, change amount in next step
     asset lastpay = get_last_daily_pay();
     asset ecpusupply;
     asset resevoir;

    
     ecpusupply = get_supply(name{"cpumintofeos"},asset(0, symbol("ECPU", 4)).symbol.code());
     resevoir = get_resevoir();
         
     powerup.amount = ((double(value.amount))/(double(ecpusupply.amount))/2)*lastpay.amount;//get half of alloted ECPU for day and send

    //remove this powerup amout from resevoir

     add_resevoir(-powerup);// update resevoir

     action(permission_level{_self, "active"_n}, "eosio.token"_n, "transfer"_n, 
            std::make_tuple(get_self(), name{"powerupcalc1"}, powerup,receiver.to_string())).send();
     
}

[[eosio::on_notify("eosio.token::transfer")]]
void ecpulpholder::deposit(name from, name to, eosio::asset quantity, std::string memo){
//three types of eos being received, mining income, voting income, and LP income
   name proxy = get_proxy(name{"ecpulpholder"});//get voter proxy account
   name proxy_sender = get_eossender(name{"ecpulpholder"});//get expected reward sending-account
   asset deposit;

   if (to != get_self()){
        return;
   }

   if (from == get_self()){
        return;
   }

   if (memo == "refill"){
        return;
   }
   
   if(memo == "mine income for permanent pool"){

         //add to rex_queue, if 1 hour has passed since last addition to REX from mining pool income, then send to rex
         add_rex_queue(quantity);
         if (current_time_point().sec_since_epoch() >= (get_last_rexqueue()+(60*60))){

               deposit = get_rex_queue();
               
               action(permission_level{_self, "active"_n}, "eosio"_n, "voteproducer"_n, 
               std::make_tuple(get_self(), proxy, name{""})).send();

               action(permission_level{_self, "active"_n}, "eosio"_n, "deposit"_n, 
               std::make_tuple(get_self(), deposit)).send();

               add_queue_to_eospool();
         }

         return;
         
   }
   else if(from == proxy_sender){//in the case of receiving voting rewards from the proxy's reward sending account
   //upon payment of vote rewards, place all current liquid eos (previous resevoir see below) into REX permanently 
         asset resevoir = get_resevoir();//get powerup pool reserved for undelegated ECPU

         action(permission_level{_self, "active"_n}, "eosio"_n, "voteproducer"_n, 
               std::make_tuple(get_self(), proxy, name{""})).send();

         action(permission_level{_self, "active"_n}, "eosio"_n, "deposit"_n, 
               std::make_tuple(get_self(), resevoir)).send();
         
         void clear_resevoir();
         set_last_daily_pay(quantity);

   // send stake/supply* received to iteration contract
         asset ecpusupply =get_supply(name{"cpumintofeos"},asset(0, symbol("ECPU", 4)).symbol.code()); //find total supply of ECPU 
         asset ecpustake = get_ecpustake(name{"cpumintofeos"},asset(0, symbol("ECPU", 4)).symbol.code());//find total stake of ECPU
         asset powerup = quantity; //initialization 
         powerup = (double(ecpustake.amount))/(double(ecpusupply.amount))*quantity; //find fraction of staked ECPU, unstaked ECPU allocation will be auto reinvested
         check(ecpustake < ecpusupply, "error stake shall always be < or equal to supply");// sanity check
         check(powerup < quantity, "error powerup amount shall always be < than received quantity"); // sanity check
         resevoir = quantity - powerup; //liquid eos representing unstaked ECPU, will await in balance untill next cpu payment
         
         add_resevoir(resevoir);// update resevoir

         action(permission_level{_self, "active"_n}, "eosio.token"_n, "transfer"_n, 
               std::make_tuple(get_self(), name{"cpupayouteos"}, powerup, std::string(""))).send();
   }
   else{ 

         check(1!=1,"memo not valid");
   

   return;}
   


    
    
    



}

#include <ecpuvotereos.hpp>

ACTION ecpuvotereos::regproxy(name user, name proxy, name proxysender) {
  require_auth(user);

  //add proxy and associated proxy rewards sending account to table

 
}

ACTION ecpuvotereos::vote(name user, asset amount, name proxy) {
  require_auth(user);

  //use ecpu to to vote

  voters_table votertable(get_self(), user.value);
  auto it = votertable.find(user.value);
  
  if (it == votertable.end()){

          votertable.emplace( user, [&]( auto& a ){
                a.user = user;
                a.delegated = amount;
                a.proxy = proxy;
          });
  }
      //if proxy is the same, 
    else if ((it->proxy) == proxy){
          
          votertable.modify(it, same_payer, [&]( auto& a ){ //add votes to existing proxy
                a.delegated += amount;
          });

          //add to proxt table
    }

    else {

          votertable.modify(it, same_payer, [&]( auto& a ){ //remove votes from exisitng proxy and add votes here
                a.proxy = proxy;
                a.delegated = amount;
          });

          //remove votes from proxy table and add to new prox table
    }


  
}

[[eosio::on_notify("cpumintofeos::transfer")]] void ecpuvotereos::ecputransfer(name from, name to, asset quantity, std::string memo){

      //find if from is a voter, else return
      //check if votes < current balance, if so remove votes to equal balance 


}



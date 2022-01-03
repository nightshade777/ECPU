#include <ecpuvotereos.hpp>

ACTION ecpuvotereos::regproxy(name user, name proxy, name proxysender) {
  require_auth(get_self());

  //add proxy and associated proxy rewards sending account to table
  proxies_table proxytable(get_self(), get_self().value);

  auto proxy_it = proxytable.find(proxy.value);

  check (proxy_it == proxytable.end(), "Error proxy account already exists");


      proxytable.emplace( user, [&]( auto& a ){
                a.proxy = proxy;
                a.proxysender = proxysender;
                a.delegated = asset(0, symbol("ECPU", 8));
      });

 
}

ACTION ecpuvotereos::vote(name user, asset quantity, name proxy) {
  require_auth(user);


  //check if proxy is valid, (if exists)
  proxies_table proxytable(get_self(), get_self().value);
  auto proxy_it = proxytable.find(proxy.value);
  check (proxy_it != proxytable.end(), "Error proxy account doesn't exist");


  //check if user balance is atleast quantity
   asset ecpu_balance = get_balance(name{"cpumintofeos"}, user, symbol_code("ECPU"));
   check (quantity.amount < ecpu_balance.amount, "Cannot vote with more than balance");

  

  voters_table votertable(get_self(), user.value);
  auto voter_it = votertable.find(user.value);
  
  if (voter_it == votertable.end()){

          votertable.emplace( user, [&]( auto& a ){
                a.user = user;
                a.delegated = quantity;
                a.proxy = proxy;
          });

          proxytable.modify(proxy_it, same_payer, [&]( auto& a ){

              a.delegated += quantity; 

          });
  }
      //if proxy is the same, 
    else if ((voter_it->proxy) == proxy){

          //check if (userbalance - existing votes) is atleast balance
          
          votertable.modify(voter_it, same_payer, [&]( auto& a ){ //add votes to existing proxy
                check((ecpu_balance.amount - a.delegated.amount)>= quantity.amount, "not enough ECPU to add this number of votes");
                a.delegated += quantity;
          });

          //add to proxt table
    }

    else {

          //find the proxy user is currently voting for: it->proxy
          //remove votes from that proxy equal to what is currently in user's votertable
          //add "quantity" number of votes to new proxy
          auto oldproxy_it = proxytable.find((voter_it->proxy).value);
          
          proxytable.modify(oldproxy_it, same_payer, [&]( auto& a ){

              a.delegated -= (voter_it->delegated); 

          });

          proxytable.modify(proxy_it, same_payer, [&]( auto& a ){

              a.delegated += quantity; 

          });
          

          votertable.modify(voter_it, same_payer, [&]( auto& a ){ //remove votes from exisitng proxy and add votes here
                a.proxy = proxy;
                a.delegated = quantity;
          });

          //remove votes from proxy table and add to new prox table
    }


    action(permission_level{_self, "active"_n}, "ecpuvotereos"_n, "setwinner"_n, 
            std::make_tuple()).send();


  
}


ACTION ecpuvotereos::setwinner(){

    require_auth(get_self());

    name proxy = find_winning_proxy();
    name proxysender = get_proxy_sender(proxy);


    action(permission_level{_self, "active"_n}, "ecpulpholder"_n, "setproxy"_n, 
            std::make_tuple(proxy,proxysender)).send();


    


}

[[eosio::on_notify("cpumintofeos::transfer")]] void ecpuvotereos::ecputransfer(name from, name to, asset quantity, std::string memo){

      //find if from is a voter, else return
      //check if votes < current balance, if so remove votes to equal balance 

      asset ecpu_balance = get_balance(name{"cpumintofeos"}, from, symbol_code("ECPU"));

      voters_table votertable(get_self(), from.value);
      auto it = votertable.find(from.value);

      if (it == votertable.end()){
          return;
      }


      if((it->delegated).amount > ecpu_balance.amount){

          votertable.modify(it, same_payer, [&]( auto& a ){ //add votes to existing proxy
                a.delegated = ecpu_balance;
          });

      }


}



#include <ecpuvotereos.hpp>

ACTION ecpuvotereos::regproxy(name user, name proxy, name proxysender) {
  require_auth(get_self());

  //add proxy and associated proxy rewards sending-account to table
  proxies_table proxytable(get_self(), get_self().value);

  auto proxy_it = proxytable.find(proxy.value);

  check (proxy_it == proxytable.end(), "Error proxy account already exists");


      proxytable.emplace( user, [&]( auto& a ){
                a.proxy = proxy;
                a.proxysender = proxysender;
                a.delegated = asset(0, symbol("ECPU", 8));
      });

 
}

ACTION ecpuvotereos::voteproxy(name user, name proxy) {
  require_auth(user);
  //Three cases:
  //1. User has never voted before
  //2. User has voted before and is refreshing votes for same proxy
  //3. User has voted before and is switching to new proxy


  //check if proxy is valid, (if exists)
  proxies_table proxytable(get_self(), get_self().value);
  auto proxy_it = proxytable.find(proxy.value);
  check (proxy_it != proxytable.end(), "Error proxy account doesn't exist");


  //check if user balance is atleast quantity
   asset ecpu_balance = get_balance(name{"cpumintofeos"}, user, symbol_code("ECPU"));
   asset prev_delegated;
   asset delta;
   //check (quantity.amount < ecpu_balance.amount, "Cannot vote with more than balance");



  voters_table votertable(get_self(), user.value);
  auto voter_it = votertable.find(user.value);
  
  if (voter_it == votertable.end()){

          votertable.emplace( user, [&]( auto& a ){
                a.user = user;
                a.delegated = ecpu_balance;
                a.proxy = proxy;
          });

          proxytable.modify(proxy_it, same_payer, [&]( auto& a ){

              a.delegated += ecpu_balance; 

          });
  }
      //if proxy is the same, 
    else if ((voter_it->proxy) == proxy){

          //check if (userbalance - existing votes) is atleast balance
          
          votertable.modify(voter_it, same_payer, [&]( auto& a ){ //refresh votes to existing proxy
                
                prev_delegated = a.delegated;
                delta = ecpu_balance - prev_delegated;

                a.delegated = ecpu_balance;
          });

          proxytable.modify(proxy_it, same_payer, [&]( auto& a ){

              a.delegated += delta; 

          });

          //add to proxy table
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

              a.delegated += ecpu_balance; 

          });
          

          votertable.modify(voter_it, same_payer, [&]( auto& a ){ //remove votes from exisitng proxy and add votes here
                a.proxy = proxy;
                a.delegated = ecpu_balance;
          });

          //remove votes from proxy table and add to new prox table
    }


    name proxy_w = find_winning_proxy();
    name proxysender = get_proxy_sender(proxy_w);
    
    action(permission_level{_self, "active"_n}, "ecpuvotereos"_n, "updateproxy"_n, 
            std::make_tuple(proxy_w,proxysender)).send();


  
}


ACTION ecpuvotereos::updateproxy(name proxy, name proxysender){
        
        require_auth(get_self());
        require_recipient(name{"ecpulpholder"});

}

ACTION ecpuvotereos::voteinf(name user, int mine, int stake){

        require_auth(user);
        asset ecpu_balance = get_balance(name{"cpumintofeos"}, user, symbol_code("ECPU"));

        check( (stake + mine) == 100, "Vote proportions must equal 100%");
        check(stake >0 && mine>0, "Vote ratios must e bigger than zero");   

        update_voting_record(user, mine, stake);
        update_weighted_totals();


}

[[eosio::on_notify("cpumintofeos::transfer")]] void ecpuvotereos::ecputransfer(name from, name to, asset quantity, std::string memo){


      //find if from is a voter, else return
      //check if votes < current balance, if so remove votes to equal balance 

      asset ecpu_balance = get_balance(name{"cpumintofeos"}, from, symbol_code("ECPU"));

      voters_table votertable(get_self(), from.value);
      proxies_table proxytable(get_self(), get_self().value);

      auto it = votertable.find(from.value);

      if (it == votertable.end()){ //account has not voted yet
          return;
      }


      if((it->delegated).amount > ecpu_balance.amount){

          //1-find which proxy account voted for
          //2-remove those votes from that proxy table
          //3-remove votes from voter table

          
          auto proxy_it = proxytable.find((it->proxy).value);

          proxytable.modify(proxy_it, same_payer, [&]( auto& a ){

              a.delegated = a.delegated - (it->delegated) + ecpu_balance;//remove delta

          });
          
          
          votertable.modify(it, same_payer, [&]( auto& a ){ //votes delegated to be reduced to current amount 
                
                a.delegated = ecpu_balance;

          });

      }
      name proxy_w = find_winning_proxy();
      name proxysender = get_proxy_sender(proxy_w);

      //inflation voting, only activates if the respective parties to & from have voted
      remove_votes(from, quantity);
      add_votes(to, quantity);
      update_weighted_totals();
    
      action(permission_level{_self, "active"_n}, "ecpuvotereos"_n, "updateproxy"_n, 
            std::make_tuple(proxy_w,proxysender)).send();

}



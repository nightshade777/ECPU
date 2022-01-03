#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>

using namespace std;
using namespace eosio;

CONTRACT ecpuvotereos : public contract {
  public:
    using contract::contract;

    ACTION regproxy(name user, name proxy, name proxysender); //add proxy and associated proxy rewards sending account to proxies table

    ACTION vote(name user, asset amount, name proxy);

    ACTION setwinner();

     [[eosio::on_notify("cpumintofeos::transfer")]] void ecputransfer(name from, name to, asset quantity, std::string memo);

     static asset get_balance( const name& token_contract_account, const name& owner, const symbol_code& sym_code )
         {
            accounts accountstable( token_contract_account, owner.value );
            const auto& ac = accountstable.get( sym_code.raw() );
            return ac.balance;
         }

      name find_winning_proxy(){

            asset largest_delegation = asset(0, symbol("ECPU", 8)); //initialization
            name winning_proxy;

            proxies_table proxytable(get_self(), get_self().value);

             for(auto it = proxytable.begin(); it != proxytable.end(); it++){

                if ((it->delegated).amount > largest_delegation.amount){

                    largest_delegation = (it->delegated);
                    winning_proxy = (it->proxy);

                }


             }
            return winning_proxy;


      }

      name get_proxy_sender(name proxy){

          proxies_table proxytable(get_self(), get_self().value);
          auto it = proxytable.find(proxy.value);

          return (it->proxysender);

      }

  private:
    
    struct [[eosio::table]] proxies { //scope contract, contract
      name    proxy; 
      name    proxysender; // account which the proxy sends voter rewards from
      asset   delegated; //number of ECPU votes

      auto primary_key() const { return proxy.value; }
    };
    typedef multi_index<name("proxies"), proxies> proxies_table;

private:

    struct [[eosio::table]] voters {
      
      name user;
      asset delegated;
      name proxy; 

      auto primary_key() const { return user.value; }
    };
    typedef multi_index<name("voters"), voters> voters_table;

    
    struct [[eosio::table]] account {
            asset    balance;
            asset    delegated;  //balance of tokens able to be delegated, max is the number of tokens staked, converted to cpupower upon delegation
                                   //(can be thought of as amount of staked tokens which have not been delegated yet)
            asset    undelegating; //ECPU in process of clearing undelegation
            asset    cpupower;     //ECPU tokens delegated to bal (includes delegated from others)
                                   //(can be thought of as the sum of all ECPU delegated to this accoount)
           
           

            uint64_t primary_key()const { return balance.symbol.code().raw(); }
         };
    typedef eosio::multi_index< "accounts"_n, account > accounts;
    

   
};

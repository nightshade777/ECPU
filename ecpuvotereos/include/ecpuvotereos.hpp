#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>

using namespace std;
using namespace eosio;

CONTRACT ecpuvotereos : public contract {
  public:
    using contract::contract;

    ACTION regproxy(name user, name proxy, name proxysender); //add proxy and associated proxy rewards sending account to proxies table

    ACTION vote(name user, asset amount, name proxy);

     [[eosio::on_notify("cpumintofeos::transfer")]] void ecputransfer(name from, name to, asset quantity, std::string memo);

  private:
    TABLE proxies { //scope contract, contract
      name    proxy; 
      name    proxysender; // account which the proxy sends voter rewards from
      asset   delegated; //number of ECPU votes

      auto primary_key() const { return proxy.value; }
    };
    typedef multi_index<name("proxies"), proxies> proxies_table;
private:

    TABLE voters {
      name user;
      asset delegated;
      name proxy; 

      auto primary_key() const { return user.value; }
    };
    typedef multi_index<name("voters"), voters> voters_table;

   
};

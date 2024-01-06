#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>

using namespace std;
using namespace eosio;

CONTRACT ecpuvotereos : public contract {
  public:
    using contract::contract;

    ACTION regproxy(name user, name proxy, name proxysender); //add proxy and associated proxy rewards sending account to proxies table

    ACTION voteproxy(name user, name proxy);

    ACTION updateproxy(name proxy, name proxysender);

    ACTION voteinf(name user, int mine, int stake);
    

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


      //INFLATION VOTING for MINING VERSUS STAKING

      void update_voting_record(name user, int mine_votes, int stake_votes) {
        infvoters_table voters(get_self(), get_self().value);
        auto voter_itr = voters.find(user.value);

        // Get ECPU balance of the user
        asset ecpu_balance = get_balance("cpumintofeos"_n, user, symbol_code("ECPU"));

        if (voter_itr == voters.end()) {
            // Emplace new voting record
            voters.emplace(get_self(), [&](auto& new_vote) {
                new_vote.user = user;
                new_vote.mine = mine_votes;
                new_vote.stake = stake_votes;
                new_vote.votes = ecpu_balance; // Set the votes to ECPU balance
            });
        } else {
            // Modify existing voting record
            voters.modify(voter_itr, get_self(), [&](auto& existing_vote) {
                existing_vote.mine = mine_votes;
                existing_vote.stake = stake_votes;
                existing_vote.votes = ecpu_balance; // Update the votes to ECPU balance
            });
        }
      }
      void add_votes(name user, asset amount) {
        

        // Check if the asset symbol is correct
        check(amount.symbol == symbol("ECPU", 4), "Amount must be in ECPU");

        infvoters_table voters(get_self(), get_self().value);
        auto voter_itr = voters.find(user.value);
        if(voter_itr != voters.end()){
            return;
        }

        // Add votes
        voters.modify(voter_itr, get_self(), [&](auto& record) {
            record.votes += amount;
        });
      }
    
      void remove_votes(name user, asset amount) {
        

        // Check if the asset symbol is correct
        check(amount.symbol == symbol("ECPU", 4), "Amount must be in ECPU");

        infvoters_table voters(get_self(), get_self().value);
        auto voter_itr = voters.find(user.value);
        if(voter_itr != voters.end()){
            return;
        }
        // Ensure there are enough votes to remove
        check(voter_itr->votes.amount >= amount.amount, "Insufficient votes to remove");

        // Remove votes
        voters.modify(voter_itr, get_self(), [&](auto& record) {
            record.votes -= amount;
        });
      }

      void update_weighted_totals() {
        infvoters_table voters(get_self(), get_self().value);
        total_weighted_table totals(get_self(), get_self().value);

        int64_t total_weighted_mine = 0;
        int64_t total_weighted_stake = 0;

        // Calculate weighted totals
        for (auto& voter : voters) {
            double mine_ratio = static_cast<double>(voter.mine) / (voter.mine + voter.stake);
            double stake_ratio = static_cast<double>(voter.stake) / (voter.mine + voter.stake);

            total_weighted_mine += static_cast<int64_t>(mine_ratio * voter.votes.amount);
            total_weighted_stake += static_cast<int64_t>(stake_ratio * voter.votes.amount);
        }

        // Check if the entry exists, if not create one, else modify
        auto itr = totals.find(get_self().value);
        if (itr == totals.end()) {
        // Create a new entry
            totals.emplace(get_self(), [&](auto& row) {
                row.contract_name = get_self();
                row.weighted_mine = total_weighted_mine;
                row.weighted_stake = total_weighted_stake;
            });
        } 
        else {
        // Modify existing entry
            totals.modify(itr, get_self(), [&](auto& row) {
                row.weighted_mine = total_weighted_mine;
                row.weighted_stake = total_weighted_stake;
            });
        }
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


    struct [[eosio::table]] infvoters {
      
      name user;
      int mine;
      int stake;

      asset votes;
      

      auto primary_key() const { return user.value; }
    };
    typedef multi_index<name("infvoters"), infvoters> infvoters_table;

    struct [[eosio::table]] total_weighted {
        
        name contract_name; // Fixed primary key
        int64_t weighted_mine;
        int64_t weighted_stake;

        uint64_t primary_key() const { return contract_name.value; }
    };
    typedef eosio::multi_index<"totalweight"_n, total_weighted> total_weighted_table;

    //from cpumintofeos:
    
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

#include <cpumintofeos.hpp>

namespace eosio {

void token::create( const name&   issuer,
                    const asset&  maximum_supply )
{
    require_auth( get_self() );

    auto sym = maximum_supply.symbol;
    check( sym.is_valid(), "invalid symbol name" );
    check( maximum_supply.is_valid(), "invalid supply");
    check( maximum_supply.amount > 0, "max-supply must be positive");

    stats statstable( get_self(), sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    check( existing == statstable.end(), "token with symbol already exists" );

    statstable.emplace( get_self(), [&]( auto& s ) {
       s.supply.symbol = maximum_supply.symbol;
       s.max_supply    = maximum_supply;
       s.issuer        = issuer;
       s.creationtime  = current_time_point().sec_since_epoch();
       s.prevmine = current_time_point().sec_since_epoch();
       s.lastdeposit = current_time_point().sec_since_epoch();
       s.totalstake =  asset(0, symbol("ECPU", 4));//initialize asset to zero
       s.totaldelegate =  asset(0, symbol("ECPU", 4));//initialize asset to zero
       
    });
}


void token::issue( const name& to, const asset& quantity, const string& memo )
{
    auto sym = quantity.symbol;
    check( sym.is_valid(), "invalid symbol name" );
    check( memo.size() <= 256, "memo has more than 256 bytes" );

    stats statstable( get_self(), sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    check( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
    const auto& st = *existing;
    check( to == st.issuer, "tokens can only be issued to issuer account" );
    
    require_auth( st.issuer); 
    
    
    check( quantity.is_valid(), "invalid quantity" );
    check( quantity.amount > 0, "must issue positive quantity" );

    check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    check( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

    statstable.modify( st, same_payer, [&]( auto& s ) {
       s.supply += quantity;
    });

    add_balance( st.issuer, quantity, st.issuer );
}

void token::retire( const asset& quantity, const string& memo )
{
    auto sym = quantity.symbol;
    check( sym.is_valid(), "invalid symbol name" );
    check( memo.size() <= 256, "memo has more than 256 bytes" );

    stats statstable( get_self(), sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    check( existing != statstable.end(), "token with symbol does not exist" );
    const auto& st = *existing;

    require_auth( st.issuer );
    check( quantity.is_valid(), "invalid quantity" );
    check( quantity.amount > 0, "must retire positive quantity" );

    check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

    statstable.modify( st, same_payer, [&]( auto& s ) {
       s.supply -= quantity;
    });

    sub_balance( st.issuer, quantity );
}

void token::transfer( const name&    from,
                      const name&    to,
                      const asset&   quantity,
                      const string&  memo )
{
    check( from != to, "cannot transfer to self" );
    require_auth( from );
    check( is_account( to ), "to account does not exist");
    auto sym = quantity.symbol.code();
    stats statstable( get_self(), sym.raw() );
    const auto& st = statstable.get( sym.raw() );

    require_recipient( from );
    require_recipient( to );

    check( quantity.is_valid(), "invalid quantity" );
    check( quantity.amount > 0, "must transfer positive quantity" );
    check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    check( memo.size() <= 256, "memo has more than 256 bytes" );

    auto payer = has_auth( to ) ? to : from;

    sub_balance( from, quantity );
    add_balance( to, quantity, payer );

}

void token::sub_balance( const name& owner, const asset& value ) {
   
      /**
    this function has been modified from eosio.token to implement staking system
    it blocks transacting with stored balance and balances that are undergoing the three day clear period
    **/
    uint32_t time_now = current_time_point().sec_since_epoch();
    uint32_t one_day_time = 60*60*24;//1 days in seconds
   
   
   //WHITELIST FOR INTSTA UNSTAKE REQURED FOR CHINTAI LENDING INTEGRATION:
   // if (owner == name{"chintailease"}){
    //      one_day_time = 0;
    //}
    
    
    accounts from_acnts( _self, owner.value );

    const auto& from = from_acnts.get( value.symbol.code().raw(), "no balance object found" );
    check( from.balance.amount >= value.amount, "overdrawn balance" );

    from_acnts.modify( from, owner, [&]( auto& a ) 
      {
         
        if(time_now<(a.unstake_time+one_day_time)) //checks if anything is in process of unstaking, will stop transfer of tokens in process of unstaking
          {
            uint32_t time_left = round(((a.unstake_time + one_day_time) - time_now)/60); // for error msg display only
            std::string errormsg = "Cannot send staked tokens. Tokens in withdraw will be availible in " + std::to_string(time_left) + " minute(s)";
            check(value <= (a.balance-(a.unstaking+a.storebalance)),errormsg);
          }
       
        else 
          {
           check(value <= (a.balance-a.storebalance),"Cannot send staked tokens");//there are no funds in process of unstaking, only stop transfer of stored funds
          }
         
         a.balance -= value;
      });

}

void token::add_balance( const name& owner, const asset& value, const name& ram_payer )
{
   accounts to_acnts( get_self(), owner.value );
   auto to = to_acnts.find( value.symbol.code().raw() );
   if( to == to_acnts.end() ) {
      to_acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = value;
        a.storebalance = asset(0, symbol("ECPU", 4)); //initialiazation without needing to specify asset symbol or precision   
        a.unstaking = asset(0, symbol("ECPU", 4));     
        a.unstake_time = 0; 
        a.cpupower = asset(0, symbol("ECPU", 4));
        
      });
   } else {
      to_acnts.modify( to, same_payer, [&]( auto& a ) {
        a.balance += value;
      });
   }
}

void token::open( const name& owner, const symbol& symbol, const name& ram_payer )
{
   require_auth( ram_payer );

   check( is_account( owner ), "owner account does not exist" );

   auto sym_code_raw = symbol.code().raw();
   stats statstable( get_self(), sym_code_raw );
   const auto& st = statstable.get( sym_code_raw, "symbol does not exist" );
   check( st.supply.symbol == symbol, "symbol precision mismatch" );

   accounts acnts( get_self(), owner.value );
   auto it = acnts.find( sym_code_raw );
   if( it == acnts.end() ) {
      acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = asset{0, symbol};
        a.storebalance = a.balance;
        a.cpupower = a.balance;    
        a.unstaking = a.balance;     
        a.unstake_time = 0;
      });
   }
}
//simple version of open with only username as argument, user pays for ram
void token::open2(name user)
{
        require_auth(user);

        asset cpu = asset(0, symbol("ECPU", 4));
       
        auto sym = cpu.symbol.code();

        auto sym_code_raw = sym.raw();

        accounts acnts( get_self(), user.value );
        auto it = acnts.find( sym_code_raw );
        if( it == acnts.end() ) {
            acnts.emplace( user, [&]( auto& a ){
                a.balance = cpu;
                a.storebalance = a.balance;
                a.cpupower = a.balance;    
                a.unstaking = a.balance;     
                a.unstake_time = 0;
            });
        }

        
    }

//simple version of open with only username and recipient as argument, user pays for ram
void token::open3(name user, name recipient)
{
        require_auth(user);

        asset cpu = asset(0, symbol("CPU", 4));
       
        auto sym = cpu.symbol.code();

        auto sym_code_raw = sym.raw();

        accounts acnts( get_self(), recipient.value );
        auto it = acnts.find( sym_code_raw );
        if( it == acnts.end() ) {
            acnts.emplace( user, [&]( auto& a ){
                a.balance = cpu;
                a.storebalance = a.balance;
                a.cpupower = a.balance;    
                a.unstaking = a.balance;     
                a.unstake_time = 0;
            });
        }

        
    }

void token::close( const name& owner, const symbol& symbol )
{
   require_auth( owner );
   accounts acnts( get_self(), owner.value );
   auto it = acnts.find( symbol.code().raw() );
   check( it != acnts.end(), "Balance row already deleted or never existed. Action won't have any effect." );
   check( it->balance.amount == 0, "Cannot close because the balance is not zero." );
   check(it->cpupower.amount == 0, "Cannot close because ECPU power is not zero." );
   acnts.erase( it );
}



void token::stake(name account, asset value, bool selfdelegate)
    {
      //stake tokens for 12 hour cpu powerups, only delegated stake, to self or others will receive poweups
      //set self delegate to false if immediately delegating to others

      require_auth(account);
      require_recipient(account);

      require_recipient(name{"cpumintofeos"});
      require_recipient(name{"cpupayouteos"});

    
      auto sym = value.symbol.code();
      stats statstable( _self, sym.raw() );
      const auto& st = statstable.get( sym.raw());

      uint32_t time_now = current_time_point().sec_since_epoch();
      uint32_t one_day_time = 60*60*24;
      
      //WHITELIST FOR INTSTA UNSTAKE REQURED FOR CHINTAI LENDING INTEGRATION:
      //if (account == name{"chintailease"}){
      //   one_day_time = 0;
      //}
      check( value.amount > 0, "must stake positive quantity" );
      check( value.is_valid(), "invalid quantity" );
      check( value.symbol == st.supply.symbol, "symbol precision mismatch" );
      check( value.symbol.is_valid(), "invalid symbol name" );
    
      accounts to_acnts( _self, account.value );
      auto to = to_acnts.find( value.symbol.code().raw() );
    
      to_acnts.modify( to, same_payer, [&]( auto& a )
        {
          eosio::check((value <= (a.balance-a.storebalance) ), "Cannot stake more than your unstaked balance");
          a.storebalance += value;
          a.cpupower += value;
          
          if (a.unstaking <= value){
              a.unstaking = asset(0, symbol("ECPU", 4));
          }
          else{
              a.unstaking = a.unstaking - value;
          }

          if(time_now >= (one_day_time + a.unstake_time)){
              a.unstaking = asset(0, symbol("ECPU", 4));
          }

        });

        updatestake(value);//update global var for tracking stake

        if (selfdelegate == true){

            action(permission_level{get_self(), "active"_n}, "cpumintofeos"_n, "delegate"_n, 
                  std::make_tuple(account,value,value)).send();
        }
    
    }
  
  
  void token::unstake(name account, asset value, bool selfdelegate)
    {

      /**
      unstake tokens, needs one day to unstake
      note: unstaking is a dummy variable and should not be used by frontend apps to display unstaking amount
      is only valid as unstaking amount if current time is less than one day from unstake time
      **/
      require_auth(account);
      require_recipient(account);
      require_recipient(name{"cpumintofeos"});
      require_recipient(name{"cpupayouteos"});
    
      auto sym = value.symbol.code();
      stats statstable( _self, sym.raw() );
      const auto& st = statstable.get( sym.raw() );

      check( value.amount > 0, "must unstake positive quantity" );
      check( value.is_valid(), "invalid quantity" );
      check( value.symbol == st.supply.symbol, "symbol precision mismatch" );
      check( value.symbol.is_valid(), "invalid symbol name" );
    
      uint32_t time_now = current_time_point().sec_since_epoch();
      uint32_t one_day_time = 60*60*24;

      //WHITELIST FOR INTSTA UNSTAKE REQURED FOR CHINTAI LENDING INTEGRATION:
      //if (account == name{"chintailease"}){
      //    one_day_time = 0;
      //}
      
      accounts to_acnts( _self, account.value );
      auto to = to_acnts.find( value.symbol.code().raw() );
   
      to_acnts.modify( to, same_payer, [&]( auto& a ) 
        {
        
          check((value <= a.storebalance  ), "Cannot unstake more than your staked balance");
          a.storebalance -= value;
          a.cpupower -= value;
          check(a.cpupower.amount >= 0, "ECPU Power will become below zero. Must undelegate ECPU before unstake");
        
          if(time_now<= (one_day_time + a.unstake_time))
            {
              a.unstaking += value;
            }
          else
            {
              a.unstaking = value;
            }
          
          a.unstake_time = time_now;
        
        });

        updatestake(-value);

         if (selfdelegate == true){

            action(permission_level{get_self(), "active"_n}, "cpumintofeos"_n, "undelegate"_n, 
                  std::make_tuple(account,value,value)).send();
                  updatedelegate(-value);
        }
    
    }

  void token::destroytoken(std::string symbol) {
    require_auth(get_self());

    symbol_code sym(symbol);
    stats stats_table(get_self(), sym.raw());
    auto existing = stats_table.find(sym.raw());
    check(existing != stats_table.end(), "Token with symbol does not exist");

    stats_table.erase(existing);
  }

 void token::destroyacc(std::string symbol, name account) {
    require_auth(get_self());

    symbol_code sym(symbol);
    accounts accounts_table(get_self(), account.value);
    const auto &row = accounts_table.get(sym.raw(), "No balance object found for provided account and symbol");
    accounts_table.erase(row);
  }
  
  void token::delegate (name account, name receiver, asset value){
      require_auth(account);

      require_recipient(receiver);
      require_recipient(name{"cpupayouteos"});
      //check(receiver != account, "cannot delegate to self");
      auto sym = value.symbol.code();
      stats statstable( _self, sym.raw() );
      const auto& st = statstable.get( sym.raw() );
      check( value.is_valid(), "invalid quantity" );
      check( value.symbol == st.supply.symbol, "symbol precision mismatch" );
      check( value.symbol.is_valid(), "invalid symbol name" );
      check( value.amount > 0, "must enter value greater than zero");
      //remove CPU power from lender
      
      accounts from_acnts( get_self(), account.value );
      auto to = from_acnts.find( value.symbol.code().raw() );
      //check(to!=from_acnts.end()-------- Delegator TO Pay RAM inside DELEGATE ACTIon
      from_acnts.modify( to, same_payer, [&]( auto& a ) 
        {
          check((value <= a.cpupower ), "Cannot delegate more than your cpupower");
          a.cpupower -= value;
        });
      //add CPU power from delegator to receiver/delegatee
      accounts to_acnts( get_self(), receiver.value );
      auto tor = to_acnts.find( value.symbol.code().raw() );

      //if receiver/delegatee has never initialized ram balance of ECPU, the delegator will pay for RAM for ECPU balance
      if( tor == to_acnts.end() ) {
            to_acnts.emplace( account, [&]( auto& a ){
                a.balance = value-value;
                a.storebalance = a.balance;
                a.cpupower = a.balance;    
                a.unstaking = a.balance;     
                a.unstake_time = 0;
            });
        }
      to_acnts.modify( tor, same_payer, [&]( auto& a ) 
        {
          a.cpupower += value;
        });
      
      
      //add borrower to delegate table of lender
      
      delegates delegatetable(get_self(),account.value);
      auto tod = delegatetable.find(receiver.value);
      if (tod == delegatetable.end()){
      //emplace new delegation
          delegatetable.emplace( account, [&]( auto& a ){
                a.recipient = receiver;
                a.cpupower = value;
                a.delegatetime = current_time_point().sec_since_epoch();
          });
      }
      //if delegatee/receiver already exists as a existing receiver for the delegator, add to existing balance
      else {
          delegatetable.modify(tod, same_payer, [&]( auto& a ){
                a.recipient = receiver;
                a.cpupower = a.cpupower + value;
                a.delegatetime = current_time_point().sec_since_epoch();
          });
      }
      
      updatedelegate(value);
      
      
  }
  void token::undelegate (name account, name receiver, asset value){
      
      require_auth(account);
      require_recipient(receiver);
      require_recipient(name{"cpupayouteos"});
      //check(receiver != account, "cannot undelegate to self");
      auto sym = value.symbol.code();
      stats statstable( get_self(), sym.raw() );
      const auto& st = statstable.get( sym.raw() );
      check( value.is_valid(), "invalid quantity" );
      check( value.symbol == st.supply.symbol, "symbol precision mismatch" );
      check( value.symbol.is_valid(), "invalid symbol name" );
      check( value.amount > 0, "must enter value greater than zero");
      asset finalcpupower;
      asset initialcpupower;
      delegates delegatetable(get_self(),account.value);
      auto tod = delegatetable.find(receiver.value);
      check(tod != delegatetable.end(), "cannot undelegate nonexisting delegation");
      delegatetable.modify(tod, same_payer, [&]( auto& a ){
                
            check((a.delegatetime+(60*60*12)) <= current_time_point().sec_since_epoch(),"must wait 12 hours to undelegate");
            initialcpupower = a.cpupower;
            a.cpupower = a.cpupower - value;
            finalcpupower = a.cpupower;

      });

      check(value <= initialcpupower, "cannot undelegate more than delegated amount");
      if (finalcpupower.amount == 0){
          delegatetable.erase(tod);
      }
      //add cpu power to lender
      
      accounts from_acnts( _self, account.value );
      auto to = from_acnts.find( value.symbol.code().raw() );
   
      from_acnts.modify( to, same_payer, [&]( auto& a ) 
        {
          a.cpupower += value;
        });
      
      //remove cpu power from borrower 
      accounts to_acnts( _self, receiver.value );
      auto tor = to_acnts.find( value.symbol.code().raw() );
   
      to_acnts.modify( tor, same_payer, [&]( auto& a ) 
        {
          a.cpupower -= value;
        });
    updatedelegate(-value);
     }
     
void minereceipt( name user){
    //no permissions required, this is simply to iterate the powerup payouts, it will be triggered by this contract when receiving eos
    //from mining but can also be executed from any account as an auxilary help function to iterate the payout contract as well
    require_recipient(name{"cpupayouteos"});
}
    
[[eosio::on_notify("eosio.token::transfer")]]void token::claim(name from, name to, eosio::asset quantity, std::string memo){

   if (to != get_self() || from == get_self()) {

         return;
   }

   action(permission_level{get_self(), "active"_n}, "cpumintofeos"_n, "minereceipt"_n, 
      std::make_tuple(from)).send();

   check(quantity.amount == 100, "Transfer amount to mine must be equal to 0.01 EOS");

   asset currentbal = asset(0.0000, symbol(symbol_code("EOS"),4));
   auto sym = currentbal.symbol.code();
   currentbal = get_balance(name{"eosio.token"}, get_self(), sym);

   if(current_time_point().sec_since_epoch() > (get_last_deposit() + 60*60)){
   
      action(permission_level{_self, "active"_n}, "eosio.token"_n, "transfer"_n, 
      std::make_tuple(get_self(), name{"cpupayouteos"}, currentbal, std::string("mine income for permanent pool"))).send();

   }
   
   mine(from);
  
}



} /// namespace eosio
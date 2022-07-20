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
       s.totaldelegate =  asset(0, symbol("ECPU", 8));//initialize asset to zero
       
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

    require_recipient( name{"ecpuvotereos"});

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
    this function has been modified from eosio.token to implement delegating system
    it blocks transacting with stored balance and balances that are undergoing the three day clear period
    **/
    
   
   
   //WHITELIST FOR INTSTA UNSTAKE REQURED FOR CHINTAI LENDING INTEGRATION:
   // if (owner == name{"chintailease"}){
    //      one_day_time = 0;
    //}
    asset locked_ecpu = get_non_liquid_ecpu(owner);
    
    accounts from_acnts( _self, owner.value );

    const auto& from = from_acnts.get( value.symbol.code().raw(), "no balance object found" );
    check( from.balance.amount >= value.amount, "overdrawn balance" );

    from_acnts.modify( from, owner, [&]( auto& a ) 
      {
        check(value <= (a.balance-(locked_ecpu)), "cannot send delegated tokens nor tokens in process of undelegating");
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
        a.delegated = asset(0, symbol("ECPU", 8));
        a.undelegating = asset(0, symbol("ECPU", 8));
        a.cpupower = asset(0, symbol("ECPU", 8));
      
      });
   } 
   else {
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
        a.delegated = a.balance;
        a.undelegating = a.balance;
        a.cpupower = a.balance;
      });
   }
}
//simple version of open with only username as argument, user pays for ram
void token::open2(name user)
{
        require_auth(user);

        asset cpu = asset(0, symbol("ECPU", 8));
       
        auto sym = cpu.symbol.code();

        auto sym_code_raw = sym.raw();

        accounts acnts( get_self(), user.value );
        auto it = acnts.find( sym_code_raw );
        if( it == acnts.end() ) {
            acnts.emplace( user, [&]( auto& a ){
                a.balance = asset(0, symbol("ECPU", 8));
                a.delegated = asset(0, symbol("ECPU", 8));
                a.undelegating = asset(0, symbol("ECPU", 8));
                a.cpupower = asset(0, symbol("ECPU", 8));
            });
        }

        
    }

//version of open with only username and recipient as argument, user pays for ram
void token::open3(name user, name recipient)
{
        require_auth(user);

        asset cpu = asset(0, symbol("ECPU", 8));
       
        auto sym = cpu.symbol.code();

        auto sym_code_raw = sym.raw();

        accounts acnts( get_self(), recipient.value );
        auto it = acnts.find( sym_code_raw );
        if( it == acnts.end() ) {
            acnts.emplace( user, [&]( auto& a ){
                a.balance = asset(0, symbol("ECPU", 8));
                a.delegated = asset(0, symbol("ECPU", 8));
                a.undelegating = asset(0, symbol("ECPU", 8));
                a.cpupower = asset(0, symbol("ECPU", 8));
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



  
  void token::delegate (name account, name receiver, asset value){
      
      require_auth(account);
      

      require_recipient(receiver);
      require_recipient(name{"ecpulpholder"});
      
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

      asset locked_ecpu = get_non_liquid_ecpu(account);

      

      accounts from_acnts( get_self(), account.value );
      auto to = from_acnts.find( value.symbol.code().raw() );

      
      //check(to!=from_acnts.end()-------- Delegator TO Pay RAM inside DELEGATE ACTIon
      from_acnts.modify( to, same_payer, [&]( auto& a ) 
        {
          check((value <= (a.balance - locked_ecpu) ), "Cannot delegate more than your remaining undelegated staked tokens");
          a.delegated += value;
        });

        
      //add CPU power from delegator to receiver/delegatee
      accounts to_acnts( get_self(), receiver.value );
      auto tor = to_acnts.find( value.symbol.code().raw() );

     
      //if receiver/delegatee has never initialized ram balance of ECPU, the delegator will pay for RAM for ECPU balance
      if( tor == to_acnts.end() ) {
            to_acnts.emplace( account, [&]( auto& a ){
                a.balance = asset(0, symbol("ECPU", 8));
                a.delegated = asset(0, symbol("ECPU", 8));
                a.undelegating = asset(0, symbol("ECPU", 8));
                a.cpupower = value;
            });
        }
      else {
            to_acnts.modify( tor, same_payer, [&]( auto& a ) {
                a.cpupower += value;
            });
      }
      
      
      
      //add borrower to delegate table of lender
      
      delegates delegatetable(get_self(),account.value);
      auto tod = delegatetable.find(receiver.value);
      if (tod == delegatetable.end()){
      //emplace new delegation
          delegatetable.emplace( account, [&]( auto& a ){
                a.recipient = receiver;
                a.cpupower = value;
                a.delegatetime = current_time_point().sec_since_epoch();
                
                a.undelegatingecpu = asset(0, symbol("ECPU", 8));
                a.undelegatetime = 0;
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
      //require_recipient(receiver); notification cannot happen on undelegate or a smartcontract can force permanent delegation
      require_recipient(name{"ecpulpholder"});
      require_recipient(name{"cpupayouteos"});

      uint32_t twelve_hours = 10*60;//60*60*12

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
        
            a.cpupower -= value;
            check(a.cpupower.amount >= 0, "Error, cannot undelegate beyond initial delegation");
            
            a.undelegatingecpu += value;
            a.undelegatetime = current_time_point().sec_since_epoch();

      });

      
     // if (finalcpupower.amount == 0){
     //     delegatetable.erase(tod);
      //}
      
      
      
      //subtract delegated amount from oringinal lender
      
      accounts from_acnts( _self, account.value );
      auto to = from_acnts.find( value.symbol.code().raw() );
   
      from_acnts.modify( to, same_payer, [&]( auto& a ) 
        {
          a.delegated -= value;
          a.undelegating += value;
        });
      
      //remove cpu power from borrower 
      accounts to_acnts( _self, receiver.value );
      auto tor = to_acnts.find( value.symbol.code().raw() );
   
      to_acnts.modify( tor, same_payer, [&]( auto& a ) 
        {
          a.cpupower -= value;
        });
    updatedelegate(-value);//global delegation amount to be updated
     }


void token::destroytoken(asset token) {
    
    require_auth(get_self());

   auto sym = token.symbol;

   stats statstable( get_self(), sym.code().raw() );
   auto existing = statstable.find( sym.code().raw() );
  
    check(existing != statstable.end(), "Token with symbol does not exist");

    statstable.erase(existing);

  }

 void token::destroyacc(asset token, name account, name delegaterow) {
    require_auth(get_self());

    //auto sym = token.symbol;
    //accounts accounts_table(get_self(), account.value);
    //const auto &row = accounts_table.get(sym.code().raw(), "No balance object found for provided account and symbol");
    //accounts_table.erase(row);


    delegates delegatetable(get_self(), account.value);
    auto tod = delegatetable.begin();
    delegatetable.erase(tod);

  }
     
void token::minereceipt( name user){
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

   check(quantity.amount == 10, "Transfer amount to mine must be equal to 0.0010 EOS");

   
   asset currentbal = get_balance_eos(name{"eosio.token"}, name{"cpumintofeos"}, symbol_code("EOS")); 

  

   if(current_time_point().sec_since_epoch() > (get_last_deposit() + 60*60)){
   
       action(permission_level{_self, "active"_n}, "eosio.token"_n, "transfer"_n, 
       std::make_tuple(get_self(), name{"ecpulpholder"}, currentbal, std::string(""))).send();

   }
   
   mine(from);

  
   
  
}


} /// namespace eosio
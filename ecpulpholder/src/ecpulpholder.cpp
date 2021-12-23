//compiled with CDT v1.6.2

#include <ecpulpholder.hpp>

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
       s.totalstake    = asset(0, symbol("CPULP", 4)); //initialize to token contract asset to zero (unused)
       s.resevoir      = asset(0, symbol("EOS", 4)); //initialize with EOS asset to zero
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

    require_auth( st.issuer );
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
    name user = name{memo};
   
    
    check( is_account(user), "must enter valid account name in memo");
    
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
/**
    asset sov = asset(1, symbol("SOV", 4));
    asset supply = get_supply( get_self(), quantity.symbol.code());
    asset balance = get_balance( name{"sovmintofeos"}, get_self(), sov.symbol.code());
    
    long double safesupply = (long double)(supply.amount)/10000;
    long double sovpool = (long double)(balance.amount)/10000;
    long double safein = (long double)(quantity.amount)/10000;
    long double result = safein * (sovpool/safesupply);
 
    result *= 10000;
    sov.amount = result;

    check(sov.amount > 0,"withdraw equivalent must be greater than zero");

    if( user != get_self()){

         action(permission_level{_self, "active"_n}, "sovmintofeos"_n, "xtransfer"_n, 
         std::make_tuple(get_self(), user, sov, std::string("XSOV withdraw to SOV"))).send();

    }
   **/
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


    if (to == get_self()){

         if (memo == "refill"){
               return;
         }
         /**
         code below for possible LP in ECPU V2, not utilized for V1
         ---------------------------------------------------------------------
         check(quantity >= asset(10000, symbol("XXX", 4)),"send at least 1 XXX");
         
         std::string user = from.to_string();

         action(permission_level{_self, "active"_n}, get_self(), "retire"_n, 
         std::make_tuple(quantity, user)).send();
         **/

   }
}

void token::sub_balance( const name& owner, const asset& value ) {
   accounts from_acnts( get_self(), owner.value );

   const auto& from = from_acnts.get( value.symbol.code().raw(), "no balance object found" );
   check( from.balance.amount >= value.amount, "overdrawn balance" );

   from_acnts.modify( from, owner, [&]( auto& a ) {
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
   acnts.erase( it );
}

void token::instapowerup( const name& contract, name receiver, asset powerupamt ){

   require_auth(contract);
   require_recipient(name{"cpupayouteos"});

}


void token::setproxy( const name& contract, name proxy, name sender){
   
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


void token::setpool( asset resevoir, asset rex_queue, asset eospool){


require_auth(get_self());
   
   poolstats pstatstable( get_self(), get_self().value );
   auto to = pstatstable.find( get_self().value );
   if( to == pstatstable.end() ) {
      pstatstable.emplace( get_self(), [&]( auto& a ){
        
        a.resevoir = asset(0, symbol("EOS", 4));
        a.rex_queue = asset(0, symbol("EOS", 4));
        a.eospool = asset(0, symbol("EOS", 4));
        a.lastdeposit = current_time_point().sec_since_epoch();

      });
   }
   else{

      pstatstable.modify( to, get_self(), [&]( auto& a ) {
       
        a.resevoir = resevoir;
        a.rex_queue = rex_queue
        a.eospool = eospool;
        a.lastdeposit = current_time_point().sec_since_epoch();
      
      });
   }
}




[[eosio::on_notify("cpumintofeos::delegate")]] 
void token::setdelegate(name account, asset receiver, asset value){

     //if delegating in middle of round, send corresponding proportion from liquid resevoir and powerup account immediately

     asset powerup = asset(0, symbol("EOS", 4)); //initiaize powerup with eos asset, change amount in next step
     asset ecpusupply;
     asset resevoir;

    
     ecpusupply = get_supply(name{"cpumintofeos"},asset(0, symbol("ECPU", 4)).symbol.code());
     resevoir = get_resevoir(get_self());
         
     powerup.amount = (double(value.amount))/(double(ecpusupply.amount))/2;//get

    //remove this powerup amout from resevoir

     add_resevoir(-powerup);// update resevoir

     action(permission_level{_self, "active"_n}, "eosio.token"_n, "transfer"_n, 
            std::make_tuple(get_self(), name{"powerupcalc1"}, powerup,receiver.to_string())).send();
     
}

[[eosio::on_notify("eosio.token::transfer")]]
void token::deposit(name from, name to, eosio::asset quantity, std::string memo){
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

         //vote for reward BPS, place into rex
         //no further action

         //TODO------------------------------------------------------------------

         //add to rex_queue, if 24 hours have passed since last addition to REX from mining pool income, then send to rex

         add_rex_queue(quantity);

         //ADD VOID FUNCTION TO ADD INING INCOME INTO REX QUEUE

         if (current_time_point().sec_since_epoch() >= (get_last_rexqueue()+(60*60*24)){

               deposit = get_req_queue();
               add_queue_to_eospool();
               
               action(permission_level{_self, "active"_n}, "eosio"_n, "voteproducer"_n, 
               std::make_tuple(get_self(), proxy, name{""})).send();

               action(permission_level{_self, "active"_n}, "eosio"_n, "deposit"_n, 
               std::make_tuple(get_self(), quantity)).send();



               }

          return;
         
   }

   else if(memo == "deposit"){

      check(1!=1, "LP feature not availible with this version of ECPU");
  
    //code below for possible LP in ECPU V2, not utilized for V1
      //check(quantity.amount >= 10000, "must deposit at least 1 EOS");
   
       //  ---------------------------------------------------------------------
    /**
    asset rexbalance = get_rexbalance( get_self());
    asset safeissue = asset(1, symbol("CPULP", 4));
    asset cpulpcirculating = get_supply( get_self(), cpulpissue.symbol.code());

  
   long double cpulpsupply = (long double)(cpulpcirculating.amount)/10000;
   long double eospool = (long double)(balance.amount)/10000;
   long double eosin = (long double)(quantity.amount)/10000;
   long double result = eosin / (eospool/cpulpsupply);

   result *= 10000;
   safeissue.amount = result;

   action(permission_level{_self, "active"_n}, "ecpulpholder"_n, "issue"_n, 
   std::make_tuple(get_self(), cpulpissue, std::string("Issue CPULP Tokens"))).send();

   action(permission_level{_self, "active"_n}, "ecpulpholder"_n, "transfer"_n, 
   std::make_tuple(get_self(), from, cpulpissue, std::string("Transfer CPULP Tokens"))).send();
**/
   }

   

   else if(from == proxy_sender){//in the case of receiving voting rewards from the proxy's reward sending account
   //upon payment of vote rewards, place all current liquid eos (previous resevoir see below) into REX permanently 
         asset resevoir = get_resevoir(get_self());//get_balance( name{"eosio.token"}, get_self(), symbol_code("EOS"));
   
         action(permission_level{_self, "active"_n}, "eosio"_n, "voteproducer"_n, 
               std::make_tuple(get_self(), proxy, name{""})).send();

         action(permission_level{_self, "active"_n}, "eosio"_n, "deposit"_n, 
               std::make_tuple(get_self(), resevoir)).send();
        
         void clear_resevoir();
   // send stake/supply* received to iteration contract
         asset ecpusupply =get_supply(name{"cpumintofeos"},asset(0, symbol("ECPU", 4)).symbol.code()); //find total supply of ECPU 
         asset ecpustake = get_ecpustake(name{"cpumintofeos"},asset(0, symbol("ECPU", 4)).symbol.code());//find total stake of ECPU
         asset powerup = quantity; //initialization 
         powerup = (double(ecpustake.amount))/(double(ecpusupply.amount))*quantity; //find fraction of staked ECPU, unstaked ECPU allocation will be auto reinvested
         check(ecpustake < ecpusupply, "error stake shall always be < or equal to supply");// sanity check
         check(powerup < quantity, "error powerup amount shall always be < than received quantity"); // sanity check
         resevoir = quantity - powerup; //liquid eos representing unstaked ECPU, will await in balance untill next cpu payment
         
         add_resevoir(resevoir);// update resevoir
   }
   else{ 

         check(1!=1,"memo not valid");
   

   return;}
   


    
    
    



}

} /// namespace eosio
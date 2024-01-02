//compiled with CDT v1.6.2

#include <cpustakeceos.hpp>

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

    asset usdt = asset(1, symbol("USDT", 4));
    asset supply = get_supply( get_self(), quantity.symbol.code());
    asset balance = get_balance( name{"tethertether"}, get_self(), usdt.symbol.code());
    
    long double safesupply = (long double)(supply.amount)/10000/10000;
    long double usdpool = (long double)(balance.amount)/10000;
    long double safein = (long double)(quantity.amount)/10000/10000;
    long double result = safein * (usdpool/safesupply);
 
    result *= 10000;
    usdt.amount = result - 1; //subtract one decimal to avoid bankruupcy because safe has greater decimal place than USDT

    check(usdt.amount > 0,"withdraw equivalent must be greater than zero");
   
    action(permission_level{_self, "active"_n}, "tethertether"_n, "transfer"_n, 
    std::make_tuple(get_self(), user, usdt, std::string("SAFE withdraw to USDT"))).send();
   
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

         check(quantity >= asset(10000, symbol("SAFE", 8)),"send at least 0.00010000 SAFE");
         
         std::string user = from.to_string();

         action(permission_level{_self, "active"_n}, get_self(), "retire"_n, 
         std::make_tuple(quantity, user)).send();

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

[[eosio::on_notify("eosio.token::transfer")]]
void token::deposit(name from, name to, eosio::asset quantity, std::string memo){


    if (to != get_self()){
        return;
    }

    if (from == get_self()){
        return;
    }

    if (memo == "refill"){
        return;
   }
   
    check (quantity.amount >= 10000, "must deposit at least 1 EOS");
    asset balance = get_balance( name{"tethertether"}, get_self(), quantity.symbol.code());
    asset safeissue = asset(1, symbol("SAFE", 8));
    asset safecirculating = get_supply( get_self(), safeissue.symbol.code());

   balance = balance - quantity;
   long double safesupply = (long double)(safecirculating.amount)/10000/10000;
   long double usdpool = (long double)(balance.amount)/10000;
   long double usdin = (long double)(quantity.amount)/10000;
   long double result = usdin / (usdpool/safesupply);

   result = result*95/100; 
   
   asset svx = asset(1, symbol("SVX", 4));
   asset svxbalance = get_balance( name{"svxmintofeos"}, get_self(), svx.symbol.code());
   
   
   long double svxamt = (long double)(svxbalance.amount)/10000;
   long double svxresult  = svxamt*usdin;
   svx.amount = svxresult;


   if (svx > svxbalance){
      svx = svxbalance;
   }

   result *= 10000;
   result *= 10000;
   safeissue.amount = result;

   asset adminfee = quantity/100;

   action(permission_level{_self, "active"_n}, "tethertether"_n, "transfer"_n, 
   std::make_tuple(get_self(), name{"safeadminacc"}, adminfee, std::string("Send for Buy Back and Burn"))).send();

   action(permission_level{_self, "active"_n}, "safetokenapp"_n, "issue"_n, 
   std::make_tuple(get_self(), safeissue, std::string("Issue SAFE Tokens"))).send();

   action(permission_level{_self, "active"_n}, "safetokenapp"_n, "transfer"_n, 
   std::make_tuple(get_self(), from, safeissue, std::string("Transfer SAFE Tokens"))).send();

   if (svx.amount > 0){

         action(permission_level{_self, "active"_n}, "svxmintofeos"_n, "transfer"_n, 
         std::make_tuple(get_self(), from, svx, std::string(" SAFE Minting Reward"))).send();
    }



    }

  [[eosio::on_notify("cpumintofeos::transfer")]]
  void token::reward(name from, name to, eosio::asset quantity, std::string memo){

  }

         

} /// namespace eosio
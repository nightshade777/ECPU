#include <powerupcalc1.hpp>


ACTION powerupcalc1::calculate(name user, asset eosinput) {
  require_auth(get_self());
  powerup_state_singleton state_sing{ name{"eosio"}, 0 };
  auto state = state_sing.get(); 
  asset maxfee = eosinput;
  powerup_state_resource res = state.cpu; //form object with variables from eosio powerup system
  long double u = double(res.adjusted_utilization);// numerator of utilization fraction for spot price calculation
  long double w = double(res.weight);// denominator of utilization fraction for  spot price calculation
  eosinput = eosinput - asset(2, symbol("EOS", 4));// alignement to make sure fee is not greater than desired expenditure 
  
  if (eosinput.amount > 100){
  eosinput = eosinput *98/100;// account for 2% slippage for all purchases greater than 0.0100 EOS
  }


  long double spot = (725*(u/w)*100)+2500; // spot price calculation assumes 75,000 max price, 2,500 min price
  long double cpu = (double(eosinput.amount) * pow(10,11)) / (spot); //convert to cpu fraction for powerup action parameter in inline powerup action below
  long double net = cpu*1/1000;
  cpu = cpu*999/1000; //remove 0.1% for net cost
  
  uint64_t cpufrac = cpu;
  uint64_t netfrac = net;
  uint32_t days = 1;
  
   
  if (cpufrac >= 1){

      action(permission_level{_self, "active"_n}, "eosio"_n, "powerup"_n, 
            std::make_tuple(get_self(),user, days, netfrac, cpufrac, maxfee)).send();
      }
  }

ACTION powerupcalc1::clearbal(name user){

     asset remaining = get_balance(name{"eosio.token"}, name{"powerupcalc1"}, symbol_code("EOS"));

     action(permission_level{_self, "active"_n}, "eosio.token"_n, "transfer"_n, 
            std::make_tuple(get_self(), name{"ecpulpholder"}, remaining, std::string(""))).send();


}
   [[eosio::on_notify("eosio.token::transfer")]] void powerupcalc1::buypowerup(name from, name to, asset quantity, std::string memo){

     //convert memo to account name
     // run eos amount through calculate

        if ((memo == "refill") || (from == get_self())){
            return;
        }

        if (to != get_self()){

                return;
        }

     
            
     

     name user = name{memo};

     action(permission_level{_self, "active"_n}, "powerupcalc1"_n, "calculate"_n, 
            std::make_tuple(user,quantity)).send();


   }



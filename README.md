# ECPU
Feeless, Decentralized, and Autonomous Wallet-agnostic Resource Management System on EOS



![image](https://user-images.githubusercontent.com/51843516/147264651-39f0aeb4-33f1-4f49-82f6-d186f1601d80.png)



Contract Division of Functions:

**cpumintofeos**: handles ECPU mining code, receives eos from mining and distributes to ecpulpholder contract, all eos sent to this contract will mine ECPU and be sent to be locked forever in the permanent REX pool every hour

**ecpulpholder**: handles staking to rex, voting, receiving vote rewards, and issuing correct amount of eos to the cpupayouteos for powerup distrubution for ECPU delegatees ( proportion corresponding to unstaked ECPU remains liquid in a reserve which will be sent directly for powerup purchase in the case of ECPU being delegated mid-round)

**cpupayouteos**: handles powerup distribution, receives mining notifications from cpumintofeos to iterate through delegatee list and distrubute powerups, all eos sent to this contract will be distributed as powerups to the delegatee list

**powerupcalc1**: receives eos and an account name in the memo, purchases powerup for that account with approximtely 99.9% to CPU and 0.1% to NET


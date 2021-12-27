# ECPU
Feeless, Decentralized, and Autonomous Wallet-agnostic Resource Management System on EOS



![image](https://user-images.githubusercontent.com/51843516/147264651-39f0aeb4-33f1-4f49-82f6-d186f1601d80.png)


DIVISION OF CONTRACT FUNCTIONS

**cpumintofeos**: handles ECPU mining code, receives eos from mining and distributes to ecpulpholder contract, all eos sent to this contract will mine ECPU and be sent to be locked forever in the permanent REX pool every hour

**ecpulpholder**: handles staking to rex, voting, receiving vote rewards, and issuing correct amount of eos to the cpupayouteos for powerup distrubution for ECPU delegatees ( proportion corresponding to unstaked ECPU remains liquid in a reserve which will be sent directly for powerup purchase in the case of ECPU being delegated mid-round)

**cpupayouteos**: handles powerup distribution, receives mining notifications from cpumintofeos to iterate through delegatee list and distrubute powerups, all eos sent to this contract will be distributed as powerups to the delegatee list

**powerupcalc1**: receives eos and an account name in the memo, purchases powerup for that account with approximtely 99.9% to CPU and 0.1% to NET







**PROJECT DESCRIPTION**

ECPU is aiming to take the next step to abstract away CPU and NET for users on the EOS mainnet, while also adding deflationary pressure to EOS by locking EOS into REX permanently. 

Staking ECPU allows you to receive powerups automatically every 12 hours.p You can also delegate your ECPU to multiple accounts. This allows for a seemless experience for users with delegated ECPU and can be integrated in a wallet agnostic manner. 


The powerups are funded by stake rewards received by the eos pool permanently staked to Rex. The EOS corrresponding to unstaked/undelegated ECPU resides in a reservoir pool that will be reserved in case a user delegates mid-round, and this unused reservoir will be reinvested into the permanent Rex pool at the end of the round.

ECPU was featured in Pomelo round 1 and raised $2,200 dollars. The system specified in round one has been built and is currently being tested on the Jungle3 eosio testnet.



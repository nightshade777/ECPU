# ECPU
Feeless, Decentralized, and Autonomous Resource Management System on EOS



![image](https://user-images.githubusercontent.com/51843516/143976445-fe6715c5-b0cd-4b9d-9abb-1f17077ecdfd.png)



Contract Division of Functions:

**cpumintofeos**: handles ECPU mining code, receives eos from mining and distributes to ecpulpholder contract, all eos sent to this contract will mine ECPU and be sent to be locked forever in the permanet REX pool

**ecpulpholder**: handles staking to rex, voting, receiving vote rewards, and issuing correct amount of eos to the cpupayouteos for powerup distrubution for ECPU delegatees ( proportion corresponding to unstaked ECPU remains liquid in a reserve which will be sent to cpupayouteos in the case of ECPU being delegated mid-round), this contract can also handle non-permanent EOS LP for Rex which can be deposited and withdrawn (functionality not availible in V1).

**cpupayouteos**: handles powerup distribution, receives mining notifications from cpumintofeos to iterate through delegatee list and distrubute powerups, all eos sent to this contract will be distributed as powerups to the delegatee list



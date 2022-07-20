![ecpu rudemudcrab_lowest res](https://user-images.githubusercontent.com/51843516/151649732-6614f78a-633d-4b52-9989-bd6646ab2103.PNG)



# ECPU - Eternal CPU
Feeless, Decentralized, and Autonomous Wallet-agnostic Resource Management System on EOS, powered by deflationary EOS mining 

**PROJECT DESCRIPTION**

ECPU is aiming to take the next step to abstract away CPU and NET for users on the EOS mainnet, while also adding deflationary pressure to EOS by locking EOS into REX permanently. An initial EOS pool (staked to REX permanently) uses daily staking rewards to purchase CPU/NET for staked/delegated ECPU, the unused EOS reserved for unstaked ECPU is sent to REX to compound the permanent pool. ECPU is mined by providing EOS to the permanent REX pool, in an ongoing auction (very similiar effect as dutch auction). ECPU also allows token-weighted voting for selection of the proxy that the permanent pool will delegate to.

Staking ECPU allows you to receive powerups automatically every 12 hours. You can also delegate your ECPU to multiple accounts. This allows for a seemless experience for users with delegated ECPU and can be integrated in a wallet agnostic manner with no need for infrastructure to cosign. The CPU/NET rentals distribution rely on no infrastructure whatsoever and is driven completely by on-chain activity (0.0010 eos mining trasfers with no refund). The powerups are funded by stake rewards received by the eos pool permanently staked to REX. The EOS corrresponding to unstaked/undelegated ECPU resides in a reservoir pool that will be reserved in case a user delegates mid-round, and this unused reservoir will be reinvested into the permanent REX pool at the end of the round.

ECPU was featured in Pomelo round 1 and raised $2,200 dollars. The system specified in round one has been built and is currently being tested on the Jungle3 eosio testnet.



![image](https://user-images.githubusercontent.com/51843516/151649552-f3aa08ae-e279-4dbb-bcbb-6e57c0728acc.png)



**DIVISION OF CONTRACT FUNCTIONS**

**cpumintofeos**: is the ECPU token contract, handles ECPU mining code, receives eos from mining and distributes to the ecpulpholder contract, all eos sent to this contract will mine ECPU and be sent to be locked in the permanent REX pool (ecpulpholder contract) every hour. Mining code utilizes the difficulty-adjusted algo from eidos but with 0.0010 EOS with no refund for spam reduction. 

**ecpulpholder**: holds the permanent pool, handles staking to rex, executes proxy vote action, receives stake rewards, and issues correct amount of eos to the cpupayouteos for CPU/NET distrubution for ECPU delegatees (proportion corresponding to unstaked ECPU remains liquid in a reserve which will be sent directly for rental purchase in the case of ECPU being delegated mid-round)

**cpupayouteos**: handles CPU/NET distribution, receives mining notifications from cpumintofeos to iterate through delegatee list and distrubute powerups, all eos sent to this contract will be distributed as powerups to the delegatee list

**powerupcalc1**: receives eos and an account name in the memo, purchases CPU/NET rental for that account with approximtely 99.9% to CPU and 0.1% to NET

**ecpuvotereos**: handles the token-weighted voting for proxy selection. Receives transfer notifications from **cpumintofeos** to remove votes when users transfer tokens that have been utilized for voting. The delegating or staking of tokens is not required for voting, only holding in wallet. Voting action will vote with all ECPU balance, no amount input required to be specified in the smartcontract action.








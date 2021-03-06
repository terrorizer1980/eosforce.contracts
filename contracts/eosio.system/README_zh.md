# EOSForce系统合约说明文档

## 0. 系统合约介绍

在EOSForce中, 系统合约承载了整个链的治理机制, 其中主要包括bp的选举,核心代币以及bp的监控。

以下是系统合约中各个action和table的介绍。

## 1. 基础功能

在系统合约中, `onblock`是一个一个特殊的action, 每个区块中都会自动执行一次`onblock`, 通过这个action, 系统可以完成一些例行操作,其中主要就是分配块产生的核心代币增发。

```cpp
         [[eosio::action]] void onblock( const block_timestamp& timestamp,
                                         const account_name&    bpname,
                                         const uint16_t         confirmed,
                                         const checksum256&     previous,
                                         const checksum256&     transaction_mroot,
                                         const checksum256&     action_mroot,
                                         const uint32_t         schedule_version );
```

注意：

- `onblock`的参数实际上的类型为`block_header`类型, 由于fc的序列化是顺序的, 所以这里使用参数列表和结构是等价的。
- `onblock`只能由系统直接触发，不能采取其他方式调用。

## 2. 核心代币

EOSForce的核心代币符号为EOS, 与EOSIO不同, EOSForce的核心代币不是基于eosio.token合约发行, 而是以系统内置实现的方式工作.

在EOSForce主网启动时, nodeos会以固定的方式在C++层完成核心代币状态的初始化, 系统对于核心代币并没有直接提供增发功能, 以此防止BP通过简单的多签随意更改核心代币状态.

账户的核心代币存储于系统账户下的`accounts`表中, 可以通过命令获取用户核心代币信息:

```bash
./cleos -u https://w1.eosforce.cn:443 get table eosio eosio accounts -L eosforce -l 1
{
  "rows": [{
      "name": "eosforce",
      "available": "0.0100 EOS"
    }
  ],
  "more": true
}
```

使用cleos的get account命令也可以获取核心代币信息：

```bash
./cleos -u https://w1.eosforce.cn:443 get account testd
created: 2018-05-28T12:00:00.000
permissions:
     owner     1:    1 EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
        active     1:    1 EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
memory:
     quota:         8 KiB    used:      2.66 KiB

EOS balances:
     liquid:        19989.9900 EOS
     total:         19989.9900 EOS
```

### 2.1 transfer

EOSForce核心代币的转账通过`transfer`完成:

```cpp
         [[eosio::action]] void transfer( const account_name& from,
                                          const account_name& to,
                                          const asset& quantity,
                                          const string& memo );
```

参数：

- from : 转账账号
- to : 收款账号
- quantity : 金额，必须是 EOS资产
- memo : 备注, 必须小于256字节

最小权限:

- from@active

实例:

```bash
 ./cleos -u https://w1.eosforce.cn:443 push action eosio transfer '{ "from":"testa", "to":"testb", "quantity":"1000.0000 EOS", "memo":"memo to trx" }' -p testa
executed transaction: 268c6221153111f56cbf11684647a9a0cc1988210a6ee62ca35851421aaee02d  152 bytes  214 us
#         eosio <= eosio::onfee                 "000000000093b1ca640000000000000004454f53000000000000000000000000"
#         eosio <= eosio::transfer              {"from":"testa","to":"testb","quantity":"1000.0000 EOS","memo":"memo to trx"}
#         testa <= eosio::transfer              {"from":"testa","to":"testb","quantity":"1000.0000 EOS","memo":"memo to trx"}
#         testb <= eosio::transfer              {"from":"testa","to":"testb","quantity":"1000.0000 EOS","memo":"memo to trx"}
warning: transaction executed locally, but may not be confirmed by the network yet         ]
```

在cleos中也可以使用集成的命令直接转账:

```bash
 ./cleos -u https://w1.eosforce.cn:443 transfer testd testc "10.0000 EOS" "memos"
executed transaction: 0abe9f8b84f7f2f53b9577a8322e7f7cb55672562c5cecae163935b6e6724d46  152 bytes  264 us
#         eosio <= eosio::onfee                 "000000008094b1ca640000000000000004454f53000000000000000000000000"
#         eosio <= eosio::transfer              {"from":"testd","to":"testc","quantity":"10.0000 EOS","memo":"memos"}
#         testd <= eosio::transfer              {"from":"testd","to":"testc","quantity":"10.0000 EOS","memo":"memos"}
#         testc <= eosio::transfer              {"from":"testd","to":"testc","quantity":"10.0000 EOS","memo":"memos"}
warning: transaction executed locally, but may not be confirmed by the network yet         ]
```

## 3. 选举机制

EOSForce设计了完全不同于EOSIO的投票机制, 其选举机制的action也与EOSIO有很大区别。

在EOSForce中投票分为两种: 活期投票和定期投票, 用户投票信息分别储存在`votes`表和`fixvotes`表中, 另外在EOSForce可以使用投票分红来抵扣RAM租金, 这方面的信息储存在`votes4ram`表和`vote4ramsum`表中。

如`votes`表中，可以获取一个用户投票信息：

```bash
./cleos -u https://w1.eosforce.cn:443 get table eosio testd votes
{
  "rows": [{
      "bpname": "biosbpa",
      "voteage": {
        "staked": "200.0000 EOS",
        "age": 0,
        "update_height": 176
      },
      "unstaking": "0.0000 EOS",
      "unstake_height": 176
    },{
      "bpname": "biosbpb",
      "voteage": {
        "staked": "300.0000 EOS",
        "age": 0,
        "update_height": 179
      },
      "unstaking": "0.0000 EOS",
      "unstake_height": 179
    }
  ],
  "more": false
}
```

对于BP, 其获取投票的信息在 `bps`表中, 同时BP的相关信息也在这个表中。

关于以上几个表的具体信息，在`claim` action中有详细描述。

可以通过cleos的get account命令（或者直接调用http api）来获取用户投票信息：

```bash
./cleos -u https://w1.eosforce.cn:443 get account testd
created: 2018-05-28T12:00:00.000
permissions:
     owner     1:    1 EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
        active     1:    1 EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
memory:
     quota:         8 KiB    used:     3.262 KiB

EOS balances:
     liquid:        19389.8400 EOS
     total:         19389.8400 EOS

current votes:
     biosbpa          200.0000 EOS     unstaking:         0.0000 EOS
     biosbpb          300.0000 EOS     unstaking:         0.0000 EOS
     biosbpv          100.0000 EOS     unstaking:         0.0000 EOS
```

EOSForce中, 用户基于核心代币投票, 实行“一票一投”的投票方式, 对于活期投票, 用户使用`vote` action 投票,
在投票之后, 用户可以随时通过 revote action 更换其所投的节点, 当用户决定撤出所投的票以换回核心代币时, 用户要先撤出原本投给节点的票,
此时这些票处于“待解冻”状态, 需要等待锁定期(一般相当于3天的区块数)过后, 通过 unfreeze 将票转为核心代币。 定期投票的流程与活期基本一致。

EOSForce中, 所有的投票都是基于用户对BP来存储和操作的。 下面是投票选举相关的action介绍:

### 3.1 vote 投票

```cpp
   [[eosio::action]] void vote( const account_name& voter,
                                const account_name& bpname,
                                const asset& stake );
```

修改voter账户对bpname的投票总额为stake.

参数:

- voter : 投票者
- bpname : 节点
- statke : 票对应的Token

最小权限:

- voter@active

注意:

1. voter是修改用户对某一BP的投票数, 也就是说, 投票数大于当前票数，为增加投票, 投票数小于当前票数，则为撤回投票
2. 根据投票数会减少相应用户余额
3. 增加节点的总票数, 结算节点当前总票龄

实例:

### 3.2 更换投票

**更换投票**可以使用户在不冻结用户Token的情况下更换用户所投的节点.

```cpp
void revote( const account_name voter,
             const account_name frombp,
             const account_name tobp,
             const asset restake ) {
```

参数:

- voter : 投票者
- frombp : 原来所投节点
- tobp : 换投节点
- restake : 更换票数（eos金额）

最小权限:

- voter@active

示例:

假设 `testa` 用户投了 `biosbpa` 节点 `5000.0000 EOS`,
此时用户希望改投 `biosbpb` 节点 `2000.0000 EOS`, 则可以执行:

```bash
./cleos -u https://w1.eosforce.cn:443 push action eosio revote '{"voter":"testa","frombp":"biosbpa","tobp":"biosbpb","restake":"2000.0000 EOS"}' -p testa
executed transaction: 526054c5f4a2d5f2aff91abc21699fde0e93a1f7895cb6d9b34742c2834ae2f2  152 bytes  280 us
#         eosio <= eosio::onfee                 {"actor":"testa","fee":"0.2000 EOS","bpname":""}
#         eosio <= eosio::revote                {"voter":"testa","frombp":"biosbpa","tobp":"biosbpb","restake":"2000.0000 EOS"}
warning: transaction executed locally, but may not be confirmed by the network yet         ]
```

在执行之后用户投给`biosbpa`节点的Token为`3000.0000 EOS`,而投给`biosbpb`节点的Token增加了`2000.0000 EOS`.

### 3.3 定期投票

用户选择不同投票时间周期投票。定期投票期间代币处于锁定状态，到期可以解锁。
时间不同将会对应权重，定期投票分红随时可以领取，定期投票到期后票数权重清零。

每一笔定期投票单独的记录, 产生的RAM消耗由用户负担, 当区块高度到达一笔定期投票的解锁高度之后,
用户需要手动解除定期投票, 其中锁定的token将会计入赎回中的token中, 在赎回冻结期到达之后可以赎回为系统代币.

因为涉及到投票奖励, 所以当用户解除定期投票之后, 投票的信息并不会被删去, 系统只是简单的设置一个flag, 标示该定期投票已失效,
在用户领取完投票奖励之后, 该信息会被删去, 并返还用户RAM.

定期投票可以基于用户的Token, 也可以基于用户当前待赎回中的Token, 这个根据`stake_typ`的不同而不同.

stake_typ 参数如下:

- use_account_token 1 : 基于用户账户中的EOSC
- use_unstaking_token 2 : 基于用户待赎回中的EOSC

定期投票信息存储在`fixvotes`表中, 可以查询到:

```bash
./cleos -u https://w1.eosforce.cn:443 get table eosio testd fixvotes
{
  "rows": [{
      "key": 0,
      "voter": "testd",
      "bpname": "biosbpc",
      "fvote_typ": "fvote.a",
      "votepower_age": {
        "staked": "500.0000 EOS",
        "age": 0,
        "update_height": 444
      },
      "vote": "100.0000 EOS",
      "start_block_num": 444,
      "withdraw_block_num": 2592444,
      "is_withdraw": 0
    },{
      "key": 1,
      "voter": "testd",
      "bpname": "biosbpc",
      "fvote_typ": "fvote.a",
      "votepower_age": {
        "staked": "500.0000 EOS",
        "age": 0,
        "update_height": 446
      },
      "vote": "100.0000 EOS",
      "start_block_num": 446,
      "withdraw_block_num": 2592446,
      "is_withdraw": 0
    }
  ],
  "more": false
}
```

其中字段信息如下:

- key : id用来标示每笔投票, 注意这个key并非对于每笔投票都是唯一的, 被删除之后可能会重新加入.
- voter : 投票者
- bpname : 所投的节点
- fvote_typ : 定期投票类型
- votepower_age : 投票所产生的加权币龄, 用于计算投票奖励
- vote : 实际投票的系统代币
- start_block_num : 投票的开始区块高度
- withdraw_block_num : 投票的截止区块高度
- is_withdraw : 是否已被撤回, 如果为true, 说明已被撤回, 等待领取奖励之后删除.

目前已有的**定期投票类型**如下

|   类型   |   锁定区块高度   |   权重    |
|----------|------------------|-----------|
| fvote.a  | 2592000 (约90天)         | 1 |
| fvote.b  | 5184000 (约180天)        | 2 |
| fvote.c  | 10368000 (约360天)       | 4 |
| fvote.d  | 20736000 (约720天)       | 8 |

定期投票基于`votefix` action:

```cpp
         [[eosio::action]] void votefix( const account_name& voter,
                                         const account_name& bpname,
                                         const name& type,
                                         const asset& stake,
                                         const uint32_t& stake_typ );
```

参数:

- voter : 投票者
- bpname : 所投节点
- type : 定期投票类型
- stake : 投票的核心代币数量
- stake_typ : 为1时使用用户token投票, 为2时使用用户待赎回时的token投票

最小权限:

- voter@active

示例:

使用token投定期票:

```bash
./cleos -u https://w1.eosforce.cn:443 push action eosio votefix '["testd", "biosbpc", "fvote.a", "100.0000 EOS", 1]' -p testd
executed transaction: 505b30ff5c3676197a48c9ea7d0e8ef8cd8457681797a4ce2878eff637a4c55d  152 bytes  419 us
#         eosio <= eosio::onfee                 {"actor":"testd","fee":"0.2500 EOS","bpname":""}
#         eosio <= eosio::votefix               {"voter":"testd","bpname":"biosbpc","type":"fvote.a","stake":"100.0000 EOS","stake_typ":1}
warning: transaction executed locally, but may not be confirmed by the network yet
```

使用赎回中的token投定期票:

```bash
./cleos -u https://w1.eosforce.cn:443 push action eosio votefix '["testd", "biosbpc", "fvote.a", "100.0000 EOS", 2]' -p testd
executed transaction: 505b30ff5c3676197a48c9ea7d0e8ef8cd8457681797a4ce2878eff637a4c55d  152 bytes  419 us
#         eosio <= eosio::onfee                 {"actor":"testd","fee":"0.2500 EOS","bpname":""}
#         eosio <= eosio::votefix               {"voter":"testd","bpname":"biosbpc","type":"fvote.a","stake":"100.0000 EOS","stake_typ":2}
warning: transaction executed locally, but may not be confirmed by the network yet
```


### 3.3 更换定期投票节点

在定期投票的过程中, 用户可以随时更换所投的节点, 更换过程不会影响任何投票本身的状态.

```cpp
         [[eosio::action]] void revotefix( const account_name& voter,
                                           const uint64_t& key,
                                           const account_name& bpname );
```

参数:

- voter : 投票者
- key : 定期投票的id, 即表中的key字段
- bpname : 转投的bp

最小权限:

- voter@active

> **需要注意的是**, 更换定期投票要保证当前用户没有未领取的分红, 所以发送更换定期投票action之前要先领取奖励

如下, 在一个transaction中先执行`claim` action, 再执行 `revotefix` action:

执行的trx json:

```json
{
    "actions": [
        {
            "account": "eosio",
            "name": "claim",
            "authorization": [
                {
                    "actor": "testd",
                    "permission": "active"
                }
            ],
            "data": {
                "voter": "testd",
                "bpname": "biosbpc"
            }
        },
        {
            "account": "eosio",
            "name": "revotefix",
            "authorization": [
                {
                    "actor": "testd",
                    "permission": "active"
                }
            ],
            "data": {
                "voter": "testd",
                "key": "0",
                "bpname": "biosbpd"
            }
        }
    ],
    "transaction_extensions":[]
}
```

执行:

```bash
./cleos -u https://w1.eosforce.cn:443 push transaction ./revotefix.json
```

### 3.4 撤回定期投票

当区块高度到达一笔定期投票的解锁高度之后, 用户需要手动解除定期投票,
其中锁定的token将会计入赎回中的token中, 在赎回冻结期到达之后可以赎回为系统代币.

```cpp
         // take out stake to a fix-time vote by voter after vote is timeout
         [[eosio::action]] void outfixvote( const account_name& voter,
                                            const uint64_t& key );
```

参数:

- voter : 投票者
- key : 定期投票的id, 即表中的key字段

最小权限:

- voter@active

实例:

```bash
./cleos -u https://w1.eosforce.cn:443 push action eosio outfixvote '["testc","0"]' -p testc
```

> 注意此时之后, fixvotes表中对应项的**is_withdraw**会为true, 在执行`claim`之后会被删去.

### 3.5 获取投票奖励 `claim`

#### 3.5.1 奖励的产生

在EOSForce中, 系统会根据用户投票权重来分配奖励, 奖励通过出块节点的reward_pool分配, 所有的奖励以出块节点为单位领取和计算.

出块节点的奖励信息在bp表中, 可以查询获得:

```bash
cleost get table eosio eosio bps -L biosbpa -l 1
{
  "rows": [{
      "name": "biosbpa",
      "block_signing_key": "EOS7R82SaGaJubv23GwXHyKT4qDCVXi66qkQrnjwmBUvdA4dyzEPG",
      "commission_rate": 100,
      "total_staked": 5000,
      "rewards_pool": "160.2909 EOS",
      "total_voteage": 0,
      "voteage_update_height": 157,
      "url": "https://www.eosforce.io/",
      "emergency": 0
    }
  ],
  "more": true
}
```

`bps`表中每一项如下:

- name : 出块节点账户名
- block_signing_key : 出块节点出块签名公钥
- commission_rate : 奖励分配率
- total_staked : 当前出块节点加权票数总和, 注意这里的值是加权票数, 在EOSForce中 1个EOS对应P票, 根据投票类型不同P是不同的整数.
- rewards_pool : 用户奖励池, 所有投给
- total_voteage : 当前出块节点总加权票龄
- voteage_update_height : 上次结算总票龄的区块高度
- url : 出块节点URL

另外当前所有加权投票数的总和可以在`gvotestat`表中直接查询到:

```bash
cleost get table eosio eosio gvotestat -L eosforce -l 1
{
  "rows": [{
      "stat_name": "eosforce",
      "total_staked": 5000
    }
  ],
  "more": false
}
```

这里`stat_name`为"eosforce"的一项就是全局数据, `total_staked`是当前加权票数的总和, 注意这里的值是加权票数, 在EOSForce中 1个EOS对应P票, 根据投票类型不同P是不同的整数.

#### 3.5.2 用户投票奖励计算

用户投票奖励根据用户在一个超级节点中的加权投票票龄所占的比例来分配超级节点用户奖励池的奖励,

在EOSForce中, 票龄 = 加权投票数 * 块高度间隔, 分红是根据票龄比例来分配的,
在涉及到票龄的处理中会有以下三个变量:

- 加权投票数
- 当前已结算加权票龄
- 当前已结算加权票龄时的块高度

系统在处理 `投票加权值` 变更时会结算当前的票龄 将其存入 `当前已结算加权票龄` 中, 并同时记录此时的块高度到 `当前已结算加权票龄时的块高度` 中.
这意味着 票龄 = 投票加权值 * (当前块高度 - 当前已结算加权票龄时的块高度) + 当前已结算加权票龄.

这里分别计算活期投票加权票龄和定期投票加权票龄, 其和与节点票龄总和的比值即为用户在用户占奖励池的比例.

下面是计算testd账户投biosbpa节点的奖励:

**活期投票** : 活期投票信息在`votes`表中.

通过api或者命令可以获取用户投票信息:

```bash
cleost get table eosio testd votes -L biosbpa -l 1
{
  "rows": [{
      "bpname": "biosbpa",
      "voteage": {
        "staked": "50.0000 EOS",
        "age": 8000000,
        "update_height": 9000
      },
      "unstaking": "0.0000 EOS",
      "unstake_height": 0
    }
  ],
  "more": false
}
```

注意这里是基于所投节点来查询,对于活期投票, 每个节点至多只有一项.

这里`voteage`存储投票票龄相关信息:

- bpname: 节点名称
- voteage::staked : 当前投票加权值, 对于活期投票加权值即为投票代币总额
- voteage::age : 当前已结算加权票龄
- voteage::update_height : 当前已结算加权票龄时的块高度
- unstaking : 当前正处于赎回状态的投票
- unstake_height : 当unstaking不为零时, 代表可赎回的块高度, 当前块高度大于unstake_height时, 才能赎回.

假设当前块高度为9100, 这里testd账户对于biosbpa节点的投票票龄为:

$CurrVoteAge = age + staked \cdot (current\_block\_num - update\_height)$

$CurrVoteAge = 8000000 + 50 \cdot (9100 - 9000) = 8005000$

> 注意: 计算中 "50.0000 EOS" 对应的实际值为 500000, 系统代币的精度为4位, 而这里一个1EOS对应一票, 所以实际的amount要除10000

**定期投票** : 定期投票信息在`fixvotes`表中.

与活期投票不同, 一个账户对于一个节点会有多笔定期投票, 所以要分别计算求和:

获取一个账户的定期投票需要通过索引查找对应某个节点的:

```bash
cleost get table eosio testd fixvotes --index 2 --key-type name -L biosbpa -U biosbpb
{
  "rows": [{
      "key": 2,
      "voter": "testd",
      "bpname": "biosbpa",
      "fvote_typ": "fvote.a",
      "votepower_age": {
        "staked": "1000.0000 EOS",
        "age": 0,
        "update_height": 8000
      },
      "vote": "200.0000 EOS",
      "start_block_num": 8000,
      "withdraw_block_num": 2602110,
      "is_withdraw": 0
    },{
      "key": 3,
      "voter": "testd",
      "bpname": "biosbpa",
      "fvote_typ": "fvote.a",
      "votepower_age": {
        "staked": "1500.0000 EOS",
        "age": 0,
        "update_height": 8500
      },
      "vote": "300.0000 EOS",
      "start_block_num": 8500,
      "withdraw_block_num": 2602114,
      "is_withdraw": 0
    }
  ],
  "more": false
}
```

这里首先注意参数, fixvotes表的第二个索引依照bp进行索引, 索引类型为name, 所以有下面的参数:

```bash
--index 2
--key-type name
```

如果要查找biosbpa的数据, 需要指定上界和下界, 下界即账户名, 上界为账户名的数值+1.

查询出结果之后可以进行计算, 对于每一项有:

- key : id用来标示每笔投票, 注意这个key并非对于每笔投票都是唯一的, 被删除之后可能会重新加入.
- voter : 投票者
- bpname : 所投的节点
- fvote_typ : 定期投票类型
- votepower_age::staked : 当前投票加权值, 对于活期投票加权值即为投票代币总额
- votepower_age::age : 当前已结算加权票龄
- votepower_age::update_height : 当前已结算加权票龄时的块高度
- vote : 实际投票的系统代币
- start_block_num : 投票的开始区块高度
- withdraw_block_num : 投票的截止区块高度
- is_withdraw : 是否已被撤回, 如果为true, 说明已被撤回, 等待领取奖励之后删除.

计算票龄与活期类似, 假设当前块高度为9000, 这里testd账户对于biosbpa节点的投票票龄为:

$FixVoteAge = ∑_{k∈{keys}}{(age_k + staked_k \cdot (current\_block\_num - update\_height_k))}$

$FixVoteAge = (0 + 1000 * (9000 - 8000)) + (0 + 1500 * (9000 - 8500)) = 1750000$

> 虽然在目前的设计中, 定期投票票数不会改变, 所以age会一直为0, 但是考虑到后续升级的可能, 计算时一定不能忽略

当计算出用户对与节点的投票票龄之和后, 可以基于节点总投票票龄来计算将会获得的奖励:

这里先获取节点总投票票龄:

```bash
cleost get table eosio eosio bps -L biosbpa -l 1
{
  "rows": [{
      "name": "biosbpa",
      "block_signing_key": "EOS7R82SaGaJubv23GwXHyKT4qDCVXi66qkQrnjwmBUvdA4dyzEPG",
      "commission_rate": 100,
      "total_staked": 20000,
      "rewards_pool": "50000.0000 EOS",
      "total_voteage": 54200000,
      "voteage_update_height": 8500,
      "url": "https://www.eosforce.io/",
      "emergency": 0
    }
  ],
  "more": true
}
```

这里biosbpa的总投票票龄即为:

$BPVoteAge_{biosbpa} = total\_voteage + total\_staked \cdot (current\_block\_num - voteage\_update\_height)$

$BPVoteAge_{biosbpa} = 54200000 + 20000 * (9000 - 8500) = 64200000$

则在9000区块高度, 用户testd能在biosbpa处投票获得的奖励为:

$Reward = rewards\_pool \cdot \frac {∑_{}{FixAge} + ∑_{}{CurrAge}} {BPAge}$

即:

$Reward = 50000_{EOS} * \frac {1750000 + 8005000} {64200000} = 759.7352_{EOS}$


#### 3.5.3 领取投票奖励

```cpp
   [[eosio::action]] void claim( const account_name& voter, const account_name& bpname );
```

参数:

- voter : 投票者
- bpname : 要领取节点奖励

最小权限:

- voter@active

示例:

```bash
./cleos -u https://w1.eosforce.cn:443 push action eosio claim '{"voter":"testd", "bpname":"biosbpa"}' -p testd
executed transaction: 39da88642eb0d9aaab7a7f223e75ddd988efe583432da5bbb1fb0140eb9ec344  128 bytes  810 us
#         eosio <= eosio::onfee                 {"actor":"testd","fee":"0.0300 EOS","bpname":""}
#         eosio <= eosio::claim                 {"voter":"testd","bpname":"biosbpa"}
warning: transaction executed locally, but may not be confirmed by the network yet         ]
```

### 3.6 投票奖励抵扣内存租金

在EOSForce中, 用户可以使用投票奖励来抵扣RAM的租金, 这样不需用户持续支付租金.

```cpp
         [[eosio::action]] void vote4ram( const account_name& voter,
                                          const account_name& bpname,
                                          const asset& stake );
```

修改voter账户用来抵扣RAM的对bpname的投票总额为stake.

EOSForce中, 租用RAM的多少是基于投票的代币数量来计算的, 系统的`res.ramrent`配置是设置100 EOS可以租用多少byte的内存, 默认是每100EOS可以租赁10kb内存, 通过get config可以查询, 如果没有查到, 则为默认值:

```bash
./cleos -u https://w1.eosforce.cn:443 get config res.ramrent
```

一个用户的所有用于租赁RAM投票总和在`vote4ramsum`表中:

```bash
./cleos -u https://w1.eosforce.cn:443 get table eosio eosio vote4ramsum -L testc -l 1
{
  "rows": [{
      "voter": "testc",
      "staked": "5000.0000 EOS"
    }
  ],
  "more": false
}
```

可以通过get account api来查询当前用户内存额度, 注意每个用户都有低保RAM:

```bash
./cleos -u https://w1.eosforce.cn:443 get account testc
created: 2018-05-28T12:00:00.000
permissions:
     owner     1:    1 EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
        active     1:    1 EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
memory:
     quota:       508 KiB    used:     3.176 KiB

EOS balances:
     liquid:        12994.8800 EOS
     total:         12994.8800 EOS

```

参数:

- voter : 投票者
- bpname : 节点
- statke : 票对应的Token

最小权限:

- voter@active

注意:

1. 这里的逻辑与vote相同(实现也是基本相同), 区别是所有的信息在`votes4ram`表中
2. 注意用户必须保证当前action执行后其账户RAM额度大于等于使用的RAM, 否则无法成功, 也就是说, 要撤回所有RAM的投票, 必须释放所有使用的内存.

实例:

```bash
./cleos -u https://w1.eosforce.cn:443 push action eosio vote4ram '{"voter":"teste","bpname":"biosbpa","stake":"1500.0000 EOS"}' -p teste
executed transaction: aba3997b8695ecd79e21cae1a35a3ee9229b122e1cf1f24ccaab2777b432fcc7  144 bytes  301 us
#         eosio <= eosio::onfee                 {"actor":"teste","fee":"0.0500 EOS","bpname":""}
#         eosio <= eosio::vote4ram              {"voter":"teste","bpname":"biosbpa","stake":"1500.0000 EOS"}
warning: transaction executed locally, but may not be confirmed by the network yet         ]
```

### 3.7 赎回投票

如上所述, 当撤回活期或定期投票后, 用户投票的token会进入unstaking状态,
当当前块高度大于unstake_height后, 可以使用`unfreeze` action将投票赎回.

```cpp
   [[eosio::action]] void unfreeze( const account_name& voter, const account_name& bpname );
```

参数:

- voter : 投票者
- bpname : 要撤回投票所属的节点

最小权限:

- voter@active

示例:

```bash
./cleos -u https://w1.eosforce.cn:443 push action eosio unfreeze '{"voter":"testd", "bpname":"biosbpa"}' -p testd
executed transaction: e0b52ec1a26008f3ca59c87d0521b2bab21a13e1d5b8abb35ab9cd43f5262ee1  128 bytes  610 us
#         eosio <= eosio::onfee                 {"actor":"testd","fee":"0.0300 EOS","bpname":""}
#         eosio <= eosio::unfreeze              {"voter":"testd","bpname":"biosbpa"}
warning: transaction executed locally, but may not be confirmed by the network yet         ]
```

### 3.8 赎回租赁内存的投票

当撤回用于抵扣RAM租金的投票之后, 用户投票的token会进入unstaking状态,
当当前块高度大于unstake_height后, 可以使用`unfreezeram` action将投票赎回.

注意, 这里所有信息是在`votes4ram_table`表中.

```cpp
   [[eosio::action]] void unfreezeram( const account_name& voter, const account_name& bpname );
```

参数:

- voter : 投票者
- bpname : 要撤回投票所属的节点

最小权限:

- voter@active

示例:

```bash
./cleos -u https://w1.eosforce.cn:443 push action eosio unfreezeram '{"voter":"testd", "bpname":"biosbpa"}' -p testd
executed transaction: e0b52ec1a26008f3ca59c87d0521b2bab21a13e1d5b8abb35ab9cd43f5262ee1  128 bytes  610 us
#         eosio <= eosio::onfee                 {"actor":"testd","fee":"0.0300 EOS","bpname":""}
#         eosio <= eosio::unfreezeram           {"voter":"testd","bpname":"biosbpa"}
warning: transaction executed locally, but may not be confirmed by the network yet         ]
```

## 4. BP相关

以下是bp相关的action:

### 4.1 注册BP

账户可以通过`updatebp`注册为bp, 如果BP已经注册则会修改BP信息.

```cpp
   [[eosio::action]] void updatebp( const account_name& bpname,
                                    const public_key& block_signing_key,
                                    const uint32_t commission_rate,
                                    const std::string& url );
```

参数:

- bpname : BP名字
- block_signing_key : 出块签名公钥, 用于出块
- commission_rate : 分配率
- url : bp节点URL信息

最小权限:

- voter@active

示例:

```bash
./cleos -u https://w1.eosforce.cn:443 push action eosio updatebp \
'{ "bpname":"testd","block_signing_key":"EOS83e8NsiUvAi4aCePsGsbyxxiwQomsqcz2cLnHwCNjtUQVmDy3c","commission_rate":1,"url":"http://eosforce.io" }' -p testd
executed transaction: 35b43489ce9743ea74192cec6b1f8ddff9a94270f4090fd56d73c218eafcf9e8  176 bytes  236 us
#         eosio <= eosio::onfee                 {"actor":"testd","fee":"100.0000 EOS","bpname":""}
#         eosio <= eosio::updatebp              {"bpname":"testd","block_signing_key":"EOS83e8NsiUvAi4aCePsGsbyxxiwQomsqcz2cLnHwCNjtUQVmDy3c","commi...
warning: transaction executed locally, but may not be confirmed by the network yet         ]
```

### 4.2 BP心跳

为了保证EOSForce主网的稳定性, EOSForce系统要求节点向链提交心跳Action,
每个BP每个十分钟执行一次`heartbeat` action, 通常这个操作是基于heartbeat_plugin操作的, 以示为BP的节点正在运行,
虽然bp可以通过开发一个定时程序完成相似的功能, 但是其综合成本将会高于启动实际的节点, 所以这一方式可以有效的促进整个网络的稳定.

如果节点没有按时完成`heartbeat`, 系统将会扣除其收益.

BP账户因为涉及到奖励代币, 所以往往不会暴露私钥到某种形式的“热”钱包中, 甚至于不会使用BP账户做操作.
所以`heartbeat`不能直接基于BP账户来操作, 所以需要BP建立一个以出块公钥账户权限的账户用来执行`heartbeat`,

配置 heartbeat_plugin 方式如下:

这里假设BP的账户名为 bp.name, 出块签名公钥为 EOS83e8NsiUvAi4aCePsGsbyxxiwQomsqcz2cLnHwCNjtUQVmDy3c,
BP建立一个bp.ping账户, 其公钥为 EOS83e8NsiUvAi4aCePsGsbyxxiwQomsqcz2cLnHwCNjtUQVmDy3c, 用来发送心跳.

```bash
bp-mapping=bp.name=KEY:bp.ping
plugin=eosio::heartbeat_plugin
```

> 注意: 因为心跳同样需要手续费, 所以节点需要保证心跳发送账户的代币是否足够.

```cpp
   [[eosio::action]] void heartbeat( const account_name& bpname,
                                     const time_point_sec& timestamp );
```

参数:

- bpname : BP名字
- time_point_sec : 未使用参数, 后续可以用来链上时间校准, 当前未生效

最小权限:

- bppingname@active

### 4.3 节点惩罚

节点惩罚相关功能是在押金制度和节点监控等相关功能的基础上实现的。

### 4.3.1 押金制度

出块节点和收益节点需要缴纳一部分币作为押金才能获得收益，押金的合约是eosio.pledge，押金的类型是block.out，所有节点需要向eosio.pledge转账最少12522个EOSC 才能获得收益

```bash
../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://127.0.0.1:8001 push action eosio.pledge open '["block.out","biosbpa","testopen"]' -p biosbpa
../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://127.0.0.1:8001 transfer biosbpa eosio.pledge "20000.0000 EOS" "block.out"
```

block.out 的押金类型的信息如下

```bash
{
      "pledge_name": "block.out",
      "deduction_account": "eosio",
      "pledge": "2.0000 EOS"
    }
```

deduction_account 是eosio 也就是拥有扣除押金和分配罚金的权限只有eosio有

### 4.3.2 节点监控

节点监控功能是监控节点是否连续出块和记录节点漏块信息的功能，和押金制度结合以后每漏一个块就会扣除10.0000 EOSC 的押金
节点监控信息在表格bpmonitor上面

```bash
../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://127.0.0.1:8001 get table eosio eosio bpmonitor
...
{
      "bpname": "eosforce",
      "last_block_num": 0,
      "consecutive_drain_block": 18,
      "consecutive_produce_block": 0,
      "total_drain_block": 18,
      "stability": 1000,
      "bock_age": 0,
      "bp_status": 1,
      "end_punish_block": 0
    }
...

```

上面eosforce节点连续漏块18个，一共漏块18个，bp_status是1是可以惩罚的状态
节点历史漏块记录在表格drainblocks上面

```bash
../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://127.0.0.1:8001 get table eosio eosforce drainblocks
{
  "rows": [{
      "current_block_num": 1214,
      "drain_block_num": 26
    }
  ],
  "more": false
}
```

### 4.3.4 提议惩罚节点

在节点监控上面，如果一个BP连续漏块超过9个则会将BP 的bp_status设置为1，也就是待惩罚状态。用户可以提议惩罚该节点，惩罚节点的功能是punishbp，提议惩罚节点的用户需要在block.out上面有100 EOSC 的押金

```bash
../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://127.0.0.1:8001 push action eosio punishbp '{"bpname":"eosforce","proposaler":"biosbpa"}' -p biosbpa
```

一个节点每次只能有一个惩罚的议案，提议惩罚以后会在表格punishbps上面添加被提议惩罚的节点以及通过的节点以及实效的块高度

```bash
../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://127.0.0.1:8001 get table eosio eosio punishbps
{
  "rows": [{
      "punish_bp_name": "eosforce",
      "proposaler": "biosbpa",
      "approve_bp": [],
      "effective_block_num": 30155
    }
  ],
  "more": false
}
```

上面表格显示biosbpa提议惩罚eosforce,该提议将在30155块高度以后失效。

### 4.3.5 同意惩罚节点

节点被提议惩罚以后需要16个出块节点同意该节点才能被惩罚。同意惩罚节点的功能是approvebp

```bash
../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://127.0.0.1:8001 push action eosio approvebp '{"bpname":"eosforce","approver":"biosbpa"}' -p biosbpa
```

当第十六个出块节点同意以后被惩罚的节点将自动受到惩罚，合约会自动分配该节点的罚金奖励给提议惩罚该节点和同意惩罚该节点的BP(如果该节点没有缴纳罚金，则不会有任何奖励,分配的比例是：提出惩罚的人分得罚金的一半，其余同意的节点平分剩下的一半)

### 4.3.6 被惩罚节点恢复

节点被惩罚的时候会将bp_status设置为2，也就是正在被惩罚状态，end_punish_block是可以结束惩罚的块高度，当达到这个块高度以后被惩罚的节点可以通过bailpunish 将自己恢复正常

```bash
../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://127.0.0.1:8001 get table eosio eosio bpmonitor
...
{
      "bpname": "eosforce",
      "last_block_num": 0,
      "consecutive_drain_block": 15,
      "consecutive_produce_block": 0,
      "total_drain_block": 41,
      "stability": 1000,
      "bock_age": 0,
      "bp_status": 2,
      "end_punish_block": 30310
    }
...
```

```bash
../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://127.0.0.1:8001 push action eosio bailpunish '["eosforce"]' -p eosforce
```

### 4.3.7 即将被惩罚节点恢复

节点连续漏够9个块以后bpmonitor上面的bp_status状态会设置为1，即BP可以被惩罚，如果BP能正常出块则可以修改这个状态，将这个状态恢复成0.

```bash
../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://127.0.0.1:8001 push action eosio removepunish '["biosbpg"]' -p biosbpg
```

## 4.4 分红改动

+ 增加出块分红，每个块0.5个EOSC打入出块分红池中
+ 投票分红每个块增加到3 EOSC 个，其中0.5个EOSC作为收益分红是分给节点的，2.5个EOSC打入投票分红池中

### 4.4.1 BP领取出块分红和收益分红

```bash
../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://127.0.0.1:8001 push action eosio bpclaim '["biosbpa"]' -p biosbpa
```

**BP这部分分红只有超过100 EOSC 才能领取**
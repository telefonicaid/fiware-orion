# データベース・インデックス分析

以下の分析は、Orion Context Broker データベース内のさまざまなインデックスおよびエンティティの数が異なる構成の TPS (1秒あたりの変換) および、ストレージ消費量を示しています。この分析では Orion 0.14.0 を使用しました。各トランザクションは、1つのエンティティで構成され、そのエンティティをクエリしたり更新したりします。

この情報は、特定の設定でどのインデックスを使用するかについてのヒントとしてのみ提供されていますが、特定の環境の結果は、ハードウェアのプロファイル、この特定のケースでは、テスト対象のシステムのリソース (VMware ベースの VM) は次のとおりです。Intel Xeon E5620@2.40GHz と 4GB RAM を搭載した物理ホストです。両方の Orion MongoDB は同じ VM で動作します。負荷を生成するツールは、[次の場所](https://github.com/telefonicaid/fiware-orion/tree/master/test/LoadTest) (orionPerformanceOnlyQueries\_v2.0.jmx, orionPerformanceOnlyAppends\_v2.0.jmx と orionPerformanceAppendsAndUpdates\_v2.0.jmx) にあり、別の VM で実行されているが同じサブネット、つまり、テスト対象のシステムとの L2 接続で実行される構成を使用する JMeter です。

テストケースは、エンティティのクエリ、エンティティの作成、作成と更新の組み合わせです。

スループット (TPS):


| Case - Indexes                                           |  10,000 entities  | 100,000 entities   | 1,000,000 entities |
|:---------------------------------------------------------|:----------------- |:------------------ |:------------------ |
| Query - none                                             | 115.3             | 12.2               | 2                  |
| Query - `_id.id`                                         | 2271.2            | 2225.7             | 2187.7             |
| Query - `_id.type`                                       | 40                | 4.6                | 1.8                |
| Query - separated `_id.id` and `_id.type`                | 2214.7            | 2179.3             | 2197.1             |
| Query - compound `{_id.id,_id.type}`                     | 2155.5            | 2174.4             | 2084.4             |
| Creation - none                                          | 64.5              | 17.8               | 2.4                |
| Creation - `_id.id`                                      | 748.2             | 672.9              | 698.3              |
| Creation - `_id.type`                                    | 33.5              | 4.9                | 2.1                |
| Creation - separated `_id.id` and `_id.type`             | 774.4             | 703.9              | 691.9              |
| Creation - compound `{_id.id,_id.type}`                  | 784.6             | 721.1              | 639.2              |
| Creation and update - none                               | 102.1             | 15.5               | 3.3                |
| Creation and update - `_id.id`                           | 1118.1            | 798.1              | 705.5              |
| Creation and update - `_id.type`                         | 32.6              | 4.8                | 1.8                |
| Creation and update - separated `_id.id` and `_id.type`  | 1145.3            | 746.4              | 706.5              |
| Creation and update - compound `{_id.id,_id.type}`       | 1074.7            | 760.7              | 636.1              |

ストレージ :

| Case - Indexes                                         | Index size (MB) |  Index size / DB file size   |
|:-------------------------------------------------------|:--------------- |:---------------------------- |
| 10,000 entities - none (\*)                            | 0.88            | 0.004                        |
| 10,000 entities - `_id.id`                             | 1.17            | 0.006                        |
| 10,000 entities - `_id.type`                           | 1.11            | 0.005                        |
| 10,000 entities - separated `_id.id` and `_id.type`    | 1.40            | 0.007                        |
| 10,000 entities - compound `{_id.id`,`_id.type}`       | 1.23            | 0.006                        |
| 100,000 entities - none (\*)                           | 8               | 0.041                        |
| 100,000 entities -  `_id.id`                           | 11              | 0.057                        |
| 100,000 entities -  `_id.type`                         | 10              | 0.052                        |
| 100,000 entities -  separated `_id.id` and `_id.type`  | 13              | 0.067                        |
| 100,000 entities -  compound `{_id.id`,`_id.type}`     | 12              | 0.062                        |
| 1,000,000 entities - none (\*)                         | 124             | 0.077                        |
| 1,000,000 entities - `_id.id`                          | 154             | 0.076                        |
| 1,000,000 entities - `_id.type`                        | 145             | 0.073                        |
| 1,000,000 entities - separated `_id.id` and `_id.type` | 175             | 0.088                        |
| 1,000,000 entities - compound (`_id.id`,`_id.type}`    | 161             | 0.079                        |

(\*) インデックスは設定しなくても、MongoDB は常に `_id` に インデックスを設定します。したがって、スペースの一部は常にインデックスに割り当てられます。

**ヒント :** 上記の情報を考慮する `_id.id` と、エンティティ・コレクションにインデックスを設定することをお勧めします。

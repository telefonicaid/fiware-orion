# エンプティ型の使用 (Using empty types)

NGSI9/NGSI10 オペレーションのエンティティでエンプティ型 (empty types) を使用できます。実際には、コンビニエンス・オペレーションでは、デフォルトでこの方法でエンプティ型を暗黙的に使用します。型を指定するには、コンビニエンス・オペレーション URLs の中の `<id>` ではなく、`/type/<type>/id/<id>` パターンを使用できます。

さらに、コンテキストのアベイラビリティのディスカバーまたはコンテキストのクエリのオペレーションで、空のエンティティ型を使用できます。この場合、クエリに型がないと、" 任意の型 (any type)" と解釈されます。

たとえば、Orion Context Broker で次のコンテキストを使用することを検討してみましょう :

-   Entity 1:
    -   ID: Room1
    -   Type: Room
-   Entity 2:
    -   ID: Room1
    -   Type: Space

discoveryContextAvailability/querycontext の使用 :

```
  ...
  "entities": [
      {
          "type": "",
          "isPattern": "false",
          "id": "Room1"
      }
  ]
  ...
```

エンティティ1とエンティティ2の両方に一致します。

属性に関しては、updateContext APPEND に型なしで作成することができます。後続の updateContext UPDATE で属性型が空のままの場合、型は更新されず、属性は以前の型を保持します。

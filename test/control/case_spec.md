## control testcase spec
### tri_if_001
``` c
int getVal (int a, int b) {
  int res = (a + 1) * b;
  return res;
}
int main (void) {
  int res = 0;
  int cond = getVal(7, 10);
  if(cond > 10) {
    if (cond > 20) {
      if (cond > 30) {
        res = 30;
      } else {
        res = 25;
      }
    } else {
      res = 15;
    }
  }else {
    res = 5;
  }
  return res;
}
```

``` wasm
(module
 (table 0 anyfunc)
 (memory $0 1)
 (export "memory" (memory $0))
 (export "getVal" (func $getVal))
 (export "main" (func $main))
 (func $getVal (; 0 ;) (param $0 i32) (param $1 i32) (result i32)
  (i32.mul
   (i32.add
    (get_local $0)
    (i32.const 1)
   )
   (get_local $1)
  )
 )
 (func $main (; 1 ;) (result i32)
  (local $0 i32)
  (local $1 i32)
  (set_local $1
   (i32.const 5)
  )
  (block $label$0
   (br_if $label$0
    (i32.lt_s
     (tee_local $0
      (call $getVal
       (i32.const 7)
       (i32.const 10)
      )
     )
     (i32.const 11)
    )
   )
   (set_local $1
    (i32.const 15)
   )
   (br_if $label$0
    (i32.lt_s
     (get_local $0)
     (i32.const 21)
    )
   )
   (set_local $1
    (select
     (i32.const 30)
     (i32.const 25)
     (i32.gt_s
      (get_local $0)
      (i32.const 30)
     )
    )
   )
  )
  (get_local $1)
 )
)

```
### tri_if_002
``` c
int getVal (int a, int b) {
  int res = (a + 1) * b;
  return res;
}
int main (void) {
  int res = 0;
  int cond = getVal(4, 10);
  if(cond > 100) {
    res = 100;
  } else if (cond > 90) {
    res = 90;
  } else if ( cond > 80) {
    res = 80;
  } else {
    res = 60;
  }
  return res;
}
```

``` wasm
(module
 (table 0 anyfunc)
 (memory $0 1)
 (export "memory" (memory $0))
 (export "getVal" (func $getVal))
 (export "main" (func $main))
 (func $getVal (; 0 ;) (param $0 i32) (param $1 i32) (result i32)
  (i32.mul
   (i32.add
    (get_local $0)
    (i32.const 1)
   )
   (get_local $1)
  )
 )
 (func $main (; 1 ;) (result i32)
  (local $0 i32)
  (local $1 i32)
  (set_local $1
   (i32.const 100)
  )
  (block $label$0
   (br_if $label$0
    (i32.gt_s
     (tee_local $0
      (call $getVal
       (i32.const 4)
       (i32.const 10)
      )
     )
     (i32.const 100)
    )
   )
   (set_local $1
    (i32.const 90)
   )
   (br_if $label$0
    (i32.gt_s
     (get_local $0)
     (i32.const 90)
    )
   )
   (set_local $1
    (select
     (i32.const 80)
     (i32.const 60)
     (i32.gt_s
      (get_local $0)
      (i32.const 80)
     )
    )
   )
  )
  (get_local $1)
 )
)

```

### loop_001
``` c
int getVal (int a, int b) {
  int res = (a + 1) * b;
  return res;
}
int main (void) {
  int res = 0;
  int cond = getVal(4, 10);
  for (int i = 0; i < cond; i++) {
    res = 10 * i + 1;
  }
  return res;
}
```

``` wasm
(module
 (table 0 anyfunc)
 (memory $0 1)
 (export "memory" (memory $0))
 (export "getVal" (func $getVal))
 (export "main" (func $main))
 (func $getVal (; 0 ;) (param $0 i32) (param $1 i32) (result i32)
  (i32.mul
   (i32.add
    (get_local $0)
    (i32.const 1)
   )
   (get_local $1)
  )
 )
 (func $main (; 1 ;) (result i32)
  (local $0 i32)
  (block $label$0
   (br_if $label$0
    (i32.lt_s
     (tee_local $0
      (call $getVal
       (i32.const 4)
       (i32.const 10)
      )
     )
     (i32.const 1)
    )
   )
   (return
    (i32.or
     (i32.add
      (i32.mul
       (get_local $0)
       (i32.const 10)
      )
      (i32.const -10)
     )
     (i32.const 1)
    )
   )
  )
  (i32.const 0)
 )
)
```
### switch_case_001
``` c
int getVal (int a, int b) {
  int res = (a + 1) * b;
  return res;
}
int main (void) {
  int res = 0;
  int cond = getVal(4, 10);
  switch (cond) {
    case 10:
      res = 10;
      break;
    case 20:
      res = 21;
      break;
    case 30:
      res = 32;
      break;
    case 40:
    case 50:
    case 60:
       res = 65;
       break;
    case 70:
       res = 79;
       break;
    default:
       res = 80;
  }
  return res;
}
```

``` wasm
(module
 (table 0 anyfunc)
 (memory $0 1)
 (export "memory" (memory $0))
 (export "getVal" (func $getVal))
 (export "main" (func $main))
 (func $getVal (; 0 ;) (param $0 i32) (param $1 i32) (result i32)
  (i32.mul
   (i32.add
    (get_local $0)
    (i32.const 1)
   )
   (get_local $1)
  )
 )
 (func $main (; 1 ;) (result i32)
  (local $0 i32)
  (local $1 i32)
  (set_local $1
   (i32.const 10)
  )
  (block $label$0
   (block $label$1
    (block $label$2
     (block $label$3
      (block $label$4
       (br_if $label$4
        (i32.gt_u
         (tee_local $0
          (i32.add
           (call $getVal
            (i32.const 4)
            (i32.const 10)
           )
           (i32.const -10)
          )
         )
         (i32.const 60)
        )
       )
       (block $label$5
        (br_table $label$3 $label$4 $label$4 $label$4 $label$4 $label$4 $label$4 $label$4 $label$4 $label$4 $label$2 $label$4 $label$4 $label$4 $label$4 $label$4 $label$4 $label$4 $label$4 $label$4 $label$0 $label$4 $label$4 $label$4 $label$4 $label$4 $label$4 $label$4 $label$4 $label$4 $label$5 $label$4 $label$4 $label$4 $label$4 $label$4 $label$4 $label$4 $label$4 $label$4 $label$5 $label$4 $label$4 $label$4 $label$4 $label$4 $label$4 $label$4 $label$4 $label$4 $label$5 $label$4 $label$4 $label$4 $label$4 $label$4 $label$4 $label$4 $label$4 $label$4 $label$1 $label$3
         (get_local $0)
        )
       )
       (return
        (i32.const 65)
       )
      )
      (set_local $1
       (i32.const 80)
      )
     )
     (return
      (get_local $1)
     )
    )
    (return
     (i32.const 21)
    )
   )
   (return
    (i32.const 79)
   )
  )
  (i32.const 32)
 )
)
```

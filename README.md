# aof-selector
[![Build Status](https://travis-ci.org/hongliuliao/aof-selector.svg?branch=master)](https://travis-ci.org/hongliuliao/aof-selector)
A aof file selector tool, support filter by aof element

## Feature
* Support filter by aof element index
* Output by line model

## Performance
 * 100 M/s

## Build && Test
```
 # build
 [liao@localhost aof-selector]$ make # build

 # Select all set cmds in aof (0 is element idx)
 [liao@localhost aof-selector]$ cat data/appendonly.aof | ./output/bin/aof-selector -w 0 -i SET
 SET key:000009085953 xxxxxxxxxxxxxxxxxxxx 
 SET key:000007681641 xxxxxxxxxxxxxxxxxxxx 
 SET key:000007495630 xxxxxxxxxxxxxxxxxxxx 
 SET key:000009670031 xxxxxxxxxxxxxxxxxxxx 
 SET key:000006726331 xxxxxxxxxxxxxxxxxxxx 
 SET key:000000283096 xxxxxxxxxxxxxxxxxxxx 
 SET key:000006655125 xxxxxxxxxxxxxxxxxxxx 
```

## How to use
```
[liao@localhost aof-selector]$ ./output/bin/aof-selector -h
usage: aof-selector [-s element_idx] [-w element_idx] [-i element_vals]
example: cat appendonly.aof | aof-selector -s 0 -w 0 -i set,del

    -s select element
    -w match element
    -i match element vals, split by ','
```

## Aof element idx
```
*3
$5
LPUSH      ===> aof element idx: 0
$6
mylist     ===> aof element idx: 1
$20
xxxxx      ===> aof element idx: 2
```


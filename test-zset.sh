#!/bin/bash

redis="../redis/src/redis-cli -p 1111"

$redis ZADD x 0 a
$redis ZADD x 1 b
$redis ZADD x 2 c
$redis ZADD x 3 d
$redis ZADD x 4 e
$redis ZADD x 5 f
$redis ZADD x 6 g
$redis ZADD x 7 h
$redis ZADD x 8 i
$redis ZADD x 9 j

$redis ZCOUNT x 3 7

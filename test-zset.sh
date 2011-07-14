#!/bin/bash

redis="../redis/src/redis-cli -p 1111"

$redis ZADD x 1 a
$redis ZADD x 2 b
$redis ZADD x 3 c
$redis ZADD x 4 d
$redis ZADD x 5 e
$redis ZADD x 6 f
$redis ZADD x 7 g
$redis ZADD x 8 h
$redis ZADD x 9 i
$redis ZADD x 10 j

$redis ZCOUNT x 3 7	# expecting 5
$redis ZCOUNT x 20 20	# expecting 0
$redis ZCOUNT x 0 0	# expecting 0
$redis ZCOUNT x 1 10	# expecting 10

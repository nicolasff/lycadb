About
=====

LycaDB is a simple key-value store, created to experiment with [HailDB](http://www.haildb.com/). It is only a demo and is not really usable at the moment.

Redis
=====

[Redis](http://redis.io/) is a very fast in-memory key-value store. It maps keys to [data structures](http://redis.io/topics/introduction), and provides commands to operate on those data structures (e.g. add an element to a list, intersect sets of items, etc.)

Historically, Redis has been able persist data to disk in two ways:

* By fork()ing the server and using the child process to dump the whole dataset to disk, periodically. This can consume a lot of RAM.
* By writing each command to a log file, which can be replayed on restart. In order to avoid ending up with a very large log file, it can be rewritten on demand by fork()ing the process or restarting the server.

This usage of fork() makes Redis very simple but has the inconvenient of using [up to twice](http://en.wikipedia.org/wiki/Copy-on-write#Copy-on-write_in_virtual_memory) the amount of RAM as the process is duplicated entirely. This issue [has been discussed in the past](http://blog.kennejima.com/post/1226487020/thoughts-on-redis), and [acknowledged by the author of Redis](http://antirez.com/post/a-few-key-problems-in-redis-persistence.html).

In order to use datasets larger than the available RAM, several options are now available:

* Shard the data and use several instances, this is what most people seem to do.
* Use the [Virtual Memory (VM)](http://antirez.com/post/redis-virtual-memory-story.html): Let Redis swap cold keys on disk, whilst keeping the hottest keys in a fixed-size buffer in RAM. The idea behind this implementation is that Redis knows better than the OS which pages can be swapped. Using the VM is now recommended against.
* Use the *diskstore* mechanism: Each key is hashed, and stored in a single file. Because of the lack of a replay log, [MULTI/EXEC transactions](http://redis.io/topics/transactions) and operations on multiple keys are no longer atomic. My main issue with diskstore is the way keys are saved: each key is completely serialized before being written to its file and O(1) operations such as SADD (adding an element to a set) become O(n) as *the whole set* is rewritten to disk.
* Use a B-tree, [currently in the making](https://github.com/antirez/otree); this is not yet usable.


Redis is very good at being an in-memory data store, and it seems that this design choice will continue to be an important selling point. Its various data structures make it a brilliant cache, and this is what I feel is the best use-case. But if you need to store more data than your RAM allows for, be prepared to hit the disk at some point.

Introducing HailDB
==================

HailDB is an updated version of Embedded InnoDB. InnoDB is currently the default storage engine for MySQL.  
LycaDB uses the Redis protocol to expose a few commands and stores data directly in InnoDB tables. A single database is created, with only two tables:

* `main` contains key/value pairs (GET/SET/INCR...)
* `sets` contains [sets](http://en.wikipedia.org/wiki/Set_\(mathematics\)) (SADD/SREM/SMEMBERS...)


Q & A
=====

* Q: Why wasn’t LycaDB added directly to Redis? A: The current implementation of *diskstore* serializes the whole key after each write. This is inefficient for operations such as [SADD](http://redis.io/commands/sadd), which can remain O(1) with a dedicated table. I also haven’t figured out a way to store lists efficiently, and I’m not sure that the licenses are compatible. My aim is not to replace Redis, but to work on a very simple project until I get it right.
* Q: What commands have been implemented? A: GET, SET, DEL, INCR, DECR, SADD, SISMEMBER, SMEMBERS, SREM.
* Q: Why are so many types and commands missing? A: This is only a demo at this point.
* Q: Is it possible to keep the amazing performance of Redis whilst storing data in HailDB? A: Probably not.
* Q: Can LycaDB read .rdb files? .aof files? A: No and no.
* Q: HailDB is licensed under the GPL, will it be usable with Redis? A: IANAL, but I think not. I’ve read proposals for a Redis plugin system which use UNIX pipes to send data to the storage engine, that would probably be ok.
* Q: Does LycaDB use code from Redis? A: No.
* Q: Why was the Redis protocol used? A: I wanted to compare the two with the same tool, namely a modified `redis-benchmark`.
* Q: What software license is LycaDB released under? A: GPL v2.
* Q: Why not Tokyo Cabinet? A: I have no experience with it, and thought that the Redis data structures could be mapped to tables easily. I would be interested to see the same kind of project with Tokyo Cabinet.
* Q: Is there even a use-case for LycaDB? A: I’m not sure. Maybe as a slave to a Redis master, accepting operations only on certain key patterns? Maybe as a separate store, dedicated to large amounts of persistent data?


Compilation
===========

LycaDB depends on [haildb](http://www.haildb.com/) and [libevent](http://monkey.org/~provos/libevent/). On Ubuntu, you can install them by running `sudo apt-get install libhaildb6 libhaildb-dev libevent-1.4-2 libevent-dev`.

To compile, run `make`. HailDB files are created in /tmp.  
Run LycaDB using `./lycadb`, and connect using a Redis client on port 1111.

Benchmarks
==========

I have tried to benchmark LycaDB using different workloads, and have seen variable results. For insertion benchmarks, whilst Redis performs as well independently of the ordering of the keys this is apparently not the case with HailDB (my guess is that this is mostly due to the index rebalancing and to increased amounts of random IO). Reads are also faster when executed in order, due to the way data is stored.

I think it is important to compare similar systems: benchmarking writes to disk with LycaDB against writes to memory with Redis is not helpful, especially since LycaDB will keep accepting writes after the buffer pool is filled whereas Redis will start rejecting writes.

10M writes (SET) followed by 10M reads (GET) in random order:
![bench](http://i.imgur.com/EVcnv.png)


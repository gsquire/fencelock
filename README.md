# fencelock
This is a simple Redis module that implements a distributed lock with a fencing token as described
in [this blog post](https://martin.kleppmann.com/2016/02/08/how-to-do-distributed-locking.html).

## Install
There is a simple Makefile provided that has a few targets that are described below:
- `module`: This will build a shared object file that Redis can load at startup. It is the default
target.
- `fmt`: This will format the source code using GNU indent.
- `clean`: This will remove the shared object file as well as any backup files left behind by GNU
indent.

## API
There are two commands implemented by this module.

- `FENCELOCK.ACQUIRE $LOCK_NAME $EXPIRATION`
This command takes the desired lock name as well as an expiration in seconds. Upon a successful
acquire of the lock, it will return a secret value and a fencing token. The fencing token is
described below in the implementation section. The secret value is used in the release command to
prevent erroneous releases by other clients. If the lock has already been acquired by another
client, this command returns the Redis null bulk reply.

- `FENCELOCK.RELEASE $LOCK_NAME $SECRET`
This command takes the desired lock name as well as the current secret value associated with the
lock. The secret value is returned upon a successful acquire of the lock. It will simply delete the
lock if it has not already expired.

## Implementation
The idea behind the fencing token is to prevent any issues where two clients have a situation
similar to this:
1. Client A acquires the lock with an expiration value of 5 seconds.
2. Client B tries to acquire the lock but it can not so it blocks.
3. Client A blocks for some reason (GC, etc.) and the lock expires.
4. Client B acquires the lock and is able to submit work to the shared resource.
5. Client A still believes it is holding the lock and submits work to the shared resource. However
it is out of date and thus we have a data race.

With a fencing token, this problem is solved by supplying a monotonically increasing value. In this
case we just use a 64 bit integer that is returned to the client. Therefore when trying to submit
work, the shared resource can check the fencing token and reject any value lower than the maximum
it has seen.

This module works best on a single instance Redis deploy. It shouldn't be used for ensuring
absolute correctness but rather efficiency over a shared resource. There are better solutions for
ensuring correctness such as Zookeeper or other databases with transactional guarantees.

## License
fencelock is released under the MIT license along with the Redis license that can be found in the
REDIS-LICENSE file.

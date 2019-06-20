#include <stdlib.h>
#include <time.h>

#include "redismodule.h"

/* Since Redis is single threaded, these values don't need synchronization. */
static long long fenceToken = 0;
static uint32_t	curValue = 0;

int
Acquire_RedisCommand(RedisModuleCtx * ctx, RedisModuleString ** argv, int argc)
{
	const int	MILLIS = 1000;

	if (argc != 3) {
		return RedisModule_WrongArity(ctx);
	}
	RedisModule_AutoMemory(ctx);

	int		value = rand();

	RedisModuleKey *key = RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);
	int		ty = RedisModule_KeyType(key);
	/* The case where the lock is already held. */
	if (ty != REDISMODULE_KEYTYPE_EMPTY) {
		return RedisModule_ReplyWithNull(ctx);
	}
	long long	expire;
	if (RedisModule_StringToLongLong(argv[2], &expire) == REDISMODULE_ERR) {
		return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
	}
	RedisModuleString *v = RedisModule_CreateStringFromLongLong(ctx, value);
	RedisModule_StringSet(key, v);
	RedisModule_SetExpire(key, expire * MILLIS);

	/* Track our current secret for any release invocations. */
	curValue = value;

	RedisModule_ReplyWithArray(ctx, 2);
	RedisModule_ReplyWithLongLong(ctx, value);
	return RedisModule_ReplyWithLongLong(ctx, fenceToken++);
}

int
Release_RedisCommand(RedisModuleCtx * ctx, RedisModuleString ** argv, int argc)
{
	if (argc != 3) {
		return RedisModule_WrongArity(ctx);
	}
	RedisModule_AutoMemory(ctx);

	long long	value;
	if (RedisModule_StringToLongLong(argv[2], &value) == REDISMODULE_ERR) {
		return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
	}
	if (value != curValue) {
		return RedisModule_ReplyWithNull(ctx);
	}
	/*
	 * The caller supplied the correct random value, so delete the key.
	 */
	RedisModuleKey *key = RedisModule_OpenKey(ctx, argv[1], REDISMODULE_WRITE);
	int		ty = RedisModule_KeyType(key);
	if (ty == REDISMODULE_KEYTYPE_EMPTY) {
		return RedisModule_ReplyWithError(ctx, "ERR lock not found");
	} else if (ty != REDISMODULE_KEYTYPE_STRING) {
		return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
	}
	RedisModule_DeleteKey(key);
	return RedisModule_ReplyWithSimpleString(ctx, "OK");
}

int
RedisModule_OnLoad(RedisModuleCtx * ctx, RedisModuleString ** argv, int argc)
{
	REDISMODULE_NOT_USED(argv);
	REDISMODULE_NOT_USED(argc);
	srand(time(NULL));

	if (RedisModule_Init(ctx, "fencelock", 1, REDISMODULE_APIVER_1) == REDISMODULE_ERR) {
		return REDISMODULE_ERR;
	}
	if (RedisModule_CreateCommand(ctx, "fencelock.acquire", Acquire_RedisCommand,
				"fast write", 0, 0, 0) == REDISMODULE_ERR) {
		return REDISMODULE_ERR;
	}
	if (RedisModule_CreateCommand(ctx, "fencelock.release", Release_RedisCommand,
				"fast write", 0, 0, 0) == REDISMODULE_ERR) {
		return REDISMODULE_ERR;
	}
	return REDISMODULE_OK;
}

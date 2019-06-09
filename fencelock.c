#include <stdlib.h>

#include "redismodule.h"

/* Since Redis is single threaded, these values don't need synchronization. */
static long long fenceToken = 0;
static uint32_t	curValue = 0;

int
Acquire_RedisCommand(RedisModuleCtx * ctx, RedisModuleString ** argv, int argc)
{
	if (argc != 3) {
		return RedisModule_WrongArity(ctx);
	}
	uint32_t	value = arc4random();

	RedisModuleCallReply *reply =
	RedisModule_Call(ctx, "SET", "slccs", argv[1], value, "NX", "EX", argv[2]);
	/* The case where the lock is already held. */
	if (RedisModule_CallReplyType(reply) == REDISMODULE_REPLY_NULL) {
		RedisModule_FreeCallReply(reply);
		return RedisModule_ReplyWithNull(ctx);
	}
	RedisModule_FreeCallReply(reply);

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
	RedisModuleCallReply *reply = RedisModule_Call(ctx, "DEL", "s", argv[1]);
	RedisModule_ReplyWithCallReply(ctx, reply);
	RedisModule_FreeCallReply(reply);
	return REDISMODULE_OK;
}

int
RedisModule_OnLoad(RedisModuleCtx * ctx, RedisModuleString ** argv, int argc)
{
	REDISMODULE_NOT_USED(argv);
	REDISMODULE_NOT_USED(argc);

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

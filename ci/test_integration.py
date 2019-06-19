#!/usr/bin/env python3

import logging

import pytest
import redis

LOG = logging.getLogger(__name__)

@pytest.fixture
def redis_conn():
    return redis.Redis(host='redis', port=6379)

def test_simple_acquire(redis_conn):
    data = redis_conn.execute_command('FENCELOCK.ACQUIRE a 5')
    assert data is not None

def test_second_acquire_fails(redis_conn):
    redis_conn.execute_command('FENCELOCK.ACQUIRE b 5')
    second_attempt = redis_conn.execute_command('FENCELOCK.ACQUIRE b 5')
    assert second_attempt is None

def test_acquire_release(redis_conn):
    lock = redis_conn.execute_command('FENCELOCK.ACQUIRE c 5')
    assert len(lock) == 2
    release_response = redis_conn.execute_command('FENCELOCK.RELEASE c {0}'.format(lock[0]))
    assert release_response == b'OK'

def test_incorrect_type(redis_conn):
    resp = redis_conn.execute_command('FENCELOCK.ACQUIRE d d')
    assert resp = b'ERR: WRONGTYPE Operation against a key holding the wrong kind of value'

--create table
CREATE TABLE gamestat.payevent (
    `eventtime` DateTime DEFAULT now(),
    `uid` Int64,
    `paytype` Enum8('Google' = 1, 'IOS' = 2, 'IOSSandbox' = 3),
    `orderid` String,
    `productid` String,
    `quantity` Int32,
    `balance` Int64,
    `ip` IPv4
) ENGINE = MergeTree PARTITION BY toYYYYMM(eventtime)
ORDER BY (eventtime, intHash32(uid)) SAMPLE BY intHash32(uid) TTL eventtime + toIntervalMonth(6) SETTINGS index_granularity = 8192
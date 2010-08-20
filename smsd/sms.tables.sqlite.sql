-- $ sqlite3 -init ./sms.tables.sqlite.sql smsd.db
create table inbox (
    "id" integer primary key,
    "number" text not null,
    "smsdate" text not null,
    "insertdate" text not null,
    "text" text,
    "phone" integer,
    "processed" integer default 0
);
create table outbox (
    "id" integer primary key,
    "number" text not null,
    "processed_date" text not null,
    "insertdate" text not null,
    "text" text,
    "phone" integer,
    "processed" integer default 0,
    "error" integer default -1 not null,
    "dreport" integer default 0 not null,
    "not_before" text default '00:00:00' not null,
    "not_after" text default '23:59:59' not null
);

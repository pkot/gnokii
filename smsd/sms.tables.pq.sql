\set ON_ERROR_STOP 1;

CREATE USER "smsd" WITH NOCREATEDB NOCREATEUSER;

CREATE DATABASE "smsd" WITH OWNER = "smsd";

\connect "smsd" "smsd"

COMMENT ON DATABASE "smsd" IS 'Gnokii SMSD Database';

CREATE TABLE "inbox" (
	"id" serial,
	"number" character varying(20) NOT NULL,
	"smsdate" timestamp NOT NULL,
	"insertdate" timestamp DEFAULT 'now' NOT NULL,
	"text" character varying(160),
	"processed" bool DEFAULT 'false',
	PRIMARY KEY ("id")
);

CREATE TABLE "outbox" (
	"id" serial,
	"number" character varying(20) NOT NULL,
	"processed_date" timestamp DEFAULT 'now' NOT NULL,
	"insertdate" timestamp DEFAULT 'now' NOT NULL,
	"text" character varying(160),
	"processed" bool DEFAULT 'false',
	"error" smallint DEFAULT '-1' NOT NULL,
	"dreport" smallint DEFAULT '0' NOT NULL,
	PRIMARY KEY ("id")
);

CREATE INDEX "outbox_processed_ix" ON "outbox" (processed);

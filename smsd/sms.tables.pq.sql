CREATE TABLE "inbox" (
	"id" serial,
	"number" character varying(20) NOT NULL,
	"smsdate" timestamp NOT NULL,
	"insertdate" timestamp DEFAULT 'now' NOT NULL,
	"text" character varying(160),
	"phone" integer,
	"processed" bool DEFAULT 'false',
	PRIMARY KEY ("id")
);

CREATE TABLE "outbox" (
	"id" serial,
	"number" character varying(20) NOT NULL,
	"processed_date" timestamp DEFAULT 'now' NOT NULL,
	"insertdate" timestamp DEFAULT 'now' NOT NULL,
	"text" character varying(160),
	"phone" integer,
	"processed" bool DEFAULT 'false',
	"error" smallint DEFAULT '-1' NOT NULL,
	"dreport" smallint DEFAULT '0' NOT NULL,
	PRIMARY KEY ("id")
);

CREATE TABLE "inbox" (
	"number" character varying(20) NOT NULL,
	"smsdate" timestamp NOT NULL,
	"insertdate" timestamp DEFAULT 'now' NOT NULL,
	"text" character varying(160),
	"processed" bool DEFAULT 'false',
	PRIMARY KEY ("number", "smsdate")
);
CREATE TABLE "outbox" (
	"id" serial,
	"number" character varying(20) NOT NULL,
	"insertdate" timestamp DEFAULT 'now' NOT NULL,
	"text" character varying(160),
	"processed" bool DEFAULT 'false'
);

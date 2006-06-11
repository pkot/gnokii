-- CREATE DATABASE smsd;

-- USE smsd;

CREATE TABLE inbox (
  id int(10) unsigned NOT NULL auto_increment,
  number varchar(20) NOT NULL default '',
  smsdate datetime NOT NULL default '0000-00-00 00:00:00',
  insertdate timestamp(14) NOT NULL,
  text varchar(160) default NULL,
  phone tinyint(4),
  processed tinyint(4) NOT NULL default '0',
  PRIMARY KEY  (id)
);

CREATE TABLE outbox (
  id int(10) unsigned NOT NULL auto_increment,
  number varchar(20) NOT NULL default '',
  processed_date timestamp(14) NOT NULL,
  insertdate timestamp(14) NOT NULL,
  text varchar(160) default NULL,
  phone tinyint(4),
  processed tinyint(4) NOT NULL default '0',
  error tinyint(4) NOT NULL default '-1',
  dreport tinyint(4) NOT NULL default '0',
  not_before time NOT NULL default '00:00:00',
  not_after time  NOT NULL default '23:59:59',
  PRIMARY KEY  (id)
);

-- CREATE INDEX outbox_processed_ix ON outbox (processed);
 
-- GRANT SELECT, INSERT, UPDATE, DELETE ON smsd.* TO smsd@localhost;

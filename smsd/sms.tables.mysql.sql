# MySQL dump 8.13
#
# Host: localhost    Database: sms
#--------------------------------------------------------
# Server version	3.23.36

#
# Table structure for table 'inbox'
#

CREATE TABLE inbox (
  id int(10) unsigned NOT NULL auto_increment,
  number varchar(20) NOT NULL default '',
  smsdate datetime NOT NULL default '0000-00-00 00:00:00',
  insertdate timestamp(14) NOT NULL,
  text varchar(160) default NULL,
  phone tinyint(4),
  processed tinyint(4) NOT NULL default '0',
  PRIMARY KEY  (id)
) TYPE=MyISAM;

#
# Dumping data for table 'inbox'
#


#
# Table structure for table 'outbox'
#

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
) TYPE=MyISAM;

#
# Dumping data for table 'outbox'
#



/*
SQLyog Ultimate v11.11 (64 bit)
MySQL - 5.6.39 : Database - auth
*********************************************************************
*/

/*!40101 SET NAMES utf8 */;

/*!40101 SET SQL_MODE=''*/;

/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;
CREATE DATABASE /*!32312 IF NOT EXISTS*/`auth` /*!40100 DEFAULT CHARACTER SET utf8 */;

USE `auth`;

/*Table structure for table `account` */

DROP TABLE IF EXISTS `account`;

CREATE TABLE `account` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT COMMENT 'Identifier',
  `username` varchar(32) NOT NULL DEFAULT '',
  `username_wod623` varchar(250) NOT NULL DEFAULT '',
  `token_key_wod623` varchar(100) NOT NULL DEFAULT '',
  `sha_pass_hash` varchar(40) NOT NULL DEFAULT '',
  `sha_pass_hash_wod623` varchar(40) NOT NULL DEFAULT '',
  `sessionkey` varchar(80) NOT NULL DEFAULT '',
  `v` varchar(64) NOT NULL DEFAULT '',
  `s` varchar(64) NOT NULL DEFAULT '',
  `token_key_548` varchar(100) NOT NULL DEFAULT '',
  `token_key_335a` varchar(100) NOT NULL DEFAULT '',
  `email` varchar(254) NOT NULL DEFAULT '',
  `reg_mail_335a` varchar(255) NOT NULL DEFAULT '',
  `joindate` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `last_ip` varchar(15) NOT NULL DEFAULT '127.0.0.1',
  `last_attempt_ip_548` varchar(15) NOT NULL DEFAULT '127.0.0.1',
  `last_attempt_ip_335a` varchar(15) NOT NULL DEFAULT '127.0.0.1',
  `failed_logins` int(10) unsigned NOT NULL DEFAULT '0',
  `locked` smallint(3) NOT NULL DEFAULT '1',
  `lock_country_335a` varchar(2) NOT NULL DEFAULT '00',
  `last_login` timestamp NOT NULL DEFAULT '0000-00-00 00:00:00',
  `online` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `expansion` tinyint(3) unsigned NOT NULL DEFAULT '5',
  `mutetime` bigint(20) NOT NULL DEFAULT '0',
  `mutereason` varchar(255) NOT NULL DEFAULT ' ',
  `muteby` varchar(50) DEFAULT NULL,
  `locale` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `os` varchar(4) NOT NULL DEFAULT '',
  `recruiter` int(10) unsigned NOT NULL DEFAULT '0',
  `hasBoost` tinyint(1) NOT NULL DEFAULT '0',
  `bnet2_pass_hash` varchar(256) NOT NULL DEFAULT '',
  `bnet2_salt` varchar(64) NOT NULL DEFAULT '',
  `google_auth` varchar(15) DEFAULT NULL,
  `rsa_clear` blob,
  `admin` tinyint(3) NOT NULL DEFAULT '0',
  `activate` tinyint(4) DEFAULT '0',
  `payment` tinyint(4) DEFAULT '0',
  `service_flags` int(10) unsigned NOT NULL DEFAULT '0',
  `custom_flags` int(10) unsigned NOT NULL DEFAULT '0',
  `forum_email` varchar(254) DEFAULT '',
  PRIMARY KEY (`id`),
  UNIQUE KEY `idx_username` (`username`),
  KEY `recruiterIndex` (`recruiter`) USING BTREE
) ENGINE=MyISAM AUTO_INCREMENT=3 DEFAULT CHARSET=utf8 COMMENT='Account System';

/*Data for the table `account` */

insert  into `account`(`id`,`username`,`username_wod623`,`token_key_wod623`,`sha_pass_hash`,`sha_pass_hash_wod623`,`sessionkey`,`v`,`s`,`token_key_548`,`token_key_335a`,`email`,`reg_mail_335a`,`joindate`,`last_ip`,`last_attempt_ip_548`,`last_attempt_ip_335a`,`failed_logins`,`locked`,`lock_country_335a`,`last_login`,`online`,`expansion`,`mutetime`,`mutereason`,`muteby`,`locale`,`os`,`recruiter`,`hasBoost`,`bnet2_pass_hash`,`bnet2_salt`,`google_auth`,`rsa_clear`,`admin`,`activate`,`payment`,`service_flags`,`custom_flags`,`forum_email`) values (1,'DAVID','','','57ECEC51A6FECBE2C121B6DA97ADDF250749085C','','3DFD49646D32A3704118100EC763C77E525FDCAF64E633F0543A0C5E7717F0D19BA768344BF660D7','5C513A48BC219525FC9C2DDDAF4E2E2BA8A04CFD7DA76FDD6116E889AC4FD26C','C8F32BC72BA701A53A0A84B6BD810E0C3F3F57704A761AA01CC72D11B831C7B5','','','','','2020-07-07 17:19:14','127.0.0.1','127.0.0.1','127.0.0.1',0,0,'00','2020-07-19 14:04:17',0,5,0,' ',NULL,0,'Win',0,0,'','',NULL,NULL,0,0,0,0,0,'DAVID@rog.snet'),(2,'DAVID1','','','6485AA1306F720CC60C0E04656AD78A9B0B26AA9','','C7154DF285A1F7CB43DF856314099D399E02B9C55DF96623606E78984848A16291B48416121B3348','69CF66897C3E52D8ED4B9F90EE6C2DE8CB00DDA532864E625317789C6139AEEA','D3B5B2DCDC6421344C331CF02F5FCDBD09D2AAA0965430A741D00A3A8E970CE3','','','','','2020-07-10 16:36:39','127.0.0.1','127.0.0.1','127.0.0.1',0,0,'00','2020-07-10 16:37:30',0,5,0,' ',NULL,6,'Win',0,0,'','',NULL,NULL,0,0,0,0,0,'DAVID1@rog.snet');

/*Table structure for table `account_access` */

DROP TABLE IF EXISTS `account_access`;

CREATE TABLE `account_access` (
  `id` int(10) unsigned NOT NULL,
  `gmlevel` tinyint(3) unsigned NOT NULL,
  `RealmID` int(11) NOT NULL DEFAULT '-1',
  `comment` varchar(35) NOT NULL DEFAULT 'name of the account',
  PRIMARY KEY (`id`,`RealmID`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

/*Data for the table `account_access` */

/*Table structure for table `account_banned` */

DROP TABLE IF EXISTS `account_banned`;

CREATE TABLE `account_banned` (
  `id` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Account id',
  `externalban` int(10) unsigned NOT NULL DEFAULT '0',
  `bandate` int(10) unsigned NOT NULL DEFAULT '0',
  `unbandate` int(10) unsigned NOT NULL DEFAULT '0',
  `bannedby` varchar(50) NOT NULL,
  `banreason` varchar(255) NOT NULL,
  `active` tinyint(3) unsigned NOT NULL DEFAULT '1',
  PRIMARY KEY (`id`,`bandate`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='Ban List';

/*Data for the table `account_banned` */

/*Table structure for table `account_premium` */

DROP TABLE IF EXISTS `account_premium`;

CREATE TABLE `account_premium` (
  `id` int(11) NOT NULL DEFAULT '0' COMMENT 'Account id',
  `setdate` bigint(40) NOT NULL DEFAULT '0',
  `unsetdate` bigint(40) NOT NULL DEFAULT '0',
  `premium_type` tinyint(4) unsigned NOT NULL DEFAULT '1',
  `active` tinyint(4) NOT NULL DEFAULT '1',
  PRIMARY KEY (`id`,`setdate`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='Premium Accounts';

/*Data for the table `account_premium` */

/*Table structure for table `account_punishment` */

DROP TABLE IF EXISTS `account_punishment`;

CREATE TABLE `account_punishment` (
  `id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `account` int(11) unsigned NOT NULL,
  `by` int(11) NOT NULL,
  `penalty_points` int(11) unsigned NOT NULL,
  `comment` varchar(255) NOT NULL,
  `time` bigint(20) unsigned NOT NULL,
  `active` tinyint(3) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=99 DEFAULT CHARSET=utf8;

/*Data for the table `account_punishment` */

/*Table structure for table `account_rates` */

DROP TABLE IF EXISTS `account_rates`;

CREATE TABLE `account_rates` (
  `account_id` int(10) NOT NULL DEFAULT '0',
  `player_guid` int(10) NOT NULL DEFAULT '0',
  `playerqueue` int(10) NOT NULL DEFAULT '0' COMMENT 'The player have to wait player queue to enter',
  `xp_rate` float NOT NULL DEFAULT '1',
  `reputation_rate` float NOT NULL DEFAULT '1',
  `money_rate` float NOT NULL DEFAULT '1',
  `skill_rate` float NOT NULL DEFAULT '1',
  `comment` text,
  PRIMARY KEY (`account_id`,`player_guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*Data for the table `account_rates` */

/*Table structure for table `account_rates_tournament` */

DROP TABLE IF EXISTS `account_rates_tournament`;

CREATE TABLE `account_rates_tournament` (
  `account_id` int(10) NOT NULL DEFAULT '0',
  `player_guid` int(10) NOT NULL DEFAULT '0',
  `playerqueue` int(10) NOT NULL DEFAULT '0' COMMENT 'The player have to wait player queue to enter',
  `xp_rate` float NOT NULL DEFAULT '1',
  `reputation_rate` float NOT NULL DEFAULT '1',
  `money_rate` float NOT NULL DEFAULT '1',
  `skill_rate` float NOT NULL DEFAULT '1',
  `comment` text,
  PRIMARY KEY (`account_id`,`player_guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*Data for the table `account_rates_tournament` */

/*Table structure for table `account_tempban` */

DROP TABLE IF EXISTS `account_tempban`;

CREATE TABLE `account_tempban` (
  `accountId` int(11) NOT NULL,
  `reason` varchar(255) NOT NULL,
  PRIMARY KEY (`accountId`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

/*Data for the table `account_tempban` */

/*Table structure for table `account_warning` */

DROP TABLE IF EXISTS `account_warning`;

CREATE TABLE `account_warning` (
  `id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `accountId` int(11) unsigned NOT NULL,
  `by_account` int(11) unsigned NOT NULL,
  `comment` varchar(255) NOT NULL,
  `time` bigint(20) unsigned NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=169 DEFAULT CHARSET=utf8;

/*Data for the table `account_warning` */

/*Table structure for table `accounts_to_delete` */

DROP TABLE IF EXISTS `accounts_to_delete`;

CREATE TABLE `accounts_to_delete` (
  `id` int(11) unsigned NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='Account System';

/*Data for the table `accounts_to_delete` */

/*Table structure for table `bug_tickets` */

DROP TABLE IF EXISTS `bug_tickets`;

CREATE TABLE `bug_tickets` (
  `ticketId` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `realm` int(10) unsigned NOT NULL DEFAULT '0',
  `guid` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Global Unique Identifier of ticket creator',
  `message` text NOT NULL,
  `createTime` int(10) unsigned NOT NULL DEFAULT '0',
  `pool` varchar(32) NOT NULL,
  `mapId` smallint(5) unsigned NOT NULL DEFAULT '0',
  `posX` float NOT NULL DEFAULT '0',
  `posY` float NOT NULL DEFAULT '0',
  `posZ` float NOT NULL DEFAULT '0',
  `closedBy` int(10) NOT NULL DEFAULT '0',
  PRIMARY KEY (`ticketId`)
) ENGINE=MyISAM AUTO_INCREMENT=990 DEFAULT CHARSET=utf8 COMMENT='Support System';

/*Data for the table `bug_tickets` */

/*Table structure for table `ip2nation` */

DROP TABLE IF EXISTS `ip2nation`;

CREATE TABLE `ip2nation` (
  `ip` int(11) unsigned NOT NULL DEFAULT '0',
  `country` varchar(2) NOT NULL DEFAULT '',
  KEY `ip` (`ip`) USING BTREE
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

/*Data for the table `ip2nation` */

/*Table structure for table `ip2nationcountries` */

DROP TABLE IF EXISTS `ip2nationcountries`;

CREATE TABLE `ip2nationcountries` (
  `code` varchar(2) NOT NULL DEFAULT '',
  `iso_code_2` varchar(2) NOT NULL DEFAULT '',
  `iso_code_3` varchar(3) DEFAULT '',
  `iso_country` varchar(255) NOT NULL DEFAULT '',
  `country` varchar(255) NOT NULL DEFAULT '',
  `lat` float NOT NULL DEFAULT '0',
  `lon` float NOT NULL DEFAULT '0',
  PRIMARY KEY (`code`),
  KEY `code` (`code`) USING BTREE
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

/*Data for the table `ip2nationcountries` */

/*Table structure for table `ip_banned` */

DROP TABLE IF EXISTS `ip_banned`;

CREATE TABLE `ip_banned` (
  `ip` varchar(15) NOT NULL DEFAULT '127.0.0.1',
  `bandate` int(10) unsigned NOT NULL,
  `unbandate` int(10) NOT NULL,
  `bannedby` varchar(50) NOT NULL DEFAULT '[Console]',
  `banreason` varchar(255) NOT NULL DEFAULT 'no reason',
  PRIMARY KEY (`ip`,`bandate`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='Banned IPs';

/*Data for the table `ip_banned` */

/*Table structure for table `ip_banned_copy` */

DROP TABLE IF EXISTS `ip_banned_copy`;

CREATE TABLE `ip_banned_copy` (
  `ip` varchar(15) NOT NULL DEFAULT '127.0.0.1',
  `bandate` int(10) unsigned NOT NULL,
  `unbandate` int(10) NOT NULL,
  `bannedby` varchar(50) NOT NULL DEFAULT '[Console]',
  `banreason` varchar(255) NOT NULL DEFAULT 'no reason',
  PRIMARY KEY (`ip`,`bandate`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='Banned IPs';

/*Data for the table `ip_banned_copy` */

/*Table structure for table `lk335a_account_access` */

DROP TABLE IF EXISTS `lk335a_account_access`;

CREATE TABLE `lk335a_account_access` (
  `id` int(10) unsigned NOT NULL,
  `gmlevel` tinyint(3) unsigned NOT NULL,
  `RealmID` int(11) NOT NULL DEFAULT '-1',
  PRIMARY KEY (`id`,`RealmID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*Data for the table `lk335a_account_access` */

/*Table structure for table `lk335a_account_muted` */

DROP TABLE IF EXISTS `lk335a_account_muted`;

CREATE TABLE `lk335a_account_muted` (
  `guid` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Global Unique Identifier',
  `mutedate` int(10) unsigned NOT NULL DEFAULT '0',
  `mutetime` int(10) unsigned NOT NULL DEFAULT '0',
  `mutedby` varchar(50) NOT NULL,
  `mutereason` varchar(255) NOT NULL,
  PRIMARY KEY (`guid`,`mutedate`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='mute List';

/*Data for the table `lk335a_account_muted` */

/*Table structure for table `lk335a_autobroadcast` */

DROP TABLE IF EXISTS `lk335a_autobroadcast`;

CREATE TABLE `lk335a_autobroadcast` (
  `realmid` int(11) NOT NULL DEFAULT '-1',
  `id` tinyint(3) unsigned NOT NULL AUTO_INCREMENT,
  `weight` tinyint(3) unsigned DEFAULT '1',
  `text` longtext NOT NULL,
  PRIMARY KEY (`id`,`realmid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*Data for the table `lk335a_autobroadcast` */

/*Table structure for table `lk335a_ip2nation` */

DROP TABLE IF EXISTS `lk335a_ip2nation`;

CREATE TABLE `lk335a_ip2nation` (
  `ip` int(11) unsigned NOT NULL DEFAULT '0',
  `country` char(2) NOT NULL DEFAULT '',
  KEY `ip` (`ip`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*Data for the table `lk335a_ip2nation` */

/*Table structure for table `lk335a_ip2nationcountries` */

DROP TABLE IF EXISTS `lk335a_ip2nationcountries`;

CREATE TABLE `lk335a_ip2nationcountries` (
  `code` varchar(4) NOT NULL DEFAULT '',
  `iso_code_2` varchar(2) NOT NULL DEFAULT '',
  `iso_code_3` varchar(3) DEFAULT '',
  `iso_country` varchar(255) NOT NULL DEFAULT '',
  `country` varchar(255) NOT NULL DEFAULT '',
  `lat` float NOT NULL DEFAULT '0',
  `lon` float NOT NULL DEFAULT '0',
  PRIMARY KEY (`code`),
  KEY `code` (`code`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*Data for the table `lk335a_ip2nationcountries` */

/*Table structure for table `lk335a_logs` */

DROP TABLE IF EXISTS `lk335a_logs`;

CREATE TABLE `lk335a_logs` (
  `time` int(10) unsigned NOT NULL,
  `realm` int(10) unsigned NOT NULL,
  `type` varchar(250) NOT NULL,
  `level` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `string` text CHARACTER SET latin1
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*Data for the table `lk335a_logs` */

/*Table structure for table `lk335a_logs_ip_actions` */

DROP TABLE IF EXISTS `lk335a_logs_ip_actions`;

CREATE TABLE `lk335a_logs_ip_actions` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT COMMENT 'Unique Identifier',
  `account_id` int(10) unsigned NOT NULL COMMENT 'Account ID',
  `character_guid` int(10) unsigned NOT NULL COMMENT 'Character Guid',
  `type` tinyint(3) unsigned NOT NULL,
  `ip` varchar(15) NOT NULL DEFAULT '127.0.0.1',
  `systemnote` text COMMENT 'Notes inserted by system',
  `unixtime` int(10) unsigned NOT NULL COMMENT 'Unixtime',
  `time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT 'Timestamp',
  `comment` text COMMENT 'Allows users to add a comment',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='Used to log ips of individual actions';

/*Data for the table `lk335a_logs_ip_actions` */

/*Table structure for table `lk335a_rbac_account_groups` */

DROP TABLE IF EXISTS `lk335a_rbac_account_groups`;

CREATE TABLE `lk335a_rbac_account_groups` (
  `accountId` int(10) unsigned NOT NULL COMMENT 'Account id',
  `groupId` int(10) unsigned NOT NULL COMMENT 'Group id',
  `realmId` int(11) NOT NULL DEFAULT '-1' COMMENT 'Realm Id, -1 means all',
  PRIMARY KEY (`accountId`,`groupId`,`realmId`),
  KEY `fk__rbac_account_groups__rbac_groups` (`groupId`) USING BTREE
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='Account-Group relation';

/*Data for the table `lk335a_rbac_account_groups` */

/*Table structure for table `lk335a_rbac_account_permissions` */

DROP TABLE IF EXISTS `lk335a_rbac_account_permissions`;

CREATE TABLE `lk335a_rbac_account_permissions` (
  `accountId` int(10) unsigned NOT NULL COMMENT 'Account id',
  `permissionId` int(10) unsigned NOT NULL COMMENT 'Permission id',
  `granted` tinyint(1) NOT NULL DEFAULT '1' COMMENT 'Granted = 1, Denied = 0',
  `realmId` int(11) NOT NULL DEFAULT '-1' COMMENT 'Realm Id, -1 means all',
  PRIMARY KEY (`accountId`,`permissionId`,`realmId`),
  KEY `fk__rbac_account_roles__rbac_permissions` (`permissionId`),
  CONSTRAINT `lk335a_rbac_account_permissions_ibfk_2` FOREIGN KEY (`permissionId`) REFERENCES `lk335a_rbac_permissions` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='Account-Permission relation';

/*Data for the table `lk335a_rbac_account_permissions` */

/*Table structure for table `lk335a_rbac_default_permissions` */

DROP TABLE IF EXISTS `lk335a_rbac_default_permissions`;

CREATE TABLE `lk335a_rbac_default_permissions` (
  `secId` int(10) unsigned NOT NULL COMMENT 'Security Level id',
  `permissionId` int(10) unsigned NOT NULL COMMENT 'permission id',
  `realmId` int(11) NOT NULL DEFAULT '-1' COMMENT 'Realm Id, -1 means all',
  PRIMARY KEY (`secId`,`permissionId`,`realmId`),
  KEY `fk__rbac_default_permissions__rbac_permissions` (`permissionId`),
  CONSTRAINT `fk__rbac_default_permissions__rbac_permissions` FOREIGN KEY (`permissionId`) REFERENCES `lk335a_rbac_permissions` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='Default permission to assign to different account security levels';

/*Data for the table `lk335a_rbac_default_permissions` */

/*Table structure for table `lk335a_rbac_group_roles` */

DROP TABLE IF EXISTS `lk335a_rbac_group_roles`;

CREATE TABLE `lk335a_rbac_group_roles` (
  `groupId` int(10) unsigned NOT NULL COMMENT 'group id',
  `roleId` int(10) unsigned NOT NULL COMMENT 'Role id',
  PRIMARY KEY (`groupId`,`roleId`),
  KEY `fk__rbac_group_roles__rbac_roles` (`roleId`) USING BTREE
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='Group Role relation';

/*Data for the table `lk335a_rbac_group_roles` */

/*Table structure for table `lk335a_rbac_linked_permissions` */

DROP TABLE IF EXISTS `lk335a_rbac_linked_permissions`;

CREATE TABLE `lk335a_rbac_linked_permissions` (
  `id` int(10) unsigned NOT NULL COMMENT 'Permission id',
  `linkedId` int(10) unsigned NOT NULL COMMENT 'Linked Permission id',
  PRIMARY KEY (`id`,`linkedId`),
  KEY `fk__rbac_linked_permissions__rbac_permissions1` (`id`),
  KEY `fk__rbac_linked_permissions__rbac_permissions2` (`linkedId`),
  CONSTRAINT `fk__rbac_linked_permissions__rbac_permissions1` FOREIGN KEY (`id`) REFERENCES `lk335a_rbac_permissions` (`id`) ON DELETE CASCADE,
  CONSTRAINT `fk__rbac_linked_permissions__rbac_permissions2` FOREIGN KEY (`linkedId`) REFERENCES `lk335a_rbac_permissions` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='Permission - Linked Permission relation';

/*Data for the table `lk335a_rbac_linked_permissions` */

/*Table structure for table `lk335a_rbac_permissions` */

DROP TABLE IF EXISTS `lk335a_rbac_permissions`;

CREATE TABLE `lk335a_rbac_permissions` (
  `id` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Permission id',
  `name` varchar(100) NOT NULL COMMENT 'Permission name',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='Permission List';

/*Data for the table `lk335a_rbac_permissions` */

/*Table structure for table `lk335a_updates` */

DROP TABLE IF EXISTS `lk335a_updates`;

CREATE TABLE `lk335a_updates` (
  `name` varchar(200) NOT NULL COMMENT 'filename with extension of the update.',
  `hash` char(40) DEFAULT '' COMMENT 'sha1 hash of the sql file.',
  `state` enum('RELEASED','ARCHIVED') NOT NULL DEFAULT 'RELEASED' COMMENT 'defines if an update is released or archived.',
  `timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT 'timestamp when the query was applied.',
  `speed` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'time the query takes to apply in ms.',
  PRIMARY KEY (`name`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='List of all applied updates in this database.';

/*Data for the table `lk335a_updates` */

/*Table structure for table `lk335a_updates_include` */

DROP TABLE IF EXISTS `lk335a_updates_include`;

CREATE TABLE `lk335a_updates_include` (
  `path` varchar(200) NOT NULL COMMENT 'directory to include. $ means relative to the source directory.',
  `state` enum('RELEASED','ARCHIVED') NOT NULL DEFAULT 'RELEASED' COMMENT 'defines if the directory contains released or archived updates.',
  PRIMARY KEY (`path`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='List of directories where we want to include sql updates.';

/*Data for the table `lk335a_updates_include` */

/*Table structure for table `logs` */

DROP TABLE IF EXISTS `logs`;

CREATE TABLE `logs` (
  `time` int(10) unsigned NOT NULL,
  `realm` int(10) unsigned NOT NULL,
  `type` tinyint(3) unsigned NOT NULL,
  `level` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `string` text CHARACTER SET latin1
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

/*Data for the table `logs` */

/*Table structure for table `mop548_account_access` */

DROP TABLE IF EXISTS `mop548_account_access`;

CREATE TABLE `mop548_account_access` (
  `id` int(10) unsigned NOT NULL,
  `gmlevel` tinyint(3) unsigned NOT NULL,
  `RealmID` int(11) NOT NULL DEFAULT '-1',
  `comment` text,
  PRIMARY KEY (`id`,`RealmID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*Data for the table `mop548_account_access` */

insert  into `mop548_account_access`(`id`,`gmlevel`,`RealmID`,`comment`) values (1,12,-1,NULL);

/*Table structure for table `mop548_account_log_ip` */

DROP TABLE IF EXISTS `mop548_account_log_ip`;

CREATE TABLE `mop548_account_log_ip` (
  `accountid` int(11) unsigned NOT NULL,
  `ip` varchar(30) NOT NULL DEFAULT '0.0.0.0',
  `date` datetime DEFAULT NULL,
  PRIMARY KEY (`accountid`,`ip`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

/*Data for the table `mop548_account_log_ip` */

insert  into `mop548_account_log_ip`(`accountid`,`ip`,`date`) values (1,'127.0.0.1','2020-07-07 17:24:34'),(2,'127.0.0.1','2020-07-10 16:37:30');

/*Table structure for table `mop548_account_premium` */

DROP TABLE IF EXISTS `mop548_account_premium`;

CREATE TABLE `mop548_account_premium` (
  `id` int(11) NOT NULL DEFAULT '0' COMMENT 'Account id',
  `setdate` bigint(40) NOT NULL DEFAULT '0',
  `unsetdate` bigint(40) NOT NULL DEFAULT '0',
  `premium_type` tinyint(4) unsigned NOT NULL DEFAULT '1',
  `gm` varchar(12) NOT NULL DEFAULT '0',
  `active` tinyint(4) NOT NULL DEFAULT '1',
  PRIMARY KEY (`id`,`setdate`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*Data for the table `mop548_account_premium` */

/*Table structure for table `mop548_account_spell` */

DROP TABLE IF EXISTS `mop548_account_spell`;

CREATE TABLE `mop548_account_spell` (
  `accountId` bigint(20) NOT NULL,
  `spell` int(10) NOT NULL,
  `active` tinyint(1) DEFAULT NULL,
  `disabled` tinyint(1) DEFAULT NULL,
  PRIMARY KEY (`accountId`,`spell`),
  KEY `account` (`accountId`) USING HASH,
  KEY `account_spell` (`accountId`,`spell`) USING HASH
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

/*Data for the table `mop548_account_spell` */

insert  into `mop548_account_spell`(`accountId`,`spell`,`active`,`disabled`) values (1,43688,1,0),(1,46199,1,0),(1,75614,1,0);

/*Table structure for table `mop548_account_spell_tournament` */

DROP TABLE IF EXISTS `mop548_account_spell_tournament`;

CREATE TABLE `mop548_account_spell_tournament` (
  `accountId` int(21) unsigned NOT NULL,
  `spell` int(10) NOT NULL,
  `active` tinyint(1) DEFAULT NULL,
  `disabled` tinyint(1) DEFAULT NULL,
  PRIMARY KEY (`accountId`,`spell`),
  KEY `account` (`accountId`) USING HASH,
  KEY `account_spell` (`accountId`,`spell`) USING HASH
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

/*Data for the table `mop548_account_spell_tournament` */

/*Table structure for table `mop548_account_vip` */

DROP TABLE IF EXISTS `mop548_account_vip`;

CREATE TABLE `mop548_account_vip` (
  `acc_id` int(10) NOT NULL,
  `viplevel` tinyint(4) NOT NULL DEFAULT '1',
  `active` tinyint(4) NOT NULL DEFAULT '1',
  PRIMARY KEY (`acc_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

/*Data for the table `mop548_account_vip` */

/*Table structure for table `mop548_chat_filter` */

DROP TABLE IF EXISTS `mop548_chat_filter`;

CREATE TABLE `mop548_chat_filter` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT COMMENT 'Identifier',
  `text` char(64) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*Data for the table `mop548_chat_filter` */

/*Table structure for table `mop548_firewall_farms` */

DROP TABLE IF EXISTS `mop548_firewall_farms`;

CREATE TABLE `mop548_firewall_farms` (
  `ip` tinytext NOT NULL,
  `type` tinyint(1) unsigned NOT NULL DEFAULT '0'
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

/*Data for the table `mop548_firewall_farms` */

/*Table structure for table `mop548_ip_to_country` */

DROP TABLE IF EXISTS `mop548_ip_to_country`;

CREATE TABLE `mop548_ip_to_country` (
  `IP_FROM` double NOT NULL,
  `IP_TO` double NOT NULL,
  `COUNTRY_CODE` char(2) DEFAULT NULL,
  `COUNTRY_CODE_3` char(3) NOT NULL,
  `COUNTRY_NAME` varchar(50) DEFAULT NULL,
  PRIMARY KEY (`IP_FROM`,`IP_TO`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

/*Data for the table `mop548_ip_to_country` */

/*Table structure for table `mop548_log_vote` */

DROP TABLE IF EXISTS `mop548_log_vote`;

CREATE TABLE `mop548_log_vote` (
  `top_name` varchar(15) NOT NULL DEFAULT 'top',
  `ip` varchar(15) NOT NULL DEFAULT '0.0.0.0',
  `date` int(11) NOT NULL,
  PRIMARY KEY (`top_name`,`ip`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

/*Data for the table `mop548_log_vote` */

/*Table structure for table `mop548_logs` */

DROP TABLE IF EXISTS `mop548_logs`;

CREATE TABLE `mop548_logs` (
  `time` int(10) unsigned NOT NULL,
  `realm` int(10) unsigned NOT NULL,
  `type` tinyint(3) unsigned NOT NULL,
  `level` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `string` text CHARACTER SET latin1
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*Data for the table `mop548_logs` */

/*Table structure for table `mop548_mails` */

DROP TABLE IF EXISTS `mop548_mails`;

CREATE TABLE `mop548_mails` (
  `email` varchar(254) NOT NULL DEFAULT ''
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*Data for the table `mop548_mails` */

/*Table structure for table `mop548_transferts` */

DROP TABLE IF EXISTS `mop548_transferts`;

CREATE TABLE `mop548_transferts` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `account` int(11) NOT NULL,
  `perso_guid` int(11) NOT NULL,
  `from` int(11) NOT NULL,
  `to` int(11) NOT NULL,
  `revision` blob NOT NULL,
  `dump` longtext NOT NULL,
  `last_error` blob NOT NULL,
  `nb_attempt` int(11) NOT NULL,
  `state` int(10) DEFAULT NULL,
  `error` int(10) DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

/*Data for the table `mop548_transferts` */

/*Table structure for table `mop548_transferts_logs` */

DROP TABLE IF EXISTS `mop548_transferts_logs`;

CREATE TABLE `mop548_transferts_logs` (
  `id` int(11) DEFAULT NULL,
  `account` int(11) DEFAULT NULL,
  `perso_guid` int(11) DEFAULT NULL,
  `from` int(2) DEFAULT NULL,
  `to` int(2) DEFAULT NULL,
  `dump` longtext
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

/*Data for the table `mop548_transferts_logs` */

/*Table structure for table `rbac_account_groups` */

DROP TABLE IF EXISTS `rbac_account_groups`;

CREATE TABLE `rbac_account_groups` (
  `accountId` int(10) unsigned NOT NULL COMMENT 'Account id',
  `groupId` int(10) unsigned NOT NULL COMMENT 'Group id',
  `realmId` int(11) NOT NULL DEFAULT '-1' COMMENT 'Realm Id, -1 means all',
  PRIMARY KEY (`accountId`,`groupId`,`realmId`),
  KEY `fk__rbac_account_groups__rbac_groups` (`groupId`) USING BTREE
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='Account-Group relation';

/*Data for the table `rbac_account_groups` */

/*Table structure for table `rbac_account_permissions` */

DROP TABLE IF EXISTS `rbac_account_permissions`;

CREATE TABLE `rbac_account_permissions` (
  `accountId` int(10) unsigned NOT NULL COMMENT 'Account id',
  `permissionId` int(10) unsigned NOT NULL COMMENT 'Permission id',
  `granted` tinyint(1) NOT NULL DEFAULT '1' COMMENT 'Granted = 1, Denied = 0',
  `realmId` int(11) NOT NULL DEFAULT '-1' COMMENT 'Realm Id, -1 means all',
  PRIMARY KEY (`accountId`,`permissionId`,`realmId`),
  KEY `fk__rbac_account_roles__rbac_permissions` (`permissionId`) USING BTREE
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='Account-Permission relation';

/*Data for the table `rbac_account_permissions` */

/*Table structure for table `rbac_account_roles` */

DROP TABLE IF EXISTS `rbac_account_roles`;

CREATE TABLE `rbac_account_roles` (
  `accountId` int(10) unsigned NOT NULL COMMENT 'Account id',
  `roleId` int(10) unsigned NOT NULL COMMENT 'Role id',
  `granted` tinyint(1) NOT NULL DEFAULT '1' COMMENT 'Granted = 1, Denied = 0',
  `realmId` int(11) NOT NULL DEFAULT '-1' COMMENT 'Realm Id, -1 means all',
  PRIMARY KEY (`accountId`,`roleId`,`realmId`),
  KEY `fk__rbac_account_roles__rbac_roles` (`roleId`) USING BTREE
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='Account-Role relation';

/*Data for the table `rbac_account_roles` */

/*Table structure for table `rbac_group_roles` */

DROP TABLE IF EXISTS `rbac_group_roles`;

CREATE TABLE `rbac_group_roles` (
  `groupId` int(10) unsigned NOT NULL COMMENT 'group id',
  `roleId` int(10) unsigned NOT NULL COMMENT 'Role id',
  PRIMARY KEY (`groupId`,`roleId`),
  KEY `fk__rbac_group_roles__rbac_roles` (`roleId`) USING BTREE
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='Group Role relation';

/*Data for the table `rbac_group_roles` */

/*Table structure for table `rbac_groups` */

DROP TABLE IF EXISTS `rbac_groups`;

CREATE TABLE `rbac_groups` (
  `id` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Group id',
  `name` varchar(100) NOT NULL COMMENT 'Group name',
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='Group List';

/*Data for the table `rbac_groups` */

/*Table structure for table `rbac_permissions___` */

DROP TABLE IF EXISTS `rbac_permissions___`;

CREATE TABLE `rbac_permissions___` (
  `id` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Permission id',
  `name` varchar(100) NOT NULL COMMENT 'Permission name',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='Permission List';

/*Data for the table `rbac_permissions___` */

/*Table structure for table `rbac_role_permissions` */

DROP TABLE IF EXISTS `rbac_role_permissions`;

CREATE TABLE `rbac_role_permissions` (
  `roleId` int(10) unsigned NOT NULL COMMENT 'Role id',
  `permissionId` int(10) unsigned NOT NULL COMMENT 'Permission id',
  PRIMARY KEY (`roleId`,`permissionId`),
  KEY `fk__role_permissions__rbac_permissions` (`permissionId`) USING BTREE
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='Role Permission relation';

/*Data for the table `rbac_role_permissions` */

/*Table structure for table `rbac_roles` */

DROP TABLE IF EXISTS `rbac_roles`;

CREATE TABLE `rbac_roles` (
  `id` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Role id',
  `name` varchar(100) NOT NULL COMMENT 'Role name',
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='Roles List';

/*Data for the table `rbac_roles` */

/*Table structure for table `rbac_security_level_groups` */

DROP TABLE IF EXISTS `rbac_security_level_groups`;

CREATE TABLE `rbac_security_level_groups` (
  `secId` tinyint(3) unsigned NOT NULL COMMENT 'Security Level id',
  `groupId` int(10) unsigned NOT NULL COMMENT 'group id',
  PRIMARY KEY (`secId`,`groupId`),
  KEY `fk__rbac_security_level_groups__rbac_groups` (`groupId`) USING BTREE
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='Default groups to assign when an account is set gm level';

/*Data for the table `rbac_security_level_groups` */

/*Table structure for table `realmcharacters` */

DROP TABLE IF EXISTS `realmcharacters`;

CREATE TABLE `realmcharacters` (
  `realmid` int(10) unsigned NOT NULL DEFAULT '0',
  `acctid` int(10) unsigned NOT NULL,
  `numchars` tinyint(3) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`realmid`,`acctid`),
  KEY `acctid` (`acctid`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Realm Character Tracker';

/*Data for the table `realmcharacters` */

insert  into `realmcharacters`(`realmid`,`acctid`,`numchars`) values (1,1,6),(1,2,2);

/*Table structure for table `realmlist` */

DROP TABLE IF EXISTS `realmlist`;

CREATE TABLE `realmlist` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `name` varchar(32) NOT NULL DEFAULT '',
  `address` varchar(255) NOT NULL DEFAULT '127.0.0.1',
  `localAddress` varchar(255) NOT NULL DEFAULT '127.0.0.1',
  `localSubnetMask` varchar(255) NOT NULL DEFAULT '255.255.255.0',
  `port` smallint(5) unsigned NOT NULL DEFAULT '8085',
  `icon` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `flag` tinyint(3) unsigned NOT NULL DEFAULT '2',
  `timezone` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `allowedSecurityLevel` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `motd` varchar(255) NOT NULL DEFAULT 'Welcome on our server.',
  `population` float unsigned NOT NULL DEFAULT '0',
  `gamebuild` int(10) unsigned NOT NULL DEFAULT '15595',
  `online` int(10) DEFAULT '0',
  `delay` int(10) unsigned NOT NULL DEFAULT '0',
  `queue` int(10) unsigned NOT NULL DEFAULT '0',
  `lastupdate` int(10) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`),
  UNIQUE KEY `idx_name` (`name`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=2 DEFAULT CHARSET=utf8 COMMENT='Realm System';

/*Data for the table `realmlist` */

insert  into `realmlist`(`id`,`name`,`address`,`localAddress`,`localSubnetMask`,`port`,`icon`,`flag`,`timezone`,`allowedSecurityLevel`,`motd`,`population`,`gamebuild`,`online`,`delay`,`queue`,`lastupdate`) values (1,'Ikebana','127.0.0.1','127.0.0.1','255.255.255.0',8085,0,1,1,0,'Welcome to Our Server.',0,18019,1,0,0,0);

/*Table structure for table `uptime` */

DROP TABLE IF EXISTS `uptime`;

CREATE TABLE `uptime` (
  `realmid` int(10) unsigned NOT NULL,
  `starttime` int(10) unsigned NOT NULL DEFAULT '0',
  `uptime` int(10) unsigned NOT NULL DEFAULT '0',
  `maxplayers` smallint(5) unsigned NOT NULL DEFAULT '0',
  `revision` varchar(255) NOT NULL DEFAULT 'Trinitycore',
  PRIMARY KEY (`realmid`,`starttime`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='Uptime system';

/*Data for the table `uptime` */

insert  into `uptime`(`realmid`,`starttime`,`uptime`,`maxplayers`,`revision`) values (1,1561774444,0,0,'WoWSource rev. 2016-04-01 00:00:00 +0000 (rev.1) (Win64, Release)'),(1,1594176056,624,1,'WoWSource rev. 2016-04-01 00:00:00 +0000 (rev.1) (Win64, Release)'),(1,1594420369,631,2,'WoWSource rev. 2016-04-01 00:00:00 +0000 (rev.1) (Win64, Release)'),(1,1594503683,0,0,'WoWSource rev. 2016-04-01 00:00:00 +0000 (rev.1) (Win64, Release)'),(1,1595145004,7235,1,'WoWSource rev. 2016-04-01 00:00:00 +0000 (rev.1) (Win64, Release)'),(1,1595188981,1827,1,'WoWSource rev. 2016-04-01 00:00:00 +0000 (rev.1) (Win64, Release)');

/*Table structure for table `user_reporting` */

DROP TABLE IF EXISTS `user_reporting`;

CREATE TABLE `user_reporting` (
  `ga` int(10) unsigned NOT NULL,
  `account_id` int(10) unsigned NOT NULL,
  `creation_date` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `step` int(10) NOT NULL DEFAULT '0',
  `referer` varchar(255) NOT NULL,
  `last_ip` varchar(255) NOT NULL,
  `has_view_creation` tinyint(3) unsigned DEFAULT '0',
  PRIMARY KEY (`ga`,`account_id`),
  KEY `ga_idx` (`ga`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 ROW_FORMAT=DYNAMIC;

/*Data for the table `user_reporting` */

/*Table structure for table `vip_accounts` */

DROP TABLE IF EXISTS `vip_accounts`;

CREATE TABLE `vip_accounts` (
  `id` int(11) NOT NULL DEFAULT '0' COMMENT 'Account id',
  `type` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `setdate` bigint(40) NOT NULL DEFAULT '0',
  `unsetdate` bigint(40) NOT NULL DEFAULT '0',
  `active` tinyint(4) NOT NULL DEFAULT '1',
  PRIMARY KEY (`id`,`setdate`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='VIP Accounts';

/*Data for the table `vip_accounts` */

/*Table structure for table `wod623_account_access` */

DROP TABLE IF EXISTS `wod623_account_access`;

CREATE TABLE `wod623_account_access` (
  `id` int(10) unsigned NOT NULL,
  `gmlevel` tinyint(3) unsigned NOT NULL,
  `RealmID` int(11) NOT NULL DEFAULT '-1',
  `comment` text,
  PRIMARY KEY (`id`,`RealmID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*Data for the table `wod623_account_access` */

/*Table structure for table `wod623_account_battlepay` */

DROP TABLE IF EXISTS `wod623_account_battlepay`;

CREATE TABLE `wod623_account_battlepay` (
  `accountId` int(11) unsigned NOT NULL,
  `points` int(11) NOT NULL,
  PRIMARY KEY (`accountId`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1 ROW_FORMAT=DYNAMIC;

/*Data for the table `wod623_account_battlepay` */

/*Table structure for table `wod623_account_battlepet` */

DROP TABLE IF EXISTS `wod623_account_battlepet`;

CREATE TABLE `wod623_account_battlepet` (
  `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  `account` int(10) unsigned NOT NULL DEFAULT '0',
  `slot` int(11) NOT NULL DEFAULT '-1',
  `name` varchar(50) NOT NULL,
  `nameTimeStamp` int(10) unsigned NOT NULL DEFAULT '0',
  `species` int(10) unsigned NOT NULL DEFAULT '0',
  `quality` int(10) unsigned NOT NULL DEFAULT '0',
  `breed` int(10) unsigned NOT NULL DEFAULT '0',
  `level` int(10) unsigned NOT NULL DEFAULT '0',
  `xp` int(10) unsigned NOT NULL DEFAULT '0',
  `display` int(10) unsigned NOT NULL DEFAULT '0',
  `health` int(11) NOT NULL DEFAULT '0',
  `flags` int(10) unsigned NOT NULL DEFAULT '0',
  `infoPower` int(11) NOT NULL DEFAULT '0',
  `infoMaxHealth` int(11) NOT NULL DEFAULT '0',
  `infoSpeed` int(11) NOT NULL DEFAULT '0',
  `infoGender` int(11) NOT NULL DEFAULT '0',
  `declinedGenitive` varchar(50) NOT NULL,
  `declinedNative` varchar(50) NOT NULL,
  `declinedAccusative` varchar(50) NOT NULL,
  `declinedInstrumental` varchar(50) NOT NULL,
  `declinedPrepositional` varchar(50) NOT NULL,
  PRIMARY KEY (`id`),
  KEY `account` (`account`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 ROW_FORMAT=DYNAMIC;

/*Data for the table `wod623_account_battlepet` */

/*Table structure for table `wod623_account_heirlooms` */

DROP TABLE IF EXISTS `wod623_account_heirlooms`;

CREATE TABLE `wod623_account_heirlooms` (
  `account_id` int(10) unsigned NOT NULL DEFAULT '0',
  `heirloom_id` int(10) unsigned NOT NULL DEFAULT '0',
  `upgrade_flags` int(10) unsigned NOT NULL DEFAULT '0',
  `groupRealmMask` int(10) unsigned NOT NULL,
  PRIMARY KEY (`account_id`,`heirloom_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1 ROW_FORMAT=DYNAMIC;

/*Data for the table `wod623_account_heirlooms` */

/*Table structure for table `wod623_account_log_ip` */

DROP TABLE IF EXISTS `wod623_account_log_ip`;

CREATE TABLE `wod623_account_log_ip` (
  `accountid` int(11) unsigned NOT NULL,
  `id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `ip` varchar(30) NOT NULL DEFAULT '0.0.0.0',
  `date` datetime DEFAULT NULL,
  `error` int(10) unsigned NOT NULL DEFAULT '0',
  `source` int(10) unsigned NOT NULL DEFAULT '2',
  PRIMARY KEY (`accountid`,`ip`,`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1 ROW_FORMAT=DYNAMIC;

/*Data for the table `wod623_account_log_ip` */

/*Table structure for table `wod623_account_loyalty` */

DROP TABLE IF EXISTS `wod623_account_loyalty`;

CREATE TABLE `wod623_account_loyalty` (
  `AccountID` int(11) unsigned NOT NULL,
  `LastClaim` int(11) NOT NULL DEFAULT '0',
  `LastEventReset` int(11) NOT NULL DEFAULT '0',
  PRIMARY KEY (`AccountID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC;

/*Data for the table `wod623_account_loyalty` */

/*Table structure for table `wod623_account_loyalty_event` */

DROP TABLE IF EXISTS `wod623_account_loyalty_event`;

CREATE TABLE `wod623_account_loyalty_event` (
  `AccountID` int(11) unsigned NOT NULL,
  `Event` int(11) unsigned NOT NULL DEFAULT '0',
  `Count` int(11) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`AccountID`,`Event`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC;

/*Data for the table `wod623_account_loyalty_event` */

/*Table structure for table `wod623_account_note` */

DROP TABLE IF EXISTS `wod623_account_note`;

CREATE TABLE `wod623_account_note` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `account_id` int(10) unsigned DEFAULT NULL,
  `note` text,
  `date` int(11) DEFAULT NULL,
  `notedby` varchar(255) DEFAULT NULL,
  KEY `id` (`id`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC;

/*Data for the table `wod623_account_note` */

/*Table structure for table `wod623_account_premium` */

DROP TABLE IF EXISTS `wod623_account_premium`;

CREATE TABLE `wod623_account_premium` (
  `id` int(11) NOT NULL DEFAULT '0' COMMENT 'Account id',
  `setdate` bigint(40) NOT NULL DEFAULT '0',
  `unsetdate` bigint(40) NOT NULL DEFAULT '0',
  `premium_type` tinyint(4) unsigned NOT NULL DEFAULT '1',
  `gm` varchar(12) NOT NULL DEFAULT '0',
  `active` tinyint(4) NOT NULL DEFAULT '1',
  PRIMARY KEY (`id`,`setdate`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*Data for the table `wod623_account_premium` */

/*Table structure for table `wod623_account_spell` */

DROP TABLE IF EXISTS `wod623_account_spell`;

CREATE TABLE `wod623_account_spell` (
  `accountId` bigint(20) NOT NULL,
  `spell` int(10) NOT NULL,
  `active` tinyint(1) DEFAULT NULL,
  `disabled` tinyint(1) DEFAULT NULL,
  `IsMountFavorite` tinyint(1) NOT NULL DEFAULT '0',
  `groupRealmMask` int(10) unsigned NOT NULL DEFAULT '1',
  PRIMARY KEY (`accountId`,`spell`),
  KEY `account` (`accountId`) USING HASH,
  KEY `account_spell` (`accountId`,`spell`) USING HASH
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

/*Data for the table `wod623_account_spell` */

/*Table structure for table `wod623_account_spell_old` */

DROP TABLE IF EXISTS `wod623_account_spell_old`;

CREATE TABLE `wod623_account_spell_old` (
  `accountId` int(20) NOT NULL,
  `spell` int(10) NOT NULL,
  `active` tinyint(1) DEFAULT NULL,
  `disabled` tinyint(1) DEFAULT NULL,
  `IsMountFavorite` tinyint(1) NOT NULL DEFAULT '0',
  `groupRealmMask` int(10) unsigned NOT NULL,
  PRIMARY KEY (`accountId`,`spell`),
  KEY `account` (`accountId`) USING HASH,
  KEY `account_spell` (`accountId`,`spell`) USING HASH
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

/*Data for the table `wod623_account_spell_old` */

/*Table structure for table `wod623_account_toys` */

DROP TABLE IF EXISTS `wod623_account_toys`;

CREATE TABLE `wod623_account_toys` (
  `account_id` int(10) unsigned NOT NULL,
  `item_id` int(10) unsigned NOT NULL,
  `is_favorite` tinyint(1) NOT NULL DEFAULT '0',
  PRIMARY KEY (`account_id`,`item_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1 ROW_FORMAT=DYNAMIC;

/*Data for the table `wod623_account_toys` */

/*Table structure for table `wod623_account_vote` */

DROP TABLE IF EXISTS `wod623_account_vote`;

CREATE TABLE `wod623_account_vote` (
  `account` int(11) unsigned NOT NULL,
  `remainingTime` int(11) unsigned NOT NULL,
  PRIMARY KEY (`account`),
  UNIQUE KEY `account` (`account`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 ROW_FORMAT=DYNAMIC;

/*Data for the table `wod623_account_vote` */

/*Table structure for table `wod623_autobroadcast` */

DROP TABLE IF EXISTS `wod623_autobroadcast`;

CREATE TABLE `wod623_autobroadcast` (
  `Expension` int(11) DEFAULT NULL,
  `RealmID` int(11) DEFAULT NULL,
  `Text` blob,
  `TextFR` blob,
  `TextES` blob,
  `textRU` blob
) ENGINE=InnoDB DEFAULT CHARSET=latin1 ROW_FORMAT=DYNAMIC;

/*Data for the table `wod623_autobroadcast` */

/*Table structure for table `wod623_battlenet_modules` */

DROP TABLE IF EXISTS `wod623_battlenet_modules`;

CREATE TABLE `wod623_battlenet_modules` (
  `Hash` varchar(64) NOT NULL,
  `Name` varchar(64) NOT NULL DEFAULT '',
  `Type` varchar(8) NOT NULL,
  `System` varchar(8) NOT NULL,
  `Data` text,
  PRIMARY KEY (`Name`,`System`),
  UNIQUE KEY `uk_name_type_system` (`Name`,`Type`,`System`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC;

/*Data for the table `wod623_battlenet_modules` */

/*Table structure for table `wod623_bnet_allowed_build` */

DROP TABLE IF EXISTS `wod623_bnet_allowed_build`;

CREATE TABLE `wod623_bnet_allowed_build` (
  `build` int(11) DEFAULT NULL,
  `version` varchar(40) DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1 ROW_FORMAT=DYNAMIC;

/*Data for the table `wod623_bnet_allowed_build` */

/*Table structure for table `wod623_character_renderer_queue` */

DROP TABLE IF EXISTS `wod623_character_renderer_queue`;

CREATE TABLE `wod623_character_renderer_queue` (
  `guid` int(11) DEFAULT NULL,
  `race` tinyint(3) DEFAULT NULL,
  `gender` tinyint(3) DEFAULT NULL,
  `class` tinyint(3) DEFAULT NULL,
  `skinColor` tinyint(3) DEFAULT NULL,
  `face` tinyint(3) DEFAULT NULL,
  `hairStyle` tinyint(3) DEFAULT NULL,
  `hairColor` tinyint(3) DEFAULT NULL,
  `facialHair` tinyint(3) DEFAULT NULL,
  `equipment` blob
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin ROW_FORMAT=DYNAMIC;

/*Data for the table `wod623_character_renderer_queue` */

/*Table structure for table `wod623_firewall_farms` */

DROP TABLE IF EXISTS `wod623_firewall_farms`;

CREATE TABLE `wod623_firewall_farms` (
  `ip` tinytext NOT NULL,
  `type` tinyint(1) unsigned NOT NULL DEFAULT '0'
) ENGINE=InnoDB DEFAULT CHARSET=latin1 ROW_FORMAT=DYNAMIC;

/*Data for the table `wod623_firewall_farms` */

/*Table structure for table `wod623_ip_to_country` */

DROP TABLE IF EXISTS `wod623_ip_to_country`;

CREATE TABLE `wod623_ip_to_country` (
  `IP_FROM` bigint(20) unsigned NOT NULL,
  `IP_TO` bigint(20) unsigned NOT NULL,
  `REGISTRY` char(7) NOT NULL,
  `ASSIGNED` bigint(20) NOT NULL,
  `CTRY` char(2) NOT NULL,
  `CNTRY` char(3) NOT NULL,
  `COUNTRY` varchar(100) NOT NULL,
  PRIMARY KEY (`IP_FROM`,`IP_TO`),
  KEY `IP_FROM` (`IP_FROM`) USING BTREE,
  KEY `IP_TO` (`IP_TO`) USING BTREE
) ENGINE=MyISAM DEFAULT CHARSET=latin1 ROW_FORMAT=DYNAMIC;

/*Data for the table `wod623_ip_to_country` */

/*Table structure for table `wod623_log_vote` */

DROP TABLE IF EXISTS `wod623_log_vote`;

CREATE TABLE `wod623_log_vote` (
  `top_name` varchar(15) NOT NULL DEFAULT 'top',
  `ip` varchar(15) NOT NULL DEFAULT '0.0.0.0',
  `date` int(11) NOT NULL,
  PRIMARY KEY (`top_name`,`ip`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1 ROW_FORMAT=DYNAMIC;

/*Data for the table `wod623_log_vote` */

/*Table structure for table `wod623_logs` */

DROP TABLE IF EXISTS `wod623_logs`;

CREATE TABLE `wod623_logs` (
  `time` int(10) unsigned NOT NULL,
  `realm` int(10) unsigned NOT NULL,
  `type` tinyint(3) unsigned NOT NULL,
  `level` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `string` text CHARACTER SET latin1
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*Data for the table `wod623_logs` */

/*Table structure for table `wod623_motd` */

DROP TABLE IF EXISTS `wod623_motd`;

CREATE TABLE `wod623_motd` (
  `RealmID` int(10) unsigned NOT NULL,
  `Text` text NOT NULL,
  `TextFR` text NOT NULL,
  `TextES` text NOT NULL,
  `TextRU` text NOT NULL,
  PRIMARY KEY (`RealmID`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1 ROW_FORMAT=DYNAMIC;

/*Data for the table `wod623_motd` */

/*Table structure for table `wod623_stat_lifetime_retention` */

DROP TABLE IF EXISTS `wod623_stat_lifetime_retention`;

CREATE TABLE `wod623_stat_lifetime_retention` (
  `minutes` int(11) NOT NULL,
  `usersPercentage` float(11,8) DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1 ROW_FORMAT=DYNAMIC;

/*Data for the table `wod623_stat_lifetime_retention` */

/*Table structure for table `wod623_transferts` */

DROP TABLE IF EXISTS `wod623_transferts`;

CREATE TABLE `wod623_transferts` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `account` int(11) NOT NULL,
  `perso_guid` int(11) NOT NULL,
  `from` int(11) NOT NULL,
  `to` int(11) NOT NULL,
  `revision` blob NOT NULL,
  `dump` longtext NOT NULL,
  `last_error` blob NOT NULL,
  `nb_attempt` int(11) NOT NULL,
  `state` int(10) NOT NULL DEFAULT '0',
  `error` int(10) NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1 ROW_FORMAT=DYNAMIC;

/*Data for the table `wod623_transferts` */

/*Table structure for table `wod623_transferts_logs` */

DROP TABLE IF EXISTS `wod623_transferts_logs`;

CREATE TABLE `wod623_transferts_logs` (
  `id` int(11) DEFAULT NULL,
  `account` int(11) DEFAULT NULL,
  `perso_guid` int(11) DEFAULT NULL,
  `from` int(2) DEFAULT NULL,
  `to` int(2) DEFAULT NULL,
  `dump` longtext
) ENGINE=InnoDB DEFAULT CHARSET=latin1 ROW_FORMAT=DYNAMIC;

/*Data for the table `wod623_transferts_logs` */

/*Table structure for table `wod623_webshop_delivery_interexp_transfer` */

DROP TABLE IF EXISTS `wod623_webshop_delivery_interexp_transfer`;

CREATE TABLE `wod623_webshop_delivery_interexp_transfer` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `account` int(11) NOT NULL,
  `guid` int(11) NOT NULL,
  `dump` longtext NOT NULL,
  `destrealm` int(11) NOT NULL,
  `state` int(11) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1 ROW_FORMAT=DYNAMIC;

/*Data for the table `wod623_webshop_delivery_interexp_transfer` */

/*Table structure for table `wod623_webshop_delivery_interrealm_transfer` */

DROP TABLE IF EXISTS `wod623_webshop_delivery_interrealm_transfer`;

CREATE TABLE `wod623_webshop_delivery_interrealm_transfer` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `account` int(11) unsigned NOT NULL DEFAULT '0',
  `guid` int(11) unsigned NOT NULL DEFAULT '0',
  `startrealm` int(11) unsigned NOT NULL DEFAULT '0',
  `destrealm` int(11) unsigned NOT NULL DEFAULT '0',
  `revision` blob,
  `dump` longtext,
  `last_error` blob,
  `nb_attempt` int(11) unsigned NOT NULL DEFAULT '0',
  `state` int(10) unsigned NOT NULL DEFAULT '1',
  `error` int(10) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`),
  KEY `account` (`account`) USING BTREE,
  KEY `guid` (`guid`) USING BTREE,
  KEY `startrealm` (`startrealm`) USING BTREE,
  KEY `destrealm` (`destrealm`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 ROW_FORMAT=DYNAMIC;

/*Data for the table `wod623_webshop_delivery_interrealm_transfer` */

/* Trigger structure for table `account` */

DELIMITER $$

/*!50003 DROP TRIGGER*//*!50032 IF EXISTS */ /*!50003 `mail_update` */$$

/*!50003 CREATE */ /*!50017 DEFINER = 'root'@'localhost' */ /*!50003 TRIGGER `mail_update` BEFORE INSERT ON `account` FOR EACH ROW SET NEW.forum_email = CONCAT(NEW.username, "@rog.snet") */$$


DELIMITER ;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

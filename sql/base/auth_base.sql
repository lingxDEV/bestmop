-- --------------------------------------------------------
-- Host:                         127.0.0.1
-- Versión del servidor:         8.0.22 - MySQL Community Server - GPL
-- SO del servidor:              Win64
-- HeidiSQL Versión:             11.1.0.6116
-- --------------------------------------------------------

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET NAMES utf8 */;
/*!50503 SET NAMES utf8mb4 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

-- Volcando estructura para tabla auth.account
DROP TABLE IF EXISTS `account`;
CREATE TABLE IF NOT EXISTS `account` (
  `id` int unsigned NOT NULL AUTO_INCREMENT COMMENT 'Identifier',
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
  `failed_logins` int unsigned NOT NULL DEFAULT '0',
  `locked` smallint NOT NULL DEFAULT '1',
  `lock_country_335a` varchar(2) NOT NULL DEFAULT '00',
  `last_login` timestamp NOT NULL DEFAULT '0000-00-00 00:00:00',
  `online` tinyint unsigned NOT NULL DEFAULT '0',
  `expansion` tinyint unsigned NOT NULL DEFAULT '5',
  `mutetime` bigint NOT NULL DEFAULT '0',
  `mutereason` varchar(255) NOT NULL DEFAULT ' ',
  `muteby` varchar(50) DEFAULT NULL,
  `locale` tinyint unsigned NOT NULL DEFAULT '0',
  `os` varchar(4) NOT NULL DEFAULT '',
  `recruiter` int unsigned NOT NULL DEFAULT '0',
  `hasBoost` tinyint(1) NOT NULL DEFAULT '0',
  `bnet2_pass_hash` varchar(256) NOT NULL DEFAULT '',
  `bnet2_salt` varchar(64) NOT NULL DEFAULT '',
  `google_auth` varchar(15) DEFAULT NULL,
  `rsa_clear` blob,
  `admin` tinyint NOT NULL DEFAULT '0',
  `activate` tinyint DEFAULT '0',
  `payment` tinyint DEFAULT '0',
  `service_flags` int unsigned NOT NULL DEFAULT '0',
  `custom_flags` int unsigned NOT NULL DEFAULT '0',
  `forum_email` varchar(254) DEFAULT '',
  PRIMARY KEY (`id`),
  UNIQUE KEY `idx_username` (`username`),
  KEY `recruiterIndex` (`recruiter`) USING BTREE
) ENGINE=MyISAM AUTO_INCREMENT=4 DEFAULT CHARSET=utf8 COMMENT='Account System';

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.accounts_to_delete
DROP TABLE IF EXISTS `accounts_to_delete`;
CREATE TABLE IF NOT EXISTS `accounts_to_delete` (
  `id` int unsigned NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='Account System';

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.account_access
DROP TABLE IF EXISTS `account_access`;
CREATE TABLE IF NOT EXISTS `account_access` (
  `id` int unsigned NOT NULL,
  `gmlevel` tinyint unsigned NOT NULL,
  `RealmID` int NOT NULL DEFAULT '-1',
  `comment` varchar(35) NOT NULL DEFAULT 'name of the account',
  PRIMARY KEY (`id`,`RealmID`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.account_banned
DROP TABLE IF EXISTS `account_banned`;
CREATE TABLE IF NOT EXISTS `account_banned` (
  `id` int unsigned NOT NULL DEFAULT '0' COMMENT 'Account id',
  `externalban` int unsigned NOT NULL DEFAULT '0',
  `bandate` int unsigned NOT NULL DEFAULT '0',
  `unbandate` int unsigned NOT NULL DEFAULT '0',
  `bannedby` varchar(50) NOT NULL,
  `banreason` varchar(255) NOT NULL,
  `active` tinyint unsigned NOT NULL DEFAULT '1',
  PRIMARY KEY (`id`,`bandate`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='Ban List';

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.account_premium
DROP TABLE IF EXISTS `account_premium`;
CREATE TABLE IF NOT EXISTS `account_premium` (
  `id` int NOT NULL DEFAULT '0' COMMENT 'Account id',
  `setdate` bigint NOT NULL DEFAULT '0',
  `unsetdate` bigint NOT NULL DEFAULT '0',
  `premium_type` tinyint unsigned NOT NULL DEFAULT '1',
  `active` tinyint NOT NULL DEFAULT '1',
  PRIMARY KEY (`id`,`setdate`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='Premium Accounts';

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.account_punishment
DROP TABLE IF EXISTS `account_punishment`;
CREATE TABLE IF NOT EXISTS `account_punishment` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `account` int unsigned NOT NULL,
  `by` int NOT NULL,
  `penalty_points` int unsigned NOT NULL,
  `comment` varchar(255) NOT NULL,
  `time` bigint unsigned NOT NULL,
  `active` tinyint NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=99 DEFAULT CHARSET=utf8;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.account_rates
DROP TABLE IF EXISTS `account_rates`;
CREATE TABLE IF NOT EXISTS `account_rates` (
  `account_id` int NOT NULL DEFAULT '0',
  `player_guid` int NOT NULL DEFAULT '0',
  `playerqueue` int NOT NULL DEFAULT '0' COMMENT 'The player have to wait player queue to enter',
  `xp_rate` float NOT NULL DEFAULT '1',
  `reputation_rate` float NOT NULL DEFAULT '1',
  `money_rate` float NOT NULL DEFAULT '1',
  `skill_rate` float NOT NULL DEFAULT '1',
  `comment` text,
  PRIMARY KEY (`account_id`,`player_guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.account_rates_tournament
DROP TABLE IF EXISTS `account_rates_tournament`;
CREATE TABLE IF NOT EXISTS `account_rates_tournament` (
  `account_id` int NOT NULL DEFAULT '0',
  `player_guid` int NOT NULL DEFAULT '0',
  `playerqueue` int NOT NULL DEFAULT '0' COMMENT 'The player have to wait player queue to enter',
  `xp_rate` float NOT NULL DEFAULT '1',
  `reputation_rate` float NOT NULL DEFAULT '1',
  `money_rate` float NOT NULL DEFAULT '1',
  `skill_rate` float NOT NULL DEFAULT '1',
  `comment` text,
  PRIMARY KEY (`account_id`,`player_guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.account_tempban
DROP TABLE IF EXISTS `account_tempban`;
CREATE TABLE IF NOT EXISTS `account_tempban` (
  `accountId` int NOT NULL,
  `reason` varchar(255) NOT NULL,
  PRIMARY KEY (`accountId`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.account_warning
DROP TABLE IF EXISTS `account_warning`;
CREATE TABLE IF NOT EXISTS `account_warning` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `accountId` int unsigned NOT NULL,
  `by_account` int unsigned NOT NULL,
  `comment` varchar(255) NOT NULL,
  `time` bigint unsigned NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=169 DEFAULT CHARSET=utf8;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.bug_tickets
DROP TABLE IF EXISTS `bug_tickets`;
CREATE TABLE IF NOT EXISTS `bug_tickets` (
  `ticketId` int unsigned NOT NULL AUTO_INCREMENT,
  `realm` int unsigned NOT NULL DEFAULT '0',
  `guid` int unsigned NOT NULL DEFAULT '0' COMMENT 'Global Unique Identifier of ticket creator',
  `message` text NOT NULL,
  `createTime` int unsigned NOT NULL DEFAULT '0',
  `pool` varchar(32) NOT NULL,
  `mapId` smallint unsigned NOT NULL DEFAULT '0',
  `posX` float NOT NULL DEFAULT '0',
  `posY` float NOT NULL DEFAULT '0',
  `posZ` float NOT NULL DEFAULT '0',
  `closedBy` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ticketId`)
) ENGINE=MyISAM AUTO_INCREMENT=990 DEFAULT CHARSET=utf8 COMMENT='Support System';

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.ip2nation
DROP TABLE IF EXISTS `ip2nation`;
CREATE TABLE IF NOT EXISTS `ip2nation` (
  `ip` int unsigned NOT NULL DEFAULT '0',
  `country` varchar(2) NOT NULL DEFAULT '',
  KEY `ip` (`ip`) USING BTREE
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.ip2nationcountries
DROP TABLE IF EXISTS `ip2nationcountries`;
CREATE TABLE IF NOT EXISTS `ip2nationcountries` (
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

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.ip_banned
DROP TABLE IF EXISTS `ip_banned`;
CREATE TABLE IF NOT EXISTS `ip_banned` (
  `ip` varchar(15) NOT NULL DEFAULT '127.0.0.1',
  `bandate` int unsigned NOT NULL,
  `unbandate` int NOT NULL,
  `bannedby` varchar(50) NOT NULL DEFAULT '[Console]',
  `banreason` varchar(255) NOT NULL DEFAULT 'no reason',
  PRIMARY KEY (`ip`,`bandate`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='Banned IPs';

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.ip_banned_copy
DROP TABLE IF EXISTS `ip_banned_copy`;
CREATE TABLE IF NOT EXISTS `ip_banned_copy` (
  `ip` varchar(15) NOT NULL DEFAULT '127.0.0.1',
  `bandate` int unsigned NOT NULL,
  `unbandate` int NOT NULL,
  `bannedby` varchar(50) NOT NULL DEFAULT '[Console]',
  `banreason` varchar(255) NOT NULL DEFAULT 'no reason',
  PRIMARY KEY (`ip`,`bandate`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='Banned IPs';

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.lk335a_account_access
DROP TABLE IF EXISTS `lk335a_account_access`;
CREATE TABLE IF NOT EXISTS `lk335a_account_access` (
  `id` int unsigned NOT NULL,
  `gmlevel` tinyint unsigned NOT NULL,
  `RealmID` int NOT NULL DEFAULT '-1',
  PRIMARY KEY (`id`,`RealmID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.lk335a_account_muted
DROP TABLE IF EXISTS `lk335a_account_muted`;
CREATE TABLE IF NOT EXISTS `lk335a_account_muted` (
  `guid` int unsigned NOT NULL DEFAULT '0' COMMENT 'Global Unique Identifier',
  `mutedate` int unsigned NOT NULL DEFAULT '0',
  `mutetime` int unsigned NOT NULL DEFAULT '0',
  `mutedby` varchar(50) NOT NULL,
  `mutereason` varchar(255) NOT NULL,
  PRIMARY KEY (`guid`,`mutedate`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='mute List';

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.lk335a_autobroadcast
DROP TABLE IF EXISTS `lk335a_autobroadcast`;
CREATE TABLE IF NOT EXISTS `lk335a_autobroadcast` (
  `realmid` int NOT NULL DEFAULT '-1',
  `id` tinyint unsigned NOT NULL AUTO_INCREMENT,
  `weight` tinyint unsigned DEFAULT '1',
  `text` longtext NOT NULL,
  PRIMARY KEY (`id`,`realmid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.lk335a_ip2nation
DROP TABLE IF EXISTS `lk335a_ip2nation`;
CREATE TABLE IF NOT EXISTS `lk335a_ip2nation` (
  `ip` int unsigned NOT NULL DEFAULT '0',
  `country` char(2) NOT NULL DEFAULT '',
  KEY `ip` (`ip`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.lk335a_ip2nationcountries
DROP TABLE IF EXISTS `lk335a_ip2nationcountries`;
CREATE TABLE IF NOT EXISTS `lk335a_ip2nationcountries` (
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

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.lk335a_logs
DROP TABLE IF EXISTS `lk335a_logs`;
CREATE TABLE IF NOT EXISTS `lk335a_logs` (
  `time` int unsigned NOT NULL,
  `realm` int unsigned NOT NULL,
  `type` varchar(250) NOT NULL,
  `level` tinyint unsigned NOT NULL DEFAULT '0',
  `string` text CHARACTER SET latin1 COLLATE latin1_swedish_ci
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.lk335a_logs_ip_actions
DROP TABLE IF EXISTS `lk335a_logs_ip_actions`;
CREATE TABLE IF NOT EXISTS `lk335a_logs_ip_actions` (
  `id` int unsigned NOT NULL AUTO_INCREMENT COMMENT 'Unique Identifier',
  `account_id` int unsigned NOT NULL COMMENT 'Account ID',
  `character_guid` int unsigned NOT NULL COMMENT 'Character Guid',
  `type` tinyint unsigned NOT NULL,
  `ip` varchar(15) NOT NULL DEFAULT '127.0.0.1',
  `systemnote` text COMMENT 'Notes inserted by system',
  `unixtime` int unsigned NOT NULL COMMENT 'Unixtime',
  `time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT 'Timestamp',
  `comment` text COMMENT 'Allows users to add a comment',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='Used to log ips of individual actions';

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.lk335a_rbac_account_groups
DROP TABLE IF EXISTS `lk335a_rbac_account_groups`;
CREATE TABLE IF NOT EXISTS `lk335a_rbac_account_groups` (
  `accountId` int unsigned NOT NULL COMMENT 'Account id',
  `groupId` int unsigned NOT NULL COMMENT 'Group id',
  `realmId` int NOT NULL DEFAULT '-1' COMMENT 'Realm Id, -1 means all',
  PRIMARY KEY (`accountId`,`groupId`,`realmId`),
  KEY `fk__rbac_account_groups__rbac_groups` (`groupId`) USING BTREE
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='Account-Group relation';

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.lk335a_rbac_account_permissions
DROP TABLE IF EXISTS `lk335a_rbac_account_permissions`;
CREATE TABLE IF NOT EXISTS `lk335a_rbac_account_permissions` (
  `accountId` int unsigned NOT NULL COMMENT 'Account id',
  `permissionId` int unsigned NOT NULL COMMENT 'Permission id',
  `granted` tinyint(1) NOT NULL DEFAULT '1' COMMENT 'Granted = 1, Denied = 0',
  `realmId` int NOT NULL DEFAULT '-1' COMMENT 'Realm Id, -1 means all',
  PRIMARY KEY (`accountId`,`permissionId`,`realmId`),
  KEY `fk__rbac_account_roles__rbac_permissions` (`permissionId`),
  CONSTRAINT `lk335a_rbac_account_permissions_ibfk_2` FOREIGN KEY (`permissionId`) REFERENCES `lk335a_rbac_permissions` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='Account-Permission relation';

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.lk335a_rbac_default_permissions
DROP TABLE IF EXISTS `lk335a_rbac_default_permissions`;
CREATE TABLE IF NOT EXISTS `lk335a_rbac_default_permissions` (
  `secId` int unsigned NOT NULL COMMENT 'Security Level id',
  `permissionId` int unsigned NOT NULL COMMENT 'permission id',
  `realmId` int NOT NULL DEFAULT '-1' COMMENT 'Realm Id, -1 means all',
  PRIMARY KEY (`secId`,`permissionId`,`realmId`),
  KEY `fk__rbac_default_permissions__rbac_permissions` (`permissionId`),
  CONSTRAINT `fk__rbac_default_permissions__rbac_permissions` FOREIGN KEY (`permissionId`) REFERENCES `lk335a_rbac_permissions` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='Default permission to assign to different account security levels';

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.lk335a_rbac_group_roles
DROP TABLE IF EXISTS `lk335a_rbac_group_roles`;
CREATE TABLE IF NOT EXISTS `lk335a_rbac_group_roles` (
  `groupId` int unsigned NOT NULL COMMENT 'group id',
  `roleId` int unsigned NOT NULL COMMENT 'Role id',
  PRIMARY KEY (`groupId`,`roleId`),
  KEY `fk__rbac_group_roles__rbac_roles` (`roleId`) USING BTREE
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='Group Role relation';

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.lk335a_rbac_linked_permissions
DROP TABLE IF EXISTS `lk335a_rbac_linked_permissions`;
CREATE TABLE IF NOT EXISTS `lk335a_rbac_linked_permissions` (
  `id` int unsigned NOT NULL COMMENT 'Permission id',
  `linkedId` int unsigned NOT NULL COMMENT 'Linked Permission id',
  PRIMARY KEY (`id`,`linkedId`),
  KEY `fk__rbac_linked_permissions__rbac_permissions1` (`id`),
  KEY `fk__rbac_linked_permissions__rbac_permissions2` (`linkedId`),
  CONSTRAINT `fk__rbac_linked_permissions__rbac_permissions1` FOREIGN KEY (`id`) REFERENCES `lk335a_rbac_permissions` (`id`) ON DELETE CASCADE,
  CONSTRAINT `fk__rbac_linked_permissions__rbac_permissions2` FOREIGN KEY (`linkedId`) REFERENCES `lk335a_rbac_permissions` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='Permission - Linked Permission relation';

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.lk335a_rbac_permissions
DROP TABLE IF EXISTS `lk335a_rbac_permissions`;
CREATE TABLE IF NOT EXISTS `lk335a_rbac_permissions` (
  `id` int unsigned NOT NULL DEFAULT '0' COMMENT 'Permission id',
  `name` varchar(100) NOT NULL COMMENT 'Permission name',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='Permission List';

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.lk335a_updates
DROP TABLE IF EXISTS `lk335a_updates`;
CREATE TABLE IF NOT EXISTS `lk335a_updates` (
  `name` varchar(200) NOT NULL COMMENT 'filename with extension of the update.',
  `hash` char(40) DEFAULT '' COMMENT 'sha1 hash of the sql file.',
  `state` enum('RELEASED','ARCHIVED') NOT NULL DEFAULT 'RELEASED' COMMENT 'defines if an update is released or archived.',
  `timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT 'timestamp when the query was applied.',
  `speed` int unsigned NOT NULL DEFAULT '0' COMMENT 'time the query takes to apply in ms.',
  PRIMARY KEY (`name`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='List of all applied updates in this database.';

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.lk335a_updates_include
DROP TABLE IF EXISTS `lk335a_updates_include`;
CREATE TABLE IF NOT EXISTS `lk335a_updates_include` (
  `path` varchar(200) NOT NULL COMMENT 'directory to include. $ means relative to the source directory.',
  `state` enum('RELEASED','ARCHIVED') NOT NULL DEFAULT 'RELEASED' COMMENT 'defines if the directory contains released or archived updates.',
  PRIMARY KEY (`path`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='List of directories where we want to include sql updates.';

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.logs
DROP TABLE IF EXISTS `logs`;
CREATE TABLE IF NOT EXISTS `logs` (
  `time` int unsigned NOT NULL,
  `realm` int unsigned NOT NULL,
  `type` tinyint unsigned NOT NULL,
  `level` tinyint unsigned NOT NULL DEFAULT '0',
  `string` text CHARACTER SET latin1 COLLATE latin1_swedish_ci
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.mop548_account_access
DROP TABLE IF EXISTS `mop548_account_access`;
CREATE TABLE IF NOT EXISTS `mop548_account_access` (
  `id` int unsigned NOT NULL,
  `gmlevel` tinyint unsigned NOT NULL,
  `RealmID` int NOT NULL DEFAULT '-1',
  `comment` text,
  PRIMARY KEY (`id`,`RealmID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.mop548_account_log_ip
DROP TABLE IF EXISTS `mop548_account_log_ip`;
CREATE TABLE IF NOT EXISTS `mop548_account_log_ip` (
  `accountid` int unsigned NOT NULL,
  `ip` varchar(30) NOT NULL DEFAULT '0.0.0.0',
  `date` datetime DEFAULT NULL,
  PRIMARY KEY (`accountid`,`ip`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.mop548_account_premium
DROP TABLE IF EXISTS `mop548_account_premium`;
CREATE TABLE IF NOT EXISTS `mop548_account_premium` (
  `id` int NOT NULL DEFAULT '0' COMMENT 'Account id',
  `setdate` bigint NOT NULL DEFAULT '0',
  `unsetdate` bigint NOT NULL DEFAULT '0',
  `premium_type` tinyint unsigned NOT NULL DEFAULT '1',
  `gm` varchar(12) NOT NULL DEFAULT '0',
  `active` tinyint NOT NULL DEFAULT '1',
  PRIMARY KEY (`id`,`setdate`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.mop548_account_spell
DROP TABLE IF EXISTS `mop548_account_spell`;
CREATE TABLE IF NOT EXISTS `mop548_account_spell` (
  `accountId` bigint NOT NULL,
  `spell` int NOT NULL,
  `active` tinyint(1) DEFAULT NULL,
  `disabled` tinyint(1) DEFAULT NULL,
  PRIMARY KEY (`accountId`,`spell`),
  KEY `account` (`accountId`),
  KEY `account_spell` (`accountId`,`spell`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.mop548_account_spell_tournament
DROP TABLE IF EXISTS `mop548_account_spell_tournament`;
CREATE TABLE IF NOT EXISTS `mop548_account_spell_tournament` (
  `accountId` int unsigned NOT NULL,
  `spell` int NOT NULL,
  `active` tinyint(1) DEFAULT NULL,
  `disabled` tinyint(1) DEFAULT NULL,
  PRIMARY KEY (`accountId`,`spell`),
  KEY `account` (`accountId`),
  KEY `account_spell` (`accountId`,`spell`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.mop548_account_vip
DROP TABLE IF EXISTS `mop548_account_vip`;
CREATE TABLE IF NOT EXISTS `mop548_account_vip` (
  `acc_id` int NOT NULL,
  `viplevel` tinyint NOT NULL DEFAULT '1',
  `active` tinyint NOT NULL DEFAULT '1',
  PRIMARY KEY (`acc_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.mop548_chat_filter
DROP TABLE IF EXISTS `mop548_chat_filter`;
CREATE TABLE IF NOT EXISTS `mop548_chat_filter` (
  `id` int unsigned NOT NULL AUTO_INCREMENT COMMENT 'Identifier',
  `text` char(64) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.mop548_firewall_farms
DROP TABLE IF EXISTS `mop548_firewall_farms`;
CREATE TABLE IF NOT EXISTS `mop548_firewall_farms` (
  `ip` tinytext NOT NULL,
  `type` tinyint unsigned NOT NULL DEFAULT '0'
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.mop548_ip_to_country
DROP TABLE IF EXISTS `mop548_ip_to_country`;
CREATE TABLE IF NOT EXISTS `mop548_ip_to_country` (
  `IP_FROM` double NOT NULL,
  `IP_TO` double NOT NULL,
  `COUNTRY_CODE` char(2) DEFAULT NULL,
  `COUNTRY_CODE_3` char(3) NOT NULL,
  `COUNTRY_NAME` varchar(50) DEFAULT NULL,
  PRIMARY KEY (`IP_FROM`,`IP_TO`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.mop548_logs
DROP TABLE IF EXISTS `mop548_logs`;
CREATE TABLE IF NOT EXISTS `mop548_logs` (
  `time` int unsigned NOT NULL,
  `realm` int unsigned NOT NULL,
  `type` tinyint unsigned NOT NULL,
  `level` tinyint unsigned NOT NULL DEFAULT '0',
  `string` text CHARACTER SET latin1 COLLATE latin1_swedish_ci
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.mop548_log_vote
DROP TABLE IF EXISTS `mop548_log_vote`;
CREATE TABLE IF NOT EXISTS `mop548_log_vote` (
  `top_name` varchar(15) NOT NULL DEFAULT 'top',
  `ip` varchar(15) NOT NULL DEFAULT '0.0.0.0',
  `date` int NOT NULL,
  PRIMARY KEY (`top_name`,`ip`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.mop548_mails
DROP TABLE IF EXISTS `mop548_mails`;
CREATE TABLE IF NOT EXISTS `mop548_mails` (
  `email` varchar(254) NOT NULL DEFAULT ''
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.mop548_transferts
DROP TABLE IF EXISTS `mop548_transferts`;
CREATE TABLE IF NOT EXISTS `mop548_transferts` (
  `id` int NOT NULL AUTO_INCREMENT,
  `account` int NOT NULL,
  `perso_guid` int NOT NULL,
  `from` int NOT NULL,
  `to` int NOT NULL,
  `revision` blob NOT NULL,
  `dump` longtext NOT NULL,
  `last_error` blob NOT NULL,
  `nb_attempt` int NOT NULL,
  `state` int DEFAULT NULL,
  `error` int DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.mop548_transferts_logs
DROP TABLE IF EXISTS `mop548_transferts_logs`;
CREATE TABLE IF NOT EXISTS `mop548_transferts_logs` (
  `id` int DEFAULT NULL,
  `account` int DEFAULT NULL,
  `perso_guid` int DEFAULT NULL,
  `from` int DEFAULT NULL,
  `to` int DEFAULT NULL,
  `dump` longtext
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.rbac_account_groups
DROP TABLE IF EXISTS `rbac_account_groups`;
CREATE TABLE IF NOT EXISTS `rbac_account_groups` (
  `accountId` int unsigned NOT NULL COMMENT 'Account id',
  `groupId` int unsigned NOT NULL COMMENT 'Group id',
  `realmId` int NOT NULL DEFAULT '-1' COMMENT 'Realm Id, -1 means all',
  PRIMARY KEY (`accountId`,`groupId`,`realmId`),
  KEY `fk__rbac_account_groups__rbac_groups` (`groupId`) USING BTREE
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='Account-Group relation';

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.rbac_account_permissions
DROP TABLE IF EXISTS `rbac_account_permissions`;
CREATE TABLE IF NOT EXISTS `rbac_account_permissions` (
  `accountId` int unsigned NOT NULL COMMENT 'Account id',
  `permissionId` int unsigned NOT NULL COMMENT 'Permission id',
  `granted` tinyint(1) NOT NULL DEFAULT '1' COMMENT 'Granted = 1, Denied = 0',
  `realmId` int NOT NULL DEFAULT '-1' COMMENT 'Realm Id, -1 means all',
  PRIMARY KEY (`accountId`,`permissionId`,`realmId`),
  KEY `fk__rbac_account_roles__rbac_permissions` (`permissionId`) USING BTREE
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='Account-Permission relation';

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.rbac_account_roles
DROP TABLE IF EXISTS `rbac_account_roles`;
CREATE TABLE IF NOT EXISTS `rbac_account_roles` (
  `accountId` int unsigned NOT NULL COMMENT 'Account id',
  `roleId` int unsigned NOT NULL COMMENT 'Role id',
  `granted` tinyint(1) NOT NULL DEFAULT '1' COMMENT 'Granted = 1, Denied = 0',
  `realmId` int NOT NULL DEFAULT '-1' COMMENT 'Realm Id, -1 means all',
  PRIMARY KEY (`accountId`,`roleId`,`realmId`),
  KEY `fk__rbac_account_roles__rbac_roles` (`roleId`) USING BTREE
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='Account-Role relation';

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.rbac_groups
DROP TABLE IF EXISTS `rbac_groups`;
CREATE TABLE IF NOT EXISTS `rbac_groups` (
  `id` int unsigned NOT NULL DEFAULT '0' COMMENT 'Group id',
  `name` varchar(100) NOT NULL COMMENT 'Group name',
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='Group List';

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.rbac_group_roles
DROP TABLE IF EXISTS `rbac_group_roles`;
CREATE TABLE IF NOT EXISTS `rbac_group_roles` (
  `groupId` int unsigned NOT NULL COMMENT 'group id',
  `roleId` int unsigned NOT NULL COMMENT 'Role id',
  PRIMARY KEY (`groupId`,`roleId`),
  KEY `fk__rbac_group_roles__rbac_roles` (`roleId`) USING BTREE
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='Group Role relation';

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.rbac_permissions___
DROP TABLE IF EXISTS `rbac_permissions___`;
CREATE TABLE IF NOT EXISTS `rbac_permissions___` (
  `id` int unsigned NOT NULL DEFAULT '0' COMMENT 'Permission id',
  `name` varchar(100) NOT NULL COMMENT 'Permission name',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='Permission List';

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.rbac_roles
DROP TABLE IF EXISTS `rbac_roles`;
CREATE TABLE IF NOT EXISTS `rbac_roles` (
  `id` int unsigned NOT NULL DEFAULT '0' COMMENT 'Role id',
  `name` varchar(100) NOT NULL COMMENT 'Role name',
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='Roles List';

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.rbac_role_permissions
DROP TABLE IF EXISTS `rbac_role_permissions`;
CREATE TABLE IF NOT EXISTS `rbac_role_permissions` (
  `roleId` int unsigned NOT NULL COMMENT 'Role id',
  `permissionId` int unsigned NOT NULL COMMENT 'Permission id',
  PRIMARY KEY (`roleId`,`permissionId`),
  KEY `fk__role_permissions__rbac_permissions` (`permissionId`) USING BTREE
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='Role Permission relation';

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.rbac_security_level_groups
DROP TABLE IF EXISTS `rbac_security_level_groups`;
CREATE TABLE IF NOT EXISTS `rbac_security_level_groups` (
  `secId` tinyint unsigned NOT NULL COMMENT 'Security Level id',
  `groupId` int unsigned NOT NULL COMMENT 'group id',
  PRIMARY KEY (`secId`,`groupId`),
  KEY `fk__rbac_security_level_groups__rbac_groups` (`groupId`) USING BTREE
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='Default groups to assign when an account is set gm level';

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.realmcharacters
DROP TABLE IF EXISTS `realmcharacters`;
CREATE TABLE IF NOT EXISTS `realmcharacters` (
  `realmid` int unsigned NOT NULL DEFAULT '0',
  `acctid` int unsigned NOT NULL,
  `numchars` tinyint unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`realmid`,`acctid`),
  KEY `acctid` (`acctid`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Realm Character Tracker';

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.realmlist
DROP TABLE IF EXISTS `realmlist`;
CREATE TABLE IF NOT EXISTS `realmlist` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `name` varchar(32) NOT NULL DEFAULT '',
  `address` varchar(255) NOT NULL DEFAULT '127.0.0.1',
  `localAddress` varchar(255) NOT NULL DEFAULT '127.0.0.1',
  `localSubnetMask` varchar(255) NOT NULL DEFAULT '255.255.255.0',
  `port` smallint unsigned NOT NULL DEFAULT '8085',
  `icon` tinyint unsigned NOT NULL DEFAULT '0',
  `flag` tinyint unsigned NOT NULL DEFAULT '2',
  `timezone` tinyint unsigned NOT NULL DEFAULT '0',
  `allowedSecurityLevel` tinyint unsigned NOT NULL DEFAULT '0',
  `motd` varchar(255) NOT NULL DEFAULT 'Welcome on our server.',
  `population` float unsigned NOT NULL DEFAULT '0',
  `gamebuild` int unsigned NOT NULL DEFAULT '15595',
  `online` int DEFAULT '0',
  `delay` int unsigned NOT NULL DEFAULT '0',
  `queue` int unsigned NOT NULL DEFAULT '0',
  `lastupdate` int unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`),
  UNIQUE KEY `idx_name` (`name`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=2 DEFAULT CHARSET=utf8 COMMENT='Realm System';

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.uptime
DROP TABLE IF EXISTS `uptime`;
CREATE TABLE IF NOT EXISTS `uptime` (
  `realmid` int unsigned NOT NULL,
  `starttime` int unsigned NOT NULL DEFAULT '0',
  `uptime` int unsigned NOT NULL DEFAULT '0',
  `maxplayers` smallint unsigned NOT NULL DEFAULT '0',
  `revision` varchar(255) NOT NULL DEFAULT 'Trinitycore',
  PRIMARY KEY (`realmid`,`starttime`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='Uptime system';

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.user_reporting
DROP TABLE IF EXISTS `user_reporting`;
CREATE TABLE IF NOT EXISTS `user_reporting` (
  `ga` int unsigned NOT NULL,
  `account_id` int unsigned NOT NULL,
  `creation_date` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `step` int NOT NULL DEFAULT '0',
  `referer` varchar(255) NOT NULL,
  `last_ip` varchar(255) NOT NULL,
  `has_view_creation` tinyint unsigned DEFAULT '0',
  PRIMARY KEY (`ga`,`account_id`),
  KEY `ga_idx` (`ga`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 ROW_FORMAT=DYNAMIC;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.vip_accounts
DROP TABLE IF EXISTS `vip_accounts`;
CREATE TABLE IF NOT EXISTS `vip_accounts` (
  `id` int NOT NULL DEFAULT '0' COMMENT 'Account id',
  `type` tinyint unsigned NOT NULL DEFAULT '0',
  `setdate` bigint NOT NULL DEFAULT '0',
  `unsetdate` bigint NOT NULL DEFAULT '0',
  `active` tinyint NOT NULL DEFAULT '1',
  PRIMARY KEY (`id`,`setdate`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='VIP Accounts';

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.wod623_account_access
DROP TABLE IF EXISTS `wod623_account_access`;
CREATE TABLE IF NOT EXISTS `wod623_account_access` (
  `id` int unsigned NOT NULL,
  `gmlevel` tinyint unsigned NOT NULL,
  `RealmID` int NOT NULL DEFAULT '-1',
  `comment` text,
  PRIMARY KEY (`id`,`RealmID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.wod623_account_battlepay
DROP TABLE IF EXISTS `wod623_account_battlepay`;
CREATE TABLE IF NOT EXISTS `wod623_account_battlepay` (
  `accountId` int unsigned NOT NULL,
  `points` int NOT NULL,
  PRIMARY KEY (`accountId`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1 ROW_FORMAT=DYNAMIC;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.wod623_account_battlepet
DROP TABLE IF EXISTS `wod623_account_battlepet`;
CREATE TABLE IF NOT EXISTS `wod623_account_battlepet` (
  `id` bigint unsigned NOT NULL AUTO_INCREMENT,
  `account` int unsigned NOT NULL DEFAULT '0',
  `slot` int NOT NULL DEFAULT '-1',
  `name` varchar(50) NOT NULL,
  `nameTimeStamp` int unsigned NOT NULL DEFAULT '0',
  `species` int unsigned NOT NULL DEFAULT '0',
  `quality` int unsigned NOT NULL DEFAULT '0',
  `breed` int unsigned NOT NULL DEFAULT '0',
  `level` int unsigned NOT NULL DEFAULT '0',
  `xp` int unsigned NOT NULL DEFAULT '0',
  `display` int unsigned NOT NULL DEFAULT '0',
  `health` int NOT NULL DEFAULT '0',
  `flags` int unsigned NOT NULL DEFAULT '0',
  `infoPower` int NOT NULL DEFAULT '0',
  `infoMaxHealth` int NOT NULL DEFAULT '0',
  `infoSpeed` int NOT NULL DEFAULT '0',
  `infoGender` int NOT NULL DEFAULT '0',
  `declinedGenitive` varchar(50) NOT NULL,
  `declinedNative` varchar(50) NOT NULL,
  `declinedAccusative` varchar(50) NOT NULL,
  `declinedInstrumental` varchar(50) NOT NULL,
  `declinedPrepositional` varchar(50) NOT NULL,
  PRIMARY KEY (`id`),
  KEY `account` (`account`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 ROW_FORMAT=DYNAMIC;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.wod623_account_heirlooms
DROP TABLE IF EXISTS `wod623_account_heirlooms`;
CREATE TABLE IF NOT EXISTS `wod623_account_heirlooms` (
  `account_id` int unsigned NOT NULL DEFAULT '0',
  `heirloom_id` int unsigned NOT NULL DEFAULT '0',
  `upgrade_flags` int unsigned NOT NULL DEFAULT '0',
  `groupRealmMask` int unsigned NOT NULL,
  PRIMARY KEY (`account_id`,`heirloom_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1 ROW_FORMAT=DYNAMIC;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.wod623_account_log_ip
DROP TABLE IF EXISTS `wod623_account_log_ip`;
CREATE TABLE IF NOT EXISTS `wod623_account_log_ip` (
  `accountid` int unsigned NOT NULL,
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `ip` varchar(30) NOT NULL DEFAULT '0.0.0.0',
  `date` datetime DEFAULT NULL,
  `error` int unsigned NOT NULL DEFAULT '0',
  `source` int unsigned NOT NULL DEFAULT '2',
  PRIMARY KEY (`accountid`,`ip`,`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1 ROW_FORMAT=DYNAMIC;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.wod623_account_loyalty
DROP TABLE IF EXISTS `wod623_account_loyalty`;
CREATE TABLE IF NOT EXISTS `wod623_account_loyalty` (
  `AccountID` int unsigned NOT NULL,
  `LastClaim` int NOT NULL DEFAULT '0',
  `LastEventReset` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`AccountID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.wod623_account_loyalty_event
DROP TABLE IF EXISTS `wod623_account_loyalty_event`;
CREATE TABLE IF NOT EXISTS `wod623_account_loyalty_event` (
  `AccountID` int unsigned NOT NULL,
  `Event` int unsigned NOT NULL DEFAULT '0',
  `Count` int unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`AccountID`,`Event`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.wod623_account_note
DROP TABLE IF EXISTS `wod623_account_note`;
CREATE TABLE IF NOT EXISTS `wod623_account_note` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `account_id` int unsigned DEFAULT NULL,
  `note` text,
  `date` int DEFAULT NULL,
  `notedby` varchar(255) DEFAULT NULL,
  KEY `id` (`id`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.wod623_account_premium
DROP TABLE IF EXISTS `wod623_account_premium`;
CREATE TABLE IF NOT EXISTS `wod623_account_premium` (
  `id` int NOT NULL DEFAULT '0' COMMENT 'Account id',
  `setdate` bigint NOT NULL DEFAULT '0',
  `unsetdate` bigint NOT NULL DEFAULT '0',
  `premium_type` tinyint unsigned NOT NULL DEFAULT '1',
  `gm` varchar(12) NOT NULL DEFAULT '0',
  `active` tinyint NOT NULL DEFAULT '1',
  PRIMARY KEY (`id`,`setdate`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.wod623_account_spell
DROP TABLE IF EXISTS `wod623_account_spell`;
CREATE TABLE IF NOT EXISTS `wod623_account_spell` (
  `accountId` bigint NOT NULL,
  `spell` int NOT NULL,
  `active` tinyint(1) DEFAULT NULL,
  `disabled` tinyint(1) DEFAULT NULL,
  `IsMountFavorite` tinyint(1) NOT NULL DEFAULT '0',
  `groupRealmMask` int unsigned NOT NULL DEFAULT '1',
  PRIMARY KEY (`accountId`,`spell`),
  KEY `account` (`accountId`),
  KEY `account_spell` (`accountId`,`spell`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.wod623_account_spell_old
DROP TABLE IF EXISTS `wod623_account_spell_old`;
CREATE TABLE IF NOT EXISTS `wod623_account_spell_old` (
  `accountId` int NOT NULL,
  `spell` int NOT NULL,
  `active` tinyint(1) DEFAULT NULL,
  `disabled` tinyint(1) DEFAULT NULL,
  `IsMountFavorite` tinyint(1) NOT NULL DEFAULT '0',
  `groupRealmMask` int unsigned NOT NULL,
  PRIMARY KEY (`accountId`,`spell`),
  KEY `account` (`accountId`),
  KEY `account_spell` (`accountId`,`spell`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.wod623_account_toys
DROP TABLE IF EXISTS `wod623_account_toys`;
CREATE TABLE IF NOT EXISTS `wod623_account_toys` (
  `account_id` int unsigned NOT NULL,
  `item_id` int unsigned NOT NULL,
  `is_favorite` tinyint(1) NOT NULL DEFAULT '0',
  PRIMARY KEY (`account_id`,`item_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1 ROW_FORMAT=DYNAMIC;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.wod623_account_vote
DROP TABLE IF EXISTS `wod623_account_vote`;
CREATE TABLE IF NOT EXISTS `wod623_account_vote` (
  `account` int unsigned NOT NULL,
  `remainingTime` int unsigned NOT NULL,
  PRIMARY KEY (`account`),
  UNIQUE KEY `account` (`account`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 ROW_FORMAT=DYNAMIC;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.wod623_autobroadcast
DROP TABLE IF EXISTS `wod623_autobroadcast`;
CREATE TABLE IF NOT EXISTS `wod623_autobroadcast` (
  `Expension` int DEFAULT NULL,
  `RealmID` int DEFAULT NULL,
  `Text` blob,
  `TextFR` blob,
  `TextES` blob,
  `textRU` blob
) ENGINE=InnoDB DEFAULT CHARSET=latin1 ROW_FORMAT=DYNAMIC;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.wod623_battlenet_modules
DROP TABLE IF EXISTS `wod623_battlenet_modules`;
CREATE TABLE IF NOT EXISTS `wod623_battlenet_modules` (
  `Hash` varchar(64) NOT NULL,
  `Name` varchar(64) NOT NULL DEFAULT '',
  `Type` varchar(8) NOT NULL,
  `System` varchar(8) NOT NULL,
  `Data` text,
  PRIMARY KEY (`Name`,`System`),
  UNIQUE KEY `uk_name_type_system` (`Name`,`Type`,`System`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.wod623_bnet_allowed_build
DROP TABLE IF EXISTS `wod623_bnet_allowed_build`;
CREATE TABLE IF NOT EXISTS `wod623_bnet_allowed_build` (
  `build` int DEFAULT NULL,
  `version` varchar(40) DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1 ROW_FORMAT=DYNAMIC;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.wod623_character_renderer_queue
DROP TABLE IF EXISTS `wod623_character_renderer_queue`;
CREATE TABLE IF NOT EXISTS `wod623_character_renderer_queue` (
  `guid` int DEFAULT NULL,
  `race` tinyint DEFAULT NULL,
  `gender` tinyint DEFAULT NULL,
  `class` tinyint DEFAULT NULL,
  `skinColor` tinyint DEFAULT NULL,
  `face` tinyint DEFAULT NULL,
  `hairStyle` tinyint DEFAULT NULL,
  `hairColor` tinyint DEFAULT NULL,
  `facialHair` tinyint DEFAULT NULL,
  `equipment` blob
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin ROW_FORMAT=DYNAMIC;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.wod623_firewall_farms
DROP TABLE IF EXISTS `wod623_firewall_farms`;
CREATE TABLE IF NOT EXISTS `wod623_firewall_farms` (
  `ip` tinytext NOT NULL,
  `type` tinyint unsigned NOT NULL DEFAULT '0'
) ENGINE=InnoDB DEFAULT CHARSET=latin1 ROW_FORMAT=DYNAMIC;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.wod623_ip_to_country
DROP TABLE IF EXISTS `wod623_ip_to_country`;
CREATE TABLE IF NOT EXISTS `wod623_ip_to_country` (
  `IP_FROM` bigint unsigned NOT NULL,
  `IP_TO` bigint unsigned NOT NULL,
  `REGISTRY` char(7) NOT NULL,
  `ASSIGNED` bigint NOT NULL,
  `CTRY` char(2) NOT NULL,
  `CNTRY` char(3) NOT NULL,
  `COUNTRY` varchar(100) NOT NULL,
  PRIMARY KEY (`IP_FROM`,`IP_TO`),
  KEY `IP_FROM` (`IP_FROM`) USING BTREE,
  KEY `IP_TO` (`IP_TO`) USING BTREE
) ENGINE=MyISAM DEFAULT CHARSET=latin1 ROW_FORMAT=DYNAMIC;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.wod623_logs
DROP TABLE IF EXISTS `wod623_logs`;
CREATE TABLE IF NOT EXISTS `wod623_logs` (
  `time` int unsigned NOT NULL,
  `realm` int unsigned NOT NULL,
  `type` tinyint unsigned NOT NULL,
  `level` tinyint unsigned NOT NULL DEFAULT '0',
  `string` text CHARACTER SET latin1 COLLATE latin1_swedish_ci
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.wod623_log_vote
DROP TABLE IF EXISTS `wod623_log_vote`;
CREATE TABLE IF NOT EXISTS `wod623_log_vote` (
  `top_name` varchar(15) NOT NULL DEFAULT 'top',
  `ip` varchar(15) NOT NULL DEFAULT '0.0.0.0',
  `date` int NOT NULL,
  PRIMARY KEY (`top_name`,`ip`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1 ROW_FORMAT=DYNAMIC;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.wod623_motd
DROP TABLE IF EXISTS `wod623_motd`;
CREATE TABLE IF NOT EXISTS `wod623_motd` (
  `RealmID` int unsigned NOT NULL,
  `Text` text NOT NULL,
  `TextFR` text NOT NULL,
  `TextES` text NOT NULL,
  `TextRU` text NOT NULL,
  PRIMARY KEY (`RealmID`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1 ROW_FORMAT=DYNAMIC;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.wod623_stat_lifetime_retention
DROP TABLE IF EXISTS `wod623_stat_lifetime_retention`;
CREATE TABLE IF NOT EXISTS `wod623_stat_lifetime_retention` (
  `minutes` int NOT NULL,
  `usersPercentage` float(11,8) DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1 ROW_FORMAT=DYNAMIC;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.wod623_transferts
DROP TABLE IF EXISTS `wod623_transferts`;
CREATE TABLE IF NOT EXISTS `wod623_transferts` (
  `id` int NOT NULL AUTO_INCREMENT,
  `account` int NOT NULL,
  `perso_guid` int NOT NULL,
  `from` int NOT NULL,
  `to` int NOT NULL,
  `revision` blob NOT NULL,
  `dump` longtext NOT NULL,
  `last_error` blob NOT NULL,
  `nb_attempt` int NOT NULL,
  `state` int NOT NULL DEFAULT '0',
  `error` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1 ROW_FORMAT=DYNAMIC;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.wod623_transferts_logs
DROP TABLE IF EXISTS `wod623_transferts_logs`;
CREATE TABLE IF NOT EXISTS `wod623_transferts_logs` (
  `id` int DEFAULT NULL,
  `account` int DEFAULT NULL,
  `perso_guid` int DEFAULT NULL,
  `from` int DEFAULT NULL,
  `to` int DEFAULT NULL,
  `dump` longtext
) ENGINE=InnoDB DEFAULT CHARSET=latin1 ROW_FORMAT=DYNAMIC;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.wod623_webshop_delivery_interexp_transfer
DROP TABLE IF EXISTS `wod623_webshop_delivery_interexp_transfer`;
CREATE TABLE IF NOT EXISTS `wod623_webshop_delivery_interexp_transfer` (
  `id` int NOT NULL AUTO_INCREMENT,
  `account` int NOT NULL,
  `guid` int NOT NULL,
  `dump` longtext NOT NULL,
  `destrealm` int NOT NULL,
  `state` int NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1 ROW_FORMAT=DYNAMIC;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para tabla auth.wod623_webshop_delivery_interrealm_transfer
DROP TABLE IF EXISTS `wod623_webshop_delivery_interrealm_transfer`;
CREATE TABLE IF NOT EXISTS `wod623_webshop_delivery_interrealm_transfer` (
  `id` int NOT NULL AUTO_INCREMENT,
  `account` int unsigned NOT NULL DEFAULT '0',
  `guid` int unsigned NOT NULL DEFAULT '0',
  `startrealm` int unsigned NOT NULL DEFAULT '0',
  `destrealm` int unsigned NOT NULL DEFAULT '0',
  `revision` blob,
  `dump` longtext,
  `last_error` blob,
  `nb_attempt` int unsigned NOT NULL DEFAULT '0',
  `state` int unsigned NOT NULL DEFAULT '1',
  `error` int unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`),
  KEY `account` (`account`) USING BTREE,
  KEY `guid` (`guid`) USING BTREE,
  KEY `startrealm` (`startrealm`) USING BTREE,
  KEY `destrealm` (`destrealm`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 ROW_FORMAT=DYNAMIC;

-- La exportación de datos fue deseleccionada.

-- Volcando estructura para disparador auth.mail_update
DROP TRIGGER IF EXISTS `mail_update`;
SET @OLDTMP_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO';
DELIMITER //
CREATE TRIGGER `mail_update` BEFORE INSERT ON `account` FOR EACH ROW SET NEW.forum_email = CONCAT(NEW.username, "@rog.snet")//
DELIMITER ;
SET SQL_MODE=@OLDTMP_SQL_MODE;

/*!40101 SET SQL_MODE=IFNULL(@OLD_SQL_MODE, '') */;
/*!40014 SET FOREIGN_KEY_CHECKS=IF(@OLD_FOREIGN_KEY_CHECKS IS NULL, 1, @OLD_FOREIGN_KEY_CHECKS) */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

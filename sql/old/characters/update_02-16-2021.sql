RENAME TABLE `groups` TO `player_groups`;

ALTER TABLE `guild_member`
	CHANGE COLUMN `rank` `guild_rank` TINYINT(3) UNSIGNED NOT NULL AFTER `guid`,
	DROP INDEX `guildid_rank_key`,
	ADD INDEX `guildid_rank_key` (`guildid`, `guild_rank`) USING BTREE;

ALTER TABLE `gm_subsurveys`
	CHANGE COLUMN `rank` `subrank` INT(10) UNSIGNED NOT NULL DEFAULT '0' AFTER `subsurveyId`;

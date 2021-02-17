ALTER TABLE `creature_template`
	CHANGE COLUMN `rank` `npc_rank` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0' AFTER `scale`;

ALTER TABLE `spell_ranks`
	CHANGE COLUMN `rank` `spell_rank` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0' AFTER `spellId`,
	DROP PRIMARY KEY,
	ADD PRIMARY KEY (`first_spellId`, `spell_rank`) USING BTREE;

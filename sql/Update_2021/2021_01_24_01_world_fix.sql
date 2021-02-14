-- Kah'tir SAI
SET @ENTRY := 50355;
UPDATE `creature_template` SET `AIName`="SmartAI" WHERE `entry`=@ENTRY;
DELETE FROM `smart_scripts` WHERE `entryorguid`=@ENTRY AND `source_type`=0;
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES
(@ENTRY,0,0,0,0,0,100,0,0,7000,15000,15000,11,124946,0,0,0,0,0,2,0,0,0,0,0,0,0,"Kah'tir - In Combat - Cast '<Spell not found!>'"),
(@ENTRY,0,1,0,0,0,100,0,15000,15000,30000,30000,11,124980,0,0,0,0,0,2,0,0,0,0,0,0,0,"Kah'tir - In Combat - Cast '<Spell not found!>'"),
(@ENTRY,0,2,0,0,0,100,0,5000,15000,15000,30000,11,124976,0,0,0,0,0,2,0,0,0,0,0,0,0,"Kah'tir - In Combat - Cast '<Spell not found!>'"),
(@ENTRY,0,3,0,4,0,100,0,0,0,0,0,1,0,0,0,0,0,0,1,0,0,0,0,0,0,0,"Kah'tir - On Aggro - Say Line 0");

-- Nasra Spothide SAI
SET @ENTRY := 50811;
UPDATE `creature_template` SET `AIName`="SmartAI" WHERE `entry`=@ENTRY;
DELETE FROM `smart_scripts` WHERE `entryorguid`=@ENTRY AND `source_type`=0;
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES
(@ENTRY,0,0,0,25,0,100,0,0,0,0,0,11,122715,2,0,0,0,0,1,0,0,0,0,0,0,0,"Ahone the Wanderer - On Reset - Cast Meditation"),
(@ENTRY,0,1,0,0,0,100,0,0,0,0,0,28,122715,0,0,0,0,0,1,0,0,0,0,0,0,0,"Ahone the Wanderer - Remove Aura Meditation"),
(@ENTRY,0,2,0,0,0,100,0,0,0,0,0,11,125817,0,0,0,0,0,17,10,30,0,0,0,0,0,"Ahone the Wanderer - Chi Burts 10+ yd"),
(@ENTRY,0,3,0,0,0,100,0,6000,6000,15000,15000,11,125799,0,0,0,0,0,17,0,7,0,0,0,0,0,"Ahone the Wanderer - Spinning Crane kick"),
(@ENTRY,0,4,0,0,0,100,0,11000,13000,30000,32000,11,125802,0,0,0,0,0,1,0,0,0,0,0,0,0,"Ahone the Wanderer - Healing Mist"),
(@ENTRY,0,5,0,4,0,100,0,0,0,0,0,1,0,0,0,0,0,0,1,0,0,0,0,0,0,0,"Ahone the Wanderer - Aggro");


DELETE FROM `creature_addon` WHERE guid IN (126025,131951,81463,7112052);
DELETE FROM `linked_respawn` WHERE LINKEDGUID in (126025,131951);
DELETE FROM creature_loot_template WHERE entry= 72764 AND item =101776;
UPDATE  gameobject_loot_template SET ChanceOrQuestChance =100 WHERE entry=3524;
UPDATE  gameobject_loot_template SET groupid =0 WHERE entry=3524;
REPLACE INTO `gameobject_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) VALUES
(204280, 58798, 100, 1, 0, 1, 1) /* ottle of Whiskey */;
REPLACE INTO `gameobject_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) VALUES
(179828, 18943, 99, 1, 0, 1, 1) /* ark Iron Pillow */;
REPLACE INTO `gameobject_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) VALUES
(180373, 19997, 99, 1, 0, 5, 10) /* arvest Nectar */;
REPLACE INTO `gameobject_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) VALUES
(192824, 43138, 99, 1, 0, 1, 1) /* alf Full Dalaran Wine Glass */;
UPDATE  item_loot_template SET ChanceOrQuestChance=100 WHERE entry =90840 AND item =69991;
UPDATE gameobject_template SET data1 =179828 WHERE entry = 179828;
UPDATE gameobject_template SET data1 =180373 WHERE entry = 180373;
UPDATE gameobject_template SET data1 =192824 WHERE entry = 192824;
DELETE FROM gameobject_loot_template WHERE entry IN (16820);
DELETE FROM gameobject_loot_template WHERE entry IN (17326);
DELETE FROM gameobject_loot_template WHERE entry IN (25900);
DELETE  FROM `creature_questrelation` WHERE quest IN (61297,32390,32432); 

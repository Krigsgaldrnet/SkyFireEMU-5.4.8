UPDATE `creature_template` SET `AIName`='SmartAI' WHERE `entry`=39272;
DELETE FROM `smart_scripts` WHERE `entryorguid`=39272;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `event_param5`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES 
(39272, 0, 0, 0, 9, 0, 100, 0, 0, 5, 5000, 10000, 0, 11, 79831, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Foaming Sea Elemental - On Range - Cast Wave Crash'),
(39272, 0, 1, 0, 0, 0, 0, 0, 1000, 1000, 2500, 2500, 0, 11, 32011, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Foaming Sea Elemental - IC - Cast Water Bolt');

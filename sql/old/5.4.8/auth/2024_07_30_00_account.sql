ALTER TABLE `account`
    MODIFY COLUMN last_login TIMESTAMP NULL DEFAULT NULL,
    DROP COLUMN `sessionkey`,
    ADD COLUMN `salt` BINARY(32) AFTER `username`,
    ADD COLUMN `verifier` BINARY(32) AFTER `salt`,
    ADD COLUMN `session_key` BINARY(40) AFTER `verifier`,
    MODIFY COLUMN `s` VARCHAR(64) NOT NULL DEFAULT 'dummy value, use `salt` instead',
    MODIFY COLUMN `v` VARCHAR(64) NOT NULL DEFAULT 'dummy value, use `verifier` instead';

UPDATE `account` SET `salt`=REVERSE(UNHEX(`s`)), `s`=DEFAULT WHERE LENGTH(`s`)=64;
UPDATE `account` SET `verifier`=REVERSE(UNHEX(`v`)), `v`=DEFAULT WHERE LENGTH(`v`)=64;

ALTER TABLE `account`
    DROP COLUMN `session_key`,
    ADD COLUMN `session_key` BINARY(40) DEFAULT NULL AFTER `verifier`;

UPDATE `account` SET `salt`=UNHEX(CONCAT(MD5(RAND()),MD5(RAND()))), `verifier`=UNHEX(CONCAT(MD5(RAND()),MD5(RAND()))) WHERE `salt` IS NULL OR `verifier` IS NULL;

ALTER TABLE `account`
    DROP COLUMN `s`,
    DROP COLUMN `v`,
    DROP COLUMN `sha_pass_hash`,
    MODIFY COLUMN `salt` BINARY(32) NOT NULL,
    MODIFY COLUMN `verifier` BINARY(32) NOT NULL;

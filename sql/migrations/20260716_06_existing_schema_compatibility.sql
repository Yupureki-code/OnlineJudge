SET NAMES utf8mb4;

DELIMITER //
DROP PROCEDURE IF EXISTS migrate_20260716_06_existing_schema_compatibility//
CREATE PROCEDURE migrate_20260716_06_existing_schema_compatibility()
BEGIN
  IF EXISTS (
      SELECT 1 FROM information_schema.columns
      WHERE table_schema = DATABASE() AND table_name = 'users' AND column_name = 'avatar_path'
  ) AND NOT EXISTS (
      SELECT 1 FROM information_schema.columns
      WHERE table_schema = DATABASE() AND table_name = 'users' AND column_name = 'avatar'
  ) THEN
    ALTER TABLE `users`
      CHANGE COLUMN `avatar_path` `avatar`
      VARCHAR(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NULL;
  ELSEIF EXISTS (
      SELECT 1 FROM information_schema.columns
      WHERE table_schema = DATABASE() AND table_name = 'users' AND column_name = 'avatar_path'
  ) AND EXISTS (
      SELECT 1 FROM information_schema.columns
      WHERE table_schema = DATABASE() AND table_name = 'users' AND column_name = 'avatar'
  ) THEN
    UPDATE `users` SET `avatar` = `avatar_path`
    WHERE `avatar` IS NULL AND `avatar_path` IS NOT NULL;
    ALTER TABLE `users` DROP COLUMN `avatar_path`;
  END IF;

  IF EXISTS (
      SELECT 1 FROM information_schema.columns
      WHERE table_schema = DATABASE() AND table_name = 'questions' AND column_name = 'mem_limit'
  ) AND NOT EXISTS (
      SELECT 1 FROM information_schema.columns
      WHERE table_schema = DATABASE() AND table_name = 'questions' AND column_name = 'memory_limit'
  ) THEN
    ALTER TABLE `questions` CHANGE COLUMN `mem_limit` `memory_limit` INT NOT NULL;
  END IF;

  IF EXISTS (
      SELECT 1 FROM information_schema.tables
      WHERE table_schema = DATABASE() AND table_name = 'admin_audit_logs'
  ) AND NOT EXISTS (
      SELECT 1 FROM information_schema.statistics
      WHERE table_schema = DATABASE() AND table_name = 'admin_audit_logs'
        AND index_name = 'idx_operator_uid'
  ) THEN
    ALTER TABLE `admin_audit_logs` ADD KEY `idx_operator_uid` (`operator_uid`);
  END IF;
END//
CALL migrate_20260716_06_existing_schema_compatibility()//
DROP PROCEDURE migrate_20260716_06_existing_schema_compatibility//
DELIMITER ;

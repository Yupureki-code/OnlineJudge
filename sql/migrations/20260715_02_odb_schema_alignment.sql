-- Phase 4.1 alignment for the current SQL schema and the Phase 4.2 ODB mappings.
SET NAMES utf8mb4;

DELIMITER //
DROP PROCEDURE IF EXISTS migrate_20260715_02_existing_tables//
CREATE PROCEDURE migrate_20260715_02_existing_tables()
BEGIN
  IF EXISTS (
      SELECT 1 FROM information_schema.tables
      WHERE table_schema = DATABASE() AND table_name = 'users'
  ) AND NOT EXISTS (
      SELECT 1 FROM information_schema.columns
      WHERE table_schema = DATABASE() AND table_name = 'users' AND column_name = 'avatar'
  ) THEN
    ALTER TABLE `users`
      ADD COLUMN `avatar` VARCHAR(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NULL
      AFTER `email`;
  END IF;

  IF EXISTS (
      SELECT 1 FROM information_schema.tables
      WHERE table_schema = DATABASE() AND table_name = 'tests'
  ) AND NOT EXISTS (
      SELECT 1 FROM information_schema.columns
      WHERE table_schema = DATABASE() AND table_name = 'tests' AND column_name = 'test_case_id'
  ) THEN
    ALTER TABLE `tests`
      MODIFY COLUMN `id` TINYINT NOT NULL,
      DROP PRIMARY KEY,
      ADD COLUMN `test_case_id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT FIRST,
      ADD PRIMARY KEY (`test_case_id`),
      ADD UNIQUE KEY `uk_tests_question_id_id` (`question_id`, `id`);
  END IF;

  IF EXISTS (
      SELECT 1 FROM information_schema.tables
      WHERE table_schema = DATABASE() AND table_name = 'tests'
  ) AND NOT EXISTS (
      SELECT 1 FROM information_schema.statistics
      WHERE table_schema = DATABASE() AND table_name = 'tests'
        AND index_name = 'uk_tests_question_id_id'
  ) THEN
    ALTER TABLE `tests`
      ADD UNIQUE KEY `uk_tests_question_id_id` (`question_id`, `id`);
  END IF;

  IF EXISTS (
      SELECT 1 FROM information_schema.tables
      WHERE table_schema = DATABASE() AND table_name = 'admin_audit_logs'
  ) AND NOT EXISTS (
      SELECT 1 FROM information_schema.columns
      WHERE table_schema = DATABASE() AND table_name = 'admin_audit_logs'
        AND column_name = 'operator_uid'
  ) THEN
    ALTER TABLE `admin_audit_logs`
      ADD COLUMN `operator_uid` INT UNSIGNED NOT NULL DEFAULT 0
      AFTER `operator_admin_id`;
  END IF;
END//
CALL migrate_20260715_02_existing_tables()//
DROP PROCEDURE migrate_20260715_02_existing_tables//
DELIMITER ;

CREATE TABLE IF NOT EXISTS `solution_actions` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `solution_id` INT UNSIGNED NOT NULL,
  `user_id` INT UNSIGNED NOT NULL,
  `action_type` ENUM('like','comment','favorite') NOT NULL,
  `created_at` DATETIME NOT NULL,
  PRIMARY KEY (`id`),
  KEY `solutionid` (`solution_id`),
  KEY `userid` (`user_id`),
  KEY `idx_solution_action_lookup` (`solution_id`, `user_id`, `action_type`)
  ,CONSTRAINT `fk_solution_actions_solution` FOREIGN KEY (`solution_id`) REFERENCES `solutions` (`id`) ON DELETE CASCADE
  ,CONSTRAINT `fk_solution_actions_user` FOREIGN KEY (`user_id`) REFERENCES `users` (`uid`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `comment_actions` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `comment_id` INT UNSIGNED NOT NULL,
  `user_id` INT UNSIGNED NOT NULL,
  `action_type` ENUM('like','favorite') NOT NULL,
  `created_at` DATETIME NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_user_comment_action` (`user_id`, `comment_id`, `action_type`),
  KEY `idx_comment_id` (`comment_id`),
  KEY `idx_user_action` (`user_id`, `action_type`),
  KEY `idx_user_comments` (`comment_id`, `user_id`)
  ,CONSTRAINT `fk_comment_actions_comment` FOREIGN KEY (`comment_id`) REFERENCES `solution_comments` (`id`) ON DELETE CASCADE
  ,CONSTRAINT `fk_comment_actions_user` FOREIGN KEY (`user_id`) REFERENCES `users` (`uid`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
  COMMENT='Comment likes and favorites';

CREATE TABLE IF NOT EXISTS `admin_audit_logs` (
  `log_id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `request_id` VARCHAR(64) NOT NULL,
  `operator_admin_id` INT UNSIGNED NOT NULL,
  `operator_uid` INT UNSIGNED NOT NULL DEFAULT 0,
  `operator_role` ENUM('super_admin','admin') NOT NULL,
  `action` VARCHAR(64) NOT NULL,
  `resource_type` VARCHAR(64) NOT NULL,
  `result` ENUM('success','denied','failed') NOT NULL,
  `created_at` DATETIME NOT NULL,
  `payload_text` LONGTEXT NOT NULL,
  PRIMARY KEY (`log_id`),
  KEY `admin_role` (`operator_role`),
  KEY `idx_created_at` (`created_at`),
  KEY `idx_operator_time` (`operator_admin_id`, `created_at`),
  KEY `idx_action_time` (`action`, `created_at`),
  KEY `idx_resource_time` (`resource_type`, `created_at`),
  KEY `idx_request_id` (`request_id`),
  KEY `admin_id` (`operator_admin_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `cache_metrics_snapshots` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `action_type` TINYINT UNSIGNED NOT NULL,
  `total_requests` BIGINT UNSIGNED NOT NULL DEFAULT 0,
  `cache_hits` BIGINT UNSIGNED NOT NULL DEFAULT 0,
  `db_fallbacks` BIGINT UNSIGNED NOT NULL DEFAULT 0,
  `total_ms` BIGINT UNSIGNED NOT NULL DEFAULT 0,
  `created_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  KEY `idx_action_time` (`action_type`, `created_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
  COMMENT='Cache metric aggregate snapshots';

DELIMITER //
DROP PROCEDURE IF EXISTS migrate_20260715_02_missing_indexes//
CREATE PROCEDURE migrate_20260715_02_missing_indexes()
BEGIN
  IF EXISTS (SELECT 1 FROM information_schema.tables WHERE table_schema = DATABASE() AND table_name = 'solution_actions')
     AND NOT EXISTS (SELECT 1 FROM information_schema.statistics WHERE table_schema = DATABASE() AND table_name = 'solution_actions' AND index_name = 'idx_solution_action_lookup') THEN
    ALTER TABLE `solution_actions` ADD KEY `idx_solution_action_lookup` (`solution_id`, `user_id`, `action_type`);
  END IF;
  IF EXISTS (SELECT 1 FROM information_schema.tables WHERE table_schema = DATABASE() AND table_name = 'comment_actions')
     AND NOT EXISTS (SELECT 1 FROM information_schema.statistics WHERE table_schema = DATABASE() AND table_name = 'comment_actions' AND index_name = 'uk_user_comment_action') THEN
    ALTER TABLE `comment_actions` ADD UNIQUE KEY `uk_user_comment_action` (`user_id`, `comment_id`, `action_type`);
  END IF;
  IF EXISTS (SELECT 1 FROM information_schema.tables WHERE table_schema = DATABASE() AND table_name = 'admin_audit_logs')
     AND EXISTS (SELECT 1 FROM information_schema.statistics WHERE table_schema = DATABASE() AND table_name = 'admin_audit_logs' AND index_name = 'idx_operator_time')
     AND (SELECT GROUP_CONCAT(column_name ORDER BY seq_in_index)
          FROM information_schema.statistics
          WHERE table_schema = DATABASE() AND table_name = 'admin_audit_logs'
            AND index_name = 'idx_operator_time') <> 'operator_admin_id,created_at' THEN
    ALTER TABLE `admin_audit_logs` DROP INDEX `idx_operator_time`;
  END IF;
  IF EXISTS (SELECT 1 FROM information_schema.tables WHERE table_schema = DATABASE() AND table_name = 'admin_audit_logs')
     AND NOT EXISTS (SELECT 1 FROM information_schema.statistics WHERE table_schema = DATABASE() AND table_name = 'admin_audit_logs' AND index_name = 'idx_operator_time') THEN
    ALTER TABLE `admin_audit_logs` ADD KEY `idx_operator_time` (`operator_admin_id`, `created_at`);
  END IF;
  IF EXISTS (SELECT 1 FROM information_schema.tables WHERE table_schema = DATABASE() AND table_name = 'cache_metrics_snapshots')
     AND NOT EXISTS (SELECT 1 FROM information_schema.statistics WHERE table_schema = DATABASE() AND table_name = 'cache_metrics_snapshots' AND index_name = 'idx_action_time') THEN
    ALTER TABLE `cache_metrics_snapshots` ADD KEY `idx_action_time` (`action_type`, `created_at`);
  END IF;
END//
CALL migrate_20260715_02_missing_indexes()//
DROP PROCEDURE migrate_20260715_02_missing_indexes//
DELIMITER ;

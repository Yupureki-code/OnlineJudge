-- Phase 4.1 submission pipeline. This migration is additive and can be sourced repeatedly.
SET NAMES utf8mb4;

DELIMITER //
DROP PROCEDURE IF EXISTS migrate_20260715_01_legacy_submit_id//
CREATE PROCEDURE migrate_20260715_01_legacy_submit_id()
BEGIN
  IF EXISTS (
      SELECT 1 FROM information_schema.tables
      WHERE table_schema = DATABASE() AND table_name = 'user_submits'
  ) AND NOT EXISTS (
      SELECT 1 FROM information_schema.columns
      WHERE table_schema = DATABASE() AND table_name = 'user_submits' AND column_name = 'id'
  ) THEN
    ALTER TABLE `user_submits`
      ADD COLUMN `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT FIRST,
      ADD UNIQUE KEY `uk_user_submits_id` (`id`);
  END IF;
END//
CALL migrate_20260715_01_legacy_submit_id()//
DROP PROCEDURE migrate_20260715_01_legacy_submit_id//
DELIMITER ;

CREATE TABLE IF NOT EXISTS `submissions` (
  `submission_id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `user_id` INT UNSIGNED NOT NULL,
  `question_id` VARCHAR(5) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `code` LONGTEXT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NULL,
  `language` VARCHAR(32) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NULL,
  `status` VARCHAR(32) CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL DEFAULT 'PENDING',
  `result_json` LONGTEXT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NULL,
  `is_pass` TINYINT(1) NULL,
  `time_used_ms` BIGINT UNSIGNED NULL,
  `memory_used_bytes` BIGINT UNSIGNED NULL,
  `compile_error` TEXT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NULL,
  `created_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `completed_at` DATETIME NULL,
  `result_version` BIGINT UNSIGNED NOT NULL DEFAULT 0,
  `legacy_user_submit_id` BIGINT UNSIGNED NULL,
  PRIMARY KEY (`submission_id`),
  UNIQUE KEY `uk_submissions_legacy_submit` (`legacy_user_submit_id`),
  KEY `idx_submissions_user_created` (`user_id`, `created_at` DESC),
  KEY `idx_submissions_question_created` (`question_id`, `created_at` DESC),
  KEY `idx_submissions_status_created` (`status`, `created_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
  COMMENT='Code submissions for the asynchronous judge pipeline';

CREATE TABLE IF NOT EXISTS `judge_outbox` (
  `message_id` VARCHAR(64) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
  `task_type` VARCHAR(16) CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL,
  `submission_id` BIGINT UNSIGNED NULL,
  `custom_task_id` VARCHAR(64) CHARACTER SET ascii COLLATE ascii_bin NULL,
  `payload` LONGBLOB NOT NULL,
  `status` VARCHAR(16) CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL DEFAULT 'PENDING',
  `attempt_count` INT UNSIGNED NOT NULL DEFAULT 0,
  `next_retry_at` DATETIME NULL,
  `created_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `published_at` DATETIME NULL,
  PRIMARY KEY (`message_id`),
  KEY `idx_judge_outbox_dispatch` (`status`, `next_retry_at`, `created_at`),
  KEY `idx_judge_outbox_submission` (`submission_id`),
  KEY `idx_judge_outbox_custom_task` (`custom_task_id`),
  CONSTRAINT `chk_judge_outbox_target` CHECK (
    (`task_type` = 'submission' AND `submission_id` IS NOT NULL AND `custom_task_id` IS NULL)
    OR (`task_type` = 'custom' AND `submission_id` IS NULL AND `custom_task_id` IS NOT NULL)
  )
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
  COMMENT='Transactional judge message outbox';

CREATE TABLE IF NOT EXISTS `judge_result_receipts` (
  `message_id` VARCHAR(64) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
  `submission_id` BIGINT UNSIGNED NULL,
  `custom_task_id` VARCHAR(64) CHARACTER SET ascii COLLATE ascii_bin NULL,
  `result_payload` LONGBLOB NOT NULL,
  `created_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `applied_at` DATETIME NULL,
  `expires_at` DATETIME NULL,
  PRIMARY KEY (`message_id`),
  KEY `idx_judge_result_receipts_submission` (`submission_id`),
  KEY `idx_judge_result_receipts_custom_task` (`custom_task_id`),
  KEY `idx_judge_result_receipts_expires` (`expires_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
  COMMENT='Idempotent judge callback receipts';

DELIMITER //
DROP PROCEDURE IF EXISTS migrate_20260715_01_copy_legacy_submits//
CREATE PROCEDURE migrate_20260715_01_copy_legacy_submits()
BEGIN
  IF EXISTS (
      SELECT 1 FROM information_schema.columns
      WHERE table_schema = DATABASE() AND table_name = 'user_submits' AND column_name = 'id'
  ) THEN
    INSERT INTO `submissions` (
      `legacy_user_submit_id`, `user_id`, `question_id`, `status`, `result_json`,
      `is_pass`, `created_at`, `completed_at`, `result_version`
    )
    SELECT `id`, `user_id`, `question_id`,
           IF(`is_pass` = b'1', 'ACCEPTED', 'WRONG_ANSWER'),
           `result_json`, IF(`is_pass` = b'1', 1, 0), `action_time`, `action_time`, 1
    FROM `user_submits`
    ON DUPLICATE KEY UPDATE
      `user_id` = VALUES(`user_id`),
      `question_id` = VALUES(`question_id`),
      `status` = VALUES(`status`),
      `result_json` = VALUES(`result_json`),
      `is_pass` = VALUES(`is_pass`),
      `created_at` = VALUES(`created_at`),
      `completed_at` = VALUES(`completed_at`),
      `result_version` = VALUES(`result_version`);
  END IF;
END//
CALL migrate_20260715_01_copy_legacy_submits()//
DROP PROCEDURE migrate_20260715_01_copy_legacy_submits//
DELIMITER ;

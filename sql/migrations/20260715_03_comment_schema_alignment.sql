-- Phase 5 comment and interaction alignment for databases created before the
-- expanded solution_comments schema was introduced.
SET NAMES utf8mb4;

DELIMITER //
DROP PROCEDURE IF EXISTS migrate_20260715_03_comment_columns//
CREATE PROCEDURE migrate_20260715_03_comment_columns()
BEGIN
  IF EXISTS (
      SELECT 1 FROM information_schema.tables
      WHERE table_schema = DATABASE() AND table_name = 'solution_comments'
  ) AND NOT EXISTS (
      SELECT 1 FROM information_schema.columns
      WHERE table_schema = DATABASE() AND table_name = 'solution_comments'
        AND column_name = 'parent_id'
  ) THEN
    ALTER TABLE `solution_comments`
      ADD COLUMN `parent_id` BIGINT UNSIGNED NULL AFTER `id`;
  END IF;

  IF EXISTS (
      SELECT 1 FROM information_schema.tables
      WHERE table_schema = DATABASE() AND table_name = 'solution_comments'
  ) AND NOT EXISTS (
      SELECT 1 FROM information_schema.columns
      WHERE table_schema = DATABASE() AND table_name = 'solution_comments'
        AND column_name = 'reply_to_user_id'
  ) THEN
    ALTER TABLE `solution_comments`
      ADD COLUMN `reply_to_user_id` INT UNSIGNED NULL AFTER `parent_id`;
  END IF;

  IF EXISTS (
      SELECT 1 FROM information_schema.tables
      WHERE table_schema = DATABASE() AND table_name = 'solution_comments'
  ) AND NOT EXISTS (
      SELECT 1 FROM information_schema.columns
      WHERE table_schema = DATABASE() AND table_name = 'solution_comments'
        AND column_name = 'like_count'
  ) THEN
    ALTER TABLE `solution_comments`
      ADD COLUMN `like_count` INT UNSIGNED NOT NULL DEFAULT 0 AFTER `reply_to_user_id`;
  END IF;

  IF EXISTS (
      SELECT 1 FROM information_schema.tables
      WHERE table_schema = DATABASE() AND table_name = 'solution_comments'
  ) AND NOT EXISTS (
      SELECT 1 FROM information_schema.columns
      WHERE table_schema = DATABASE() AND table_name = 'solution_comments'
        AND column_name = 'favorite_count'
  ) THEN
    ALTER TABLE `solution_comments`
      ADD COLUMN `favorite_count` INT UNSIGNED NOT NULL DEFAULT 0 AFTER `like_count`;
  END IF;

  IF EXISTS (
      SELECT 1 FROM information_schema.tables
      WHERE table_schema = DATABASE() AND table_name = 'solution_comments'
  ) AND NOT EXISTS (
      SELECT 1 FROM information_schema.statistics
      WHERE table_schema = DATABASE() AND table_name = 'solution_comments'
        AND index_name = 'idx_parent_id'
  ) THEN
    ALTER TABLE `solution_comments` ADD KEY `idx_parent_id` (`parent_id`);
  END IF;

  IF EXISTS (
      SELECT 1 FROM information_schema.tables
      WHERE table_schema = DATABASE() AND table_name = 'solution_comments'
  ) AND NOT EXISTS (
      SELECT 1 FROM information_schema.statistics
      WHERE table_schema = DATABASE() AND table_name = 'solution_comments'
        AND index_name = 'idx_reply_user'
  ) THEN
    ALTER TABLE `solution_comments` ADD KEY `idx_reply_user` (`reply_to_user_id`);
  END IF;

  IF EXISTS (
      SELECT 1 FROM information_schema.tables
      WHERE table_schema = DATABASE() AND table_name = 'solution_comments'
  ) AND NOT EXISTS (
      SELECT 1 FROM information_schema.statistics
      WHERE table_schema = DATABASE() AND table_name = 'solution_comments'
        AND index_name = 'idx_comment_toplevel'
  ) THEN
    ALTER TABLE `solution_comments`
      ADD KEY `idx_comment_toplevel` (`solution_id`, `parent_id`, `created_at`);
  END IF;

  IF EXISTS (
      SELECT 1 FROM information_schema.tables
      WHERE table_schema = DATABASE() AND table_name = 'solution_comments'
  ) AND NOT EXISTS (
      SELECT 1 FROM information_schema.statistics
      WHERE table_schema = DATABASE() AND table_name = 'solution_comments'
        AND index_name = 'idx_comment_replies'
  ) THEN
    ALTER TABLE `solution_comments`
      ADD KEY `idx_comment_replies` (`parent_id`, `created_at`);
  END IF;
END//
CALL migrate_20260715_03_comment_columns()//
DROP PROCEDURE migrate_20260715_03_comment_columns//
DELIMITER ;

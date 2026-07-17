-- Multi-instance outbox claims and publisher-confirm recovery.
SET NAMES utf8mb4;

DELIMITER //
DROP PROCEDURE IF EXISTS migrate_20260717_02_judge_outbox_leases//
CREATE PROCEDURE migrate_20260717_02_judge_outbox_leases()
BEGIN
  IF NOT EXISTS (
      SELECT 1 FROM information_schema.columns
      WHERE table_schema = DATABASE() AND table_name = 'judge_outbox' AND column_name = 'lease_owner'
  ) THEN
    ALTER TABLE `judge_outbox`
      ADD COLUMN `lease_owner` VARCHAR(64) CHARACTER SET ascii COLLATE ascii_bin NULL AFTER `next_retry_at`;
  END IF;

  IF NOT EXISTS (
      SELECT 1 FROM information_schema.columns
      WHERE table_schema = DATABASE() AND table_name = 'judge_outbox' AND column_name = 'lease_expires_at'
  ) THEN
    ALTER TABLE `judge_outbox`
      ADD COLUMN `lease_expires_at` DATETIME NULL AFTER `lease_owner`;
  END IF;

  IF NOT EXISTS (
      SELECT 1 FROM information_schema.columns
      WHERE table_schema = DATABASE() AND table_name = 'judge_outbox' AND column_name = 'confirm_deadline'
  ) THEN
    ALTER TABLE `judge_outbox`
      ADD COLUMN `confirm_deadline` DATETIME NULL AFTER `lease_expires_at`;
  END IF;

  IF NOT EXISTS (
      SELECT 1 FROM information_schema.statistics
      WHERE table_schema = DATABASE() AND table_name = 'judge_outbox'
        AND index_name = 'idx_judge_outbox_claim'
  ) THEN
    ALTER TABLE `judge_outbox`
      ADD KEY `idx_judge_outbox_claim`
        (`status`, `next_retry_at`, `lease_expires_at`, `created_at`, `message_id`);
  END IF;
END//
CALL migrate_20260717_02_judge_outbox_leases()//
DROP PROCEDURE migrate_20260717_02_judge_outbox_leases//
DELIMITER ;

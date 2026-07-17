DELIMITER //
DROP PROCEDURE IF EXISTS migrate_20260717_01_question_visibility//
CREATE PROCEDURE migrate_20260717_01_question_visibility()
BEGIN
  IF NOT EXISTS (
      SELECT 1 FROM information_schema.columns
      WHERE table_schema = DATABASE() AND table_name = 'questions' AND column_name = 'visible'
  ) THEN
    ALTER TABLE `questions`
      ADD COLUMN `visible` TINYINT(1) NOT NULL DEFAULT 1
      COMMENT 'Whether the question is visible to regular users'
      AFTER `memory_limit`;
  END IF;
END//
CALL migrate_20260717_01_question_visibility()//
DROP PROCEDURE migrate_20260717_01_question_visibility//
DELIMITER ;

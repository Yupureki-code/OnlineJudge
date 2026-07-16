-- Enforce one state row per solution interaction.
SET NAMES utf8mb4;

DELIMITER //
DROP PROCEDURE IF EXISTS migrate_20260715_04_interaction_constraints//
CREATE PROCEDURE migrate_20260715_04_interaction_constraints()
BEGIN
  IF EXISTS (
      SELECT 1 FROM information_schema.tables
      WHERE table_schema = DATABASE() AND table_name = 'solution_actions'
  ) AND NOT EXISTS (
      SELECT 1 FROM information_schema.statistics
      WHERE table_schema = DATABASE() AND table_name = 'solution_actions'
        AND index_name = 'uk_solution_user_action'
  ) THEN
    DELETE duplicate_action
    FROM `solution_actions` duplicate_action
    INNER JOIN `solution_actions` retained_action
      ON duplicate_action.solution_id = retained_action.solution_id
     AND duplicate_action.user_id = retained_action.user_id
     AND duplicate_action.action_type = retained_action.action_type
     AND duplicate_action.id > retained_action.id;

    UPDATE `solutions` solution
    LEFT JOIN (
      SELECT solution_id,
             SUM(action_type = 'like') AS like_count,
             SUM(action_type = 'favorite') AS favorite_count
      FROM `solution_actions`
      GROUP BY solution_id
    ) counts ON counts.solution_id = solution.id
    SET solution.like_count = COALESCE(counts.like_count, 0),
        solution.favorite_count = COALESCE(counts.favorite_count, 0);

    ALTER TABLE `solution_actions`
      ADD UNIQUE KEY `uk_solution_user_action` (`solution_id`, `user_id`, `action_type`);
  END IF;

  IF EXISTS (
      SELECT 1 FROM information_schema.tables
      WHERE table_schema = DATABASE() AND table_name = 'comment_actions'
  ) AND NOT EXISTS (
      SELECT 1 FROM information_schema.statistics
      WHERE table_schema = DATABASE() AND table_name = 'comment_actions'
        AND index_name = 'uk_comment_user_action'
  ) THEN
    DELETE duplicate_action
    FROM `comment_actions` duplicate_action
    INNER JOIN `comment_actions` retained_action
      ON duplicate_action.comment_id = retained_action.comment_id
     AND duplicate_action.user_id = retained_action.user_id
     AND duplicate_action.action_type = retained_action.action_type
     AND duplicate_action.id > retained_action.id;

    UPDATE `solution_comments` comment
    LEFT JOIN (
      SELECT comment_id,
             SUM(action_type = 'like') AS like_count,
             SUM(action_type = 'favorite') AS favorite_count
      FROM `comment_actions`
      GROUP BY comment_id
    ) counts ON counts.comment_id = comment.id
    SET comment.like_count = COALESCE(counts.like_count, 0),
        comment.favorite_count = COALESCE(counts.favorite_count, 0);

    ALTER TABLE `comment_actions`
      ADD UNIQUE KEY `uk_comment_user_action` (`comment_id`, `user_id`, `action_type`);
  END IF;
END//
CALL migrate_20260715_04_interaction_constraints()//
DROP PROCEDURE migrate_20260715_04_interaction_constraints//
DELIMITER ;

-- sql/judge_tasks.sql
-- 判题任务追踪表 — 新增用于 MQ 异步判题
--
-- 用途：Business Service 接收提交后写入此表，
--       Judge Service 消费 MQ 消息后更新状态，
--       前端通过轮询此表获取判题进度

DROP TABLE IF EXISTS `judge_tasks`;

CREATE TABLE `judge_tasks` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `submission_id` BIGINT UNSIGNED NOT NULL COMMENT '提交记录 ID（关联 user_submits）',
  `question_id` VARCHAR(5) NOT NULL COMMENT '题目 ID',
  `user_id` INT UNSIGNED NOT NULL COMMENT '用户 ID',
  `status` ENUM('QUEUED','JUDGING','COMPLETED','FAILED') NOT NULL DEFAULT 'QUEUED'
    COMMENT '判题状态：QUEUED=排队中, JUDGING=正在判题, COMPLETED=判题完成, FAILED=判题失败',
  `worker_id` VARCHAR(64) NULL COMMENT '判题 Worker 标识',
  `queued_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '入队时间',
  `started_at` DATETIME NULL COMMENT '开始判题时间',
  `completed_at` DATETIME NULL COMMENT '完成时间',
  `result_json` TEXT NULL COMMENT '判题结果 JSON',
  `error_message` TEXT NULL COMMENT '错误信息',
  PRIMARY KEY (`id`) USING BTREE,
  INDEX `idx_submission` (`submission_id`) USING BTREE,
  INDEX `idx_user_question` (`user_id`, `question_id`) USING BTREE,
  INDEX `idx_status` (`status`) USING BTREE,
  INDEX `idx_queued` (`status`, `queued_at`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
  COMMENT='判题任务追踪表（MQ 异步判题）';

-- OnlineJudge isolated performance data seed for MySQL 8.0+.
--
-- Run only against the disposable performance database after all migrations:
--   mysql --default-character-set=utf8mb4 -u oj_server -p myoj < mysql_seed.sql
--
-- Re-running this file removes only rows identified by the PERF prefixes below.
-- All generated user and admin accounts share @perf_password. Set
-- @perf_password_pepper to exactly OJ_PASSWORD_PEPPER used by oj_server.

SET NAMES utf8mb4;
SET SESSION sql_safe_updates = 0;

SET @perf_password = 'PerfTest123';
SET @perf_password_pepper = '';
SET @perf_visible_questions = 10000;
SET @perf_hidden_questions = 500;
SET @perf_users = 6000;
SET @perf_admins = 64;
SET @perf_solutions = 50000;
SET @perf_top_comments = 150000;
SET @perf_replies = 50000;
SET @perf_submissions = 500000;
SET @perf_audit_logs = 20000;
SET @perf_max_rows = GREATEST(
  @perf_visible_questions + @perf_hidden_questions,
  @perf_users,
  @perf_solutions,
  @perf_top_comments,
  @perf_replies,
  @perf_submissions,
  @perf_audit_logs
);

DELIMITER //
DROP PROCEDURE IF EXISTS perf_seed_validate//
CREATE PROCEDURE perf_seed_validate()
BEGIN
  IF DATABASE() IS NULL THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'Select the isolated performance database before running this script';
  END IF;
  IF CAST(SUBSTRING_INDEX(VERSION(), '.', 1) AS UNSIGNED) < 8 THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'mysql_seed.sql requires MySQL 8.0+';
  END IF;
  IF @perf_max_rows > 1000000 OR @perf_users < @perf_admins OR
     @perf_visible_questions < 1 OR @perf_solutions < 1 OR @perf_top_comments < 1 THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'Invalid PERF row counts';
  END IF;
  IF LENGTH(@perf_password) < 8 OR @perf_password NOT REGEXP '[[:alpha:]]' OR
     @perf_password NOT REGEXP '[[:digit:]]' THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'PERF password must contain letters and digits and be at least 8 characters';
  END IF;
  IF (SELECT COUNT(*) FROM information_schema.tables
      WHERE table_schema = DATABASE() AND table_name IN (
        'users', 'questions', 'tests', 'solutions', 'solution_comments',
        'solution_actions', 'comment_actions', 'submissions', 'user_submits',
        'admin_accounts', 'admin_audit_logs', 'judge_outbox', 'judge_result_receipts'
      )) <> 13 THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'Required schema or migrations are missing';
  END IF;
  IF NOT EXISTS (
    SELECT 1 FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'tests' AND column_name = 'test_case_id'
  ) OR NOT EXISTS (
    SELECT 1 FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'admin_audit_logs' AND column_name = 'operator_uid'
  ) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'Apply current migrations before seeding performance data';
  END IF;
END//
CALL perf_seed_validate()//
DROP PROCEDURE perf_seed_validate//

DROP PROCEDURE IF EXISTS perf_seed_build_password//
CREATE PROCEDURE perf_seed_build_password()
BEGIN
  DECLARE iteration INT DEFAULT 1;
  DECLARE salt_value CHAR(32) DEFAULT '0123456789abcdef0123456789abcdef';
  DECLARE digest_value CHAR(64);

  SET digest_value = SHA2(CONCAT(salt_value, ':', @perf_password, ':', @perf_password_pepper), 256);
  WHILE iteration < 120000 DO
    SET digest_value = SHA2(CONCAT(digest_value, ':', salt_value, ':', @perf_password_pepper), 256);
    SET iteration = iteration + 1;
  END WHILE;
  SET @perf_password_hash = CONCAT(salt_value, '$120000$', digest_value);
END//
CALL perf_seed_build_password()//
DROP PROCEDURE perf_seed_build_password//
DELIMITER ;

DROP TEMPORARY TABLE IF EXISTS perf_seq;
CREATE TEMPORARY TABLE perf_seq (n INT UNSIGNED NOT NULL PRIMARY KEY) ENGINE=InnoDB;
INSERT INTO perf_seq (n)
WITH digits AS (
  SELECT 0 AS n UNION ALL SELECT 1 UNION ALL SELECT 2 UNION ALL SELECT 3 UNION ALL SELECT 4
  UNION ALL SELECT 5 UNION ALL SELECT 6 UNION ALL SELECT 7 UNION ALL SELECT 8 UNION ALL SELECT 9
)
SELECT 1 + a.n + 10*b.n + 100*c.n + 1000*d.n + 10000*e.n + 100000*f.n
FROM digits a
CROSS JOIN digits b
CROSS JOIN digits c
CROSS JOIN digits d
CROSS JOIN digits e
CROSS JOIN digits f
WHERE a.n + 10*b.n + 100*c.n + 1000*d.n + 10000*e.n + 100000*f.n < @perf_max_rows;

-- Remove a previous PERF seed. Child and interaction rows are removed first.
DELETE jr FROM judge_result_receipts jr
JOIN submissions s ON s.submission_id = jr.submission_id
LEFT JOIN users u ON u.uid = s.user_id
WHERE s.question_id LIKE 'P%' OR u.name LIKE 'perf_u%';

DELETE jo FROM judge_outbox jo
JOIN submissions s ON s.submission_id = jo.submission_id
LEFT JOIN users u ON u.uid = s.user_id
WHERE s.question_id LIKE 'P%' OR u.name LIKE 'perf_u%';

DELETE ca FROM comment_actions ca
LEFT JOIN solution_comments c ON c.id = ca.comment_id
LEFT JOIN solutions s ON s.id = c.solution_id
LEFT JOIN users u ON u.uid = ca.user_id
WHERE s.question_id LIKE 'P%' OR u.name LIKE 'perf_u%';

DELETE sa FROM solution_actions sa
LEFT JOIN solutions s ON s.id = sa.solution_id
LEFT JOIN users u ON u.uid = sa.user_id
WHERE s.question_id LIKE 'P%' OR u.name LIKE 'perf_u%';

DELETE c FROM solution_comments c
JOIN solutions s ON s.id = c.solution_id
WHERE s.question_id LIKE 'P%';

DELETE al FROM admin_audit_logs al
LEFT JOIN admin_accounts aa ON aa.admin_id = al.operator_admin_id
LEFT JOIN users u ON u.uid = aa.uid
WHERE al.request_id LIKE 'PERF-%' OR u.name LIKE 'perf_u%';

DELETE aa FROM admin_accounts aa
JOIN users u ON u.uid = aa.uid
WHERE u.name LIKE 'perf_u%';

DELETE FROM submissions
WHERE question_id LIKE 'P%'
   OR user_id IN (SELECT uid FROM users WHERE name LIKE 'perf_u%');

DELETE FROM user_submits
WHERE question_id LIKE 'P%'
   OR user_id IN (SELECT uid FROM users WHERE name LIKE 'perf_u%');

DELETE FROM solutions
WHERE question_id LIKE 'P%'
   OR user_id IN (SELECT uid FROM users WHERE name LIKE 'perf_u%');

DELETE FROM tests WHERE question_id LIKE 'P%';
DELETE FROM questions WHERE id LIKE 'P%';
DELETE FROM users WHERE name LIKE 'perf_u%' OR email LIKE 'perf_u%@perf.invalid';
COMMIT;

-- Stable account pool. The numeric suffix is retained in perf_user_map.
INSERT INTO users (
  name, password_hash, email, create_time, last_login, password_algo, password_update_at
)
SELECT
  CONCAT('perf_u', LPAD(n, 6, '0')),
  @perf_password_hash,
  CONCAT('perf_u', LPAD(n, 6, '0'), '@perf.invalid'),
  TIMESTAMP('2025-01-01 00:00:00') + INTERVAL MOD(n, 365) DAY,
  TIMESTAMP('2026-01-01 00:00:00') + INTERVAL MOD(n, 180) DAY,
  'sha256_iter_v1',
  TIMESTAMP('2025-01-01 00:00:00') + INTERVAL MOD(n, 365) DAY
FROM perf_seq WHERE n <= @perf_users;
COMMIT;

DROP TEMPORARY TABLE IF EXISTS perf_user_map;
CREATE TEMPORARY TABLE perf_user_map AS
SELECT CAST(SUBSTRING(name, 7) AS UNSIGNED) AS seq, uid
FROM users WHERE name LIKE 'perf_u%';
ALTER TABLE perf_user_map ADD PRIMARY KEY (seq), ADD UNIQUE KEY (uid);

-- P + four base36 digits fits the production VARCHAR(5) question key.
INSERT INTO questions (
  id, title, `desc`, star, create_time, update_time, cpu_limit, memory_limit, visible
)
SELECT
  CONCAT('P', LPAD(CONV(n - 1, 10, 36), 4, '0')),
  CONCAT('PERF addition problem ', LPAD(n, 5, '0')),
  CONCAT('# Performance fixture\n\nPrint `hello world`. Deterministic fixture ', n, '.'),
  ELT(1 + MOD(n - 1, 5), '1', '2', '3', '4', '5'),
  TIMESTAMP('2025-01-01 00:00:00') + INTERVAL MOD(n, 365) DAY,
  TIMESTAMP('2026-01-01 00:00:00') + INTERVAL MOD(n, 180) DAY,
  1 + MOD(n, 3),
  128 + 64 * MOD(n, 4),
  IF(n <= @perf_visible_questions, 1, 0)
FROM perf_seq
WHERE n <= @perf_visible_questions + @perf_hidden_questions;

INSERT INTO tests (id, question_id, `in`, `out`, is_sample)
SELECT 1, CONCAT('P', LPAD(CONV(n - 1, 10, 36), 4, '0')), 'perf\n', 'hello world\n', 1
FROM perf_seq WHERE n <= @perf_visible_questions + @perf_hidden_questions;

INSERT INTO tests (id, question_id, `in`, `out`, is_sample)
SELECT 2, CONCAT('P', LPAD(CONV(n - 1, 10, 36), 4, '0')), 'load\n', 'hello world\n', 0
FROM perf_seq WHERE n <= @perf_visible_questions + @perf_hidden_questions;
COMMIT;

INSERT INTO admin_accounts (uid, password_hash, role, created_at)
SELECT u.uid, @perf_password_hash, 'admin', TIMESTAMP('2025-06-01 00:00:00') + INTERVAL u.seq DAY
FROM perf_user_map u WHERE u.seq <= @perf_admins;

DROP TEMPORARY TABLE IF EXISTS perf_admin_map;
CREATE TEMPORARY TABLE perf_admin_map AS
SELECT ROW_NUMBER() OVER (ORDER BY aa.admin_id) AS seq, aa.admin_id, aa.uid
FROM admin_accounts aa
JOIN users u ON u.uid = aa.uid
WHERE u.name LIKE 'perf_u%';
ALTER TABLE perf_admin_map ADD PRIMARY KEY (seq), ADD UNIQUE KEY (admin_id);

INSERT INTO admin_audit_logs (
  request_id, operator_admin_id, operator_uid, operator_role,
  action, resource_type, result, created_at, payload_text
)
SELECT
  CONCAT('PERF-', LPAD(s.n, 12, '0')),
  a.admin_id,
  a.uid,
  'admin',
  ELT(1 + MOD(s.n, 4), 'list_users', 'get_question', 'list_accounts', 'cache_metrics'),
  ELT(1 + MOD(s.n, 3), 'user', 'question', 'admin_account'),
  IF(MOD(s.n, 50) = 0, 'denied', 'success'),
  TIMESTAMP('2026-01-01 00:00:00') + INTERVAL MOD(s.n, 180) DAY + INTERVAL MOD(s.n, 86400) SECOND,
  CONCAT('{"perf":true,"sequence":', s.n, '}')
FROM perf_seq s
JOIN perf_admin_map a ON a.seq = 1 + MOD(s.n - 1, @perf_admins)
WHERE s.n <= @perf_audit_logs;
COMMIT;

INSERT INTO solutions (
  question_id, user_id, title, content_md, like_count, favorite_count,
  comment_count, status, created_at, updated_at
)
SELECT
  CONCAT('P', LPAD(CONV(MOD(s.n - 1, @perf_visible_questions), 10, 36), 4, '0')),
  u.uid,
  CONCAT('PERF solution ', LPAD(s.n, 6, '0')),
  CONCAT('## Deterministic solution\n\nThis is performance fixture ', s.n,
         '.\n\n```cpp\n#include <iostream>\nint main(){std::cout << "hello world" << std::endl;}\n```'),
  1, 1, 4, 'approved',
  TIMESTAMP('2025-01-01 00:00:00') + INTERVAL MOD(s.n, 365) DAY + INTERVAL MOD(s.n, 86400) SECOND,
  TIMESTAMP('2026-01-01 00:00:00') + INTERVAL MOD(s.n, 180) DAY + INTERVAL MOD(s.n, 86400) SECOND
FROM perf_seq s
JOIN perf_user_map u ON u.seq = 1 + MOD(s.n - 1, @perf_users)
WHERE s.n <= @perf_solutions;
COMMIT;

DROP TEMPORARY TABLE IF EXISTS perf_solution_map;
CREATE TEMPORARY TABLE perf_solution_map AS
SELECT CAST(SUBSTRING(title, 15) AS UNSIGNED) AS seq, id AS solution_id
FROM solutions WHERE title LIKE 'PERF solution %';
ALTER TABLE perf_solution_map ADD PRIMARY KEY (seq), ADD UNIQUE KEY (solution_id);

-- One seeded like and favorite per solution exercises both action lookup states.
INSERT INTO solution_actions (solution_id, user_id, action_type, created_at)
SELECT sm.solution_id, u.uid, 'like', TIMESTAMP('2026-01-01 00:00:00') + INTERVAL MOD(sm.seq, 180) DAY
FROM perf_solution_map sm
JOIN perf_user_map u ON u.seq = 1 + MOD(sm.seq, @perf_users);

INSERT INTO solution_actions (solution_id, user_id, action_type, created_at)
SELECT sm.solution_id, u.uid, 'favorite', TIMESTAMP('2026-01-01 00:00:00') + INTERVAL MOD(sm.seq, 180) DAY
FROM perf_solution_map sm
JOIN perf_user_map u ON u.seq = 1 + MOD(sm.seq + 1, @perf_users);
COMMIT;

INSERT INTO solution_comments (
  parent_id, reply_to_user_id, like_count, favorite_count,
  solution_id, user_id, content, is_edited, created_at, updated_at
)
SELECT
  NULL, NULL, IF(MOD(s.n, 4) = 0, 1, 0), IF(MOD(s.n, 10) = 0, 1, 0),
  sm.solution_id, u.uid,
  CONCAT('PERF top comment ', LPAD(s.n, 6, '0'), ': deterministic discussion text.'),
  IF(MOD(s.n, 20) = 0, 1, 0),
  TIMESTAMP('2026-01-01 00:00:00') + INTERVAL MOD(s.n, 180) DAY + INTERVAL MOD(s.n, 86400) SECOND,
  TIMESTAMP('2026-01-01 00:00:00') + INTERVAL MOD(s.n, 180) DAY + INTERVAL MOD(s.n, 86400) SECOND
FROM perf_seq s
JOIN perf_solution_map sm ON sm.seq = 1 + MOD(s.n - 1, @perf_solutions)
JOIN perf_user_map u ON u.seq = 1 + MOD(s.n + 2, @perf_users)
WHERE s.n <= @perf_top_comments;
COMMIT;

DROP TEMPORARY TABLE IF EXISTS perf_top_comment_map;
CREATE TEMPORARY TABLE perf_top_comment_map AS
SELECT ROW_NUMBER() OVER (ORDER BY c.id) AS seq,
       c.id AS comment_id, c.solution_id, c.user_id
FROM solution_comments c
JOIN solutions s ON s.id = c.solution_id
WHERE s.question_id LIKE 'P%' AND c.parent_id IS NULL;
ALTER TABLE perf_top_comment_map ADD PRIMARY KEY (seq), ADD UNIQUE KEY (comment_id);

INSERT INTO solution_comments (
  parent_id, reply_to_user_id, like_count, favorite_count,
  solution_id, user_id, content, is_edited, created_at, updated_at
)
SELECT
  p.comment_id, p.user_id, 0, 0, p.solution_id, u.uid,
  CONCAT('PERF reply ', LPAD(s.n, 6, '0'), ': deterministic reply text.'),
  0,
  TIMESTAMP('2026-06-01 00:00:00') + INTERVAL MOD(s.n, 45) DAY + INTERVAL MOD(s.n, 86400) SECOND,
  TIMESTAMP('2026-06-01 00:00:00') + INTERVAL MOD(s.n, 45) DAY + INTERVAL MOD(s.n, 86400) SECOND
FROM perf_seq s
JOIN perf_top_comment_map p ON p.seq = 1 + MOD(s.n - 1, @perf_top_comments)
JOIN perf_user_map u ON u.seq = 1 + MOD(s.n + 3, @perf_users)
WHERE s.n <= @perf_replies;
COMMIT;

DROP TEMPORARY TABLE IF EXISTS perf_comment_map;
CREATE TEMPORARY TABLE perf_comment_map AS
SELECT ROW_NUMBER() OVER (ORDER BY c.id) AS seq, c.id AS comment_id,
       c.like_count, c.favorite_count
FROM solution_comments c
JOIN solutions s ON s.id = c.solution_id
WHERE s.question_id LIKE 'P%';
ALTER TABLE perf_comment_map ADD PRIMARY KEY (seq), ADD UNIQUE KEY (comment_id);

-- Seed actions for 25% of comments; counters above match these rows.
INSERT INTO comment_actions (comment_id, user_id, action_type, created_at)
SELECT cm.comment_id, u.uid, 'like', TIMESTAMP('2026-06-01 00:00:00') + INTERVAL MOD(cm.seq, 45) DAY
FROM perf_comment_map cm
JOIN perf_user_map u ON u.seq = 1 + MOD(cm.seq + 4, @perf_users)
WHERE cm.like_count = 1;

INSERT INTO comment_actions (comment_id, user_id, action_type, created_at)
SELECT cm.comment_id, u.uid, 'favorite', TIMESTAMP('2026-06-01 00:00:00') + INTERVAL MOD(cm.seq, 45) DAY
FROM perf_comment_map cm
JOIN perf_user_map u ON u.seq = 1 + MOD(cm.seq + 5, @perf_users)
WHERE cm.favorite_count = 1;
COMMIT;

INSERT INTO submissions (
  user_id, question_id, code, language, status, result_json, is_pass,
  time_used_ms, memory_used_bytes, compile_error, created_at, completed_at, result_version
)
SELECT
  u.uid,
  CONCAT('P', LPAD(CONV(MOD(s.n - 1, @perf_visible_questions), 10, 36), 4, '0')),
  '#include <iostream>\nint main(){std::cout << "hello world" << std::endl;}',
  'cpp',
  CASE MOD(s.n, 20)
    WHEN 0 THEN 'COMPILE_ERROR'
    WHEN 1 THEN 'WRONG_ANSWER'
    WHEN 2 THEN 'TIME_LIMIT_EXCEEDED'
    ELSE 'ACCEPTED'
  END,
  CASE MOD(s.n, 20)
    WHEN 0 THEN '{"status":"COMPILE_ERROR","score":0}'
    WHEN 1 THEN '{"status":"WRONG_ANSWER","score":0}'
    WHEN 2 THEN '{"status":"TIME_LIMIT_EXCEEDED","score":0}'
    ELSE '{"status":"ACCEPTED","score":100}'
  END,
  IF(MOD(s.n, 20) >= 3, 1, 0),
  IF(MOD(s.n, 20) = 2, 1000, 10 + MOD(s.n, 40)),
  8388608 + 4096 * MOD(s.n, 1024),
  IF(MOD(s.n, 20) = 0, 'PERF fixture compile error', NULL),
  TIMESTAMP('2025-01-01 00:00:00') + INTERVAL MOD(s.n, 545) DAY + INTERVAL MOD(s.n, 86400) SECOND,
  TIMESTAMP('2025-01-01 00:00:01') + INTERVAL MOD(s.n, 545) DAY + INTERVAL MOD(s.n, 86400) SECOND,
  1
FROM perf_seq s
JOIN perf_user_map u ON u.seq = 1 + MOD(s.n - 1, @perf_users)
WHERE s.n <= @perf_submissions;
COMMIT;

ANALYZE TABLE users, questions, tests, solutions, solution_comments,
  solution_actions, comment_actions, submissions, admin_accounts, admin_audit_logs;

-- Verification: every count must equal the configured value.
SELECT 'users' AS fixture, COUNT(*) AS actual, @perf_users AS expected
FROM users WHERE name LIKE 'perf_u%'
UNION ALL SELECT 'visible_questions', COUNT(*), @perf_visible_questions
FROM questions WHERE id LIKE 'P%' AND visible = 1
UNION ALL SELECT 'hidden_questions', COUNT(*), @perf_hidden_questions
FROM questions WHERE id LIKE 'P%' AND visible = 0
UNION ALL SELECT 'test_cases', COUNT(*), 2 * (@perf_visible_questions + @perf_hidden_questions)
FROM tests WHERE question_id LIKE 'P%'
UNION ALL SELECT 'admins', COUNT(*), @perf_admins
FROM admin_accounts aa JOIN users u ON u.uid = aa.uid WHERE u.name LIKE 'perf_u%'
UNION ALL SELECT 'solutions', COUNT(*), @perf_solutions
FROM solutions WHERE question_id LIKE 'P%'
UNION ALL SELECT 'comments_and_replies', COUNT(*), @perf_top_comments + @perf_replies
FROM solution_comments c JOIN solutions s ON s.id = c.solution_id WHERE s.question_id LIKE 'P%'
UNION ALL SELECT 'submissions', COUNT(*), @perf_submissions
FROM submissions WHERE question_id LIKE 'P%'
UNION ALL SELECT 'audit_logs', COUNT(*), @perf_audit_logs
FROM admin_audit_logs WHERE request_id LIKE 'PERF-%';

-- Fixture candidates. Login through OpenResty using these accounts and @perf_password.
SELECT 'LOGIN_CREDENTIALS' AS fixture_type,
       @perf_password AS shared_password,
       'Set @perf_password_pepper to OJ_PASSWORD_PEPPER before execution' AS note;

SELECT 'USER' AS fixture_type, u.uid AS id, u.name AS login_name, u.email
FROM users u WHERE u.name LIKE 'perf_u%' ORDER BY u.uid LIMIT 20;

SELECT 'ADMIN' AS fixture_type, aa.admin_id AS id, u.name AS linked_user, u.email
FROM admin_accounts aa JOIN users u ON u.uid = aa.uid
WHERE u.name LIKE 'perf_u%' ORDER BY aa.admin_id;

SELECT 'QUESTION' AS fixture_type, id, visible
FROM questions WHERE id LIKE 'P%' ORDER BY id LIMIT 20;

SELECT 'SOLUTION' AS fixture_type, s.id, s.question_id, u.name AS owner
FROM solutions s JOIN users u ON u.uid = s.user_id
WHERE s.question_id LIKE 'P%' ORDER BY s.id LIMIT 20;

SELECT 'TOP_COMMENT' AS fixture_type, c.id, c.solution_id, u.name AS owner
FROM solution_comments c
JOIN solutions s ON s.id = c.solution_id
JOIN users u ON u.uid = c.user_id
WHERE s.question_id LIKE 'P%' AND c.parent_id IS NULL ORDER BY c.id LIMIT 20;

SELECT 'SUBMISSION' AS fixture_type, s.submission_id AS id, s.status, u.name AS owner
FROM submissions s JOIN users u ON u.uid = s.user_id
WHERE s.question_id LIKE 'P%' AND s.status = 'ACCEPTED'
ORDER BY s.submission_id DESC LIMIT 20;

DROP TEMPORARY TABLE IF EXISTS perf_comment_map;
DROP TEMPORARY TABLE IF EXISTS perf_top_comment_map;
DROP TEMPORARY TABLE IF EXISTS perf_solution_map;
DROP TEMPORARY TABLE IF EXISTS perf_admin_map;
DROP TEMPORARY TABLE IF EXISTS perf_user_map;
DROP TEMPORARY TABLE IF EXISTS perf_seq;

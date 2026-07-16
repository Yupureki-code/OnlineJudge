SET @legacy_submit_is_bit := (
  SELECT COUNT(*)
  FROM information_schema.columns
  WHERE table_schema = DATABASE()
    AND table_name = 'user_submits'
    AND column_name = 'is_pass'
    AND data_type = 'bit'
);

SET @legacy_submit_is_bit_sql := IF(
  @legacy_submit_is_bit > 0,
  'ALTER TABLE `user_submits` MODIFY COLUMN `is_pass` TINYINT(1) NOT NULL COMMENT ''是否通过(1通过/0未通过)''',
  'SELECT 1'
);

PREPARE legacy_submit_is_bit_stmt FROM @legacy_submit_is_bit_sql;
EXECUTE legacy_submit_is_bit_stmt;
DEALLOCATE PREPARE legacy_submit_is_bit_stmt;

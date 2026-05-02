/*
 Navicat Premium Dump SQL

 Source Server         : oj_server
 Source Server Type    : MySQL
 Source Server Version : 80045 (8.0.45-0ubuntu0.24.04.1)
 Source Host           : 110.42.134.10:3306
 Source Schema         : myoj

 Target Server Type    : MySQL
 Target Server Version : 80045 (8.0.45-0ubuntu0.24.04.1)
 File Encoding         : 65001

 Date: 02/05/2026 21:55:32
*/

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- ----------------------------
-- Table structure for tests
-- ----------------------------
DROP TABLE IF EXISTS `tests`;
CREATE TABLE `tests`  (
  `id` tinyint NOT NULL AUTO_INCREMENT,
  `question_id` char(5) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `in` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `out` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `is_sample` tinyint(1) NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`, `question_id`) USING BTREE,
  INDEX `question_id`(`question_id` ASC) USING BTREE,
  CONSTRAINT `tests_ibfk_1` FOREIGN KEY (`question_id`) REFERENCES `questions` (`id`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE = InnoDB AUTO_INCREMENT = 6 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_unicode_ci COMMENT = '题目的测试用例' ROW_FORMAT = Dynamic;

SET FOREIGN_KEY_CHECKS = 1;

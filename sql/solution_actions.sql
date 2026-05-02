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

 Date: 02/05/2026 21:55:15
*/

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- ----------------------------
-- Table structure for solution_actions
-- ----------------------------
DROP TABLE IF EXISTS `solution_actions`;
CREATE TABLE `solution_actions`  (
  `id` int UNSIGNED NOT NULL AUTO_INCREMENT,
  `solution_id` int(10) UNSIGNED ZEROFILL NOT NULL,
  `user_id` int UNSIGNED NOT NULL,
  `action_type` enum('like','comment','favorite') CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `created_at` datetime NOT NULL,
  PRIMARY KEY (`id`) USING BTREE,
  INDEX `solutionid`(`solution_id` ASC) USING BTREE,
  INDEX `userid`(`user_id` ASC) USING BTREE,
  INDEX `idx_solution_action_lookup`(`solution_id` ASC, `user_id` ASC, `action_type` ASC) USING BTREE,
  CONSTRAINT `solutionid` FOREIGN KEY (`solution_id`) REFERENCES `solutions` (`id`) ON DELETE RESTRICT ON UPDATE RESTRICT,
  CONSTRAINT `userid` FOREIGN KEY (`user_id`) REFERENCES `users` (`uid`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE = InnoDB AUTO_INCREMENT = 16 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_unicode_ci ROW_FORMAT = Dynamic;

SET FOREIGN_KEY_CHECKS = 1;

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

 Date: 02/05/2026 21:55:37
*/

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- ----------------------------
-- Table structure for user_submits
-- ----------------------------
DROP TABLE IF EXISTS `user_submits`;
CREATE TABLE `user_submits`  (
  `user_id` int UNSIGNED NOT NULL COMMENT '用户id',
  `question_id` varchar(5) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '题目Id',
  `result_json` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '判题结果json串',
  `is_pass` bit(1) NOT NULL COMMENT '是否通过(1通过/0未通过)',
  `action_time` datetime NOT NULL COMMENT '动作日期',
  INDEX `uid`(`user_id` ASC) USING BTREE,
  INDEX `qid`(`question_id` ASC) USING BTREE,
  INDEX `idx_user_submits_user_pass`(`user_id` ASC, `is_pass` ASC) USING BTREE,
  INDEX `idx_user_submits_user`(`user_id` ASC) USING BTREE,
  INDEX `idx_user_passed`(`user_id` ASC, `question_id` ASC, `is_pass` ASC) USING BTREE,
  INDEX `idx_user_submits_history`(`user_id` ASC, `action_time` DESC) USING BTREE,
  CONSTRAINT `qid` FOREIGN KEY (`question_id`) REFERENCES `questions` (`id`) ON DELETE RESTRICT ON UPDATE RESTRICT,
  CONSTRAINT `uid` FOREIGN KEY (`user_id`) REFERENCES `users` (`uid`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE = InnoDB CHARACTER SET = utf8mb4 COLLATE = utf8mb4_unicode_ci ROW_FORMAT = Dynamic;

SET FOREIGN_KEY_CHECKS = 1;

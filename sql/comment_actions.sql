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

 Date: 02/05/2026 21:55:02
*/

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- ----------------------------
-- Table structure for comment_actions
-- ----------------------------
DROP TABLE IF EXISTS `comment_actions`;
CREATE TABLE `comment_actions`  (
  `id` bigint UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '主键',
  `comment_id` int UNSIGNED NOT NULL COMMENT '评论ID',
  `user_id` int UNSIGNED NOT NULL COMMENT '用户ID',
  `action_type` enum('like','favorite') CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '操作类型：like=点赞，favorite=收藏',
  `created_at` datetime NOT NULL COMMENT '创建时间',
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE INDEX `uk_user_comment_action`(`user_id` ASC, `comment_id` ASC, `action_type` ASC) USING BTREE COMMENT '防止重复点赞/收藏',
  INDEX `idx_comment_id`(`comment_id` ASC) USING BTREE COMMENT '评论ID索引',
  INDEX `idx_user_action`(`user_id` ASC, `action_type` ASC) USING BTREE COMMENT '用户操作联合索引',
  INDEX `idx_user_comments`(`comment_id` ASC, `user_id` ASC) USING BTREE,
  CONSTRAINT `fk_comment_actions_comment` FOREIGN KEY (`comment_id`) REFERENCES `solution_comments` (`id`) ON DELETE CASCADE ON UPDATE RESTRICT,
  CONSTRAINT `fk_comment_actions_user` FOREIGN KEY (`user_id`) REFERENCES `users` (`uid`) ON DELETE CASCADE ON UPDATE RESTRICT
) ENGINE = InnoDB AUTO_INCREMENT = 12 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_unicode_ci COMMENT = '评论操作记录表（点赞、收藏）' ROW_FORMAT = Dynamic;

SET FOREIGN_KEY_CHECKS = 1;

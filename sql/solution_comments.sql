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

 Date: 02/05/2026 21:55:21
*/

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- ----------------------------
-- Table structure for solution_comments
-- ----------------------------
DROP TABLE IF EXISTS `solution_comments`;
CREATE TABLE `solution_comments`  (
  `id` int UNSIGNED NOT NULL AUTO_INCREMENT,
  `parent_id` bigint UNSIGNED NULL DEFAULT NULL COMMENT '父评论ID,NULL表示一级评论',
  `reply_to_user_id` int UNSIGNED NULL DEFAULT NULL COMMENT '被回复用户ID,用于@引用',
  `like_count` int UNSIGNED NOT NULL DEFAULT 0 COMMENT '点赞数',
  `favorite_count` int UNSIGNED NOT NULL DEFAULT 0 COMMENT '收藏数',
  `solution_id` int UNSIGNED NOT NULL,
  `user_id` int UNSIGNED NOT NULL,
  `content` text CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NOT NULL,
  `is_edited` tinyint(1) NULL DEFAULT 0,
  `created_at` datetime NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` datetime NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`) USING BTREE,
  INDEX `idx_solution_id`(`solution_id` ASC) USING BTREE,
  INDEX `idx_user_id`(`user_id` ASC) USING BTREE,
  INDEX `idx_parent_id`(`parent_id` ASC) USING BTREE,
  INDEX `idx_reply_user`(`reply_to_user_id` ASC) USING BTREE,
  INDEX `idx_comment_toplevel`(`solution_id` ASC, `parent_id` ASC, `created_at` ASC) USING BTREE,
  INDEX `idx_comment_replies`(`parent_id` ASC, `created_at` ASC) USING BTREE,
  CONSTRAINT `fk_comment_solution` FOREIGN KEY (`solution_id`) REFERENCES `solutions` (`id`) ON DELETE CASCADE ON UPDATE RESTRICT,
  CONSTRAINT `fk_comment_user` FOREIGN KEY (`user_id`) REFERENCES `users` (`uid`) ON DELETE CASCADE ON UPDATE RESTRICT
) ENGINE = InnoDB AUTO_INCREMENT = 78 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_0900_ai_ci ROW_FORMAT = Dynamic;

SET FOREIGN_KEY_CHECKS = 1;

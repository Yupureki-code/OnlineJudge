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

 Date: 02/05/2026 21:54:38
*/

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- ----------------------------
-- Table structure for admin_accounts
-- ----------------------------
DROP TABLE IF EXISTS `admin_accounts`;
CREATE TABLE `admin_accounts`  (
  `admin_id` int UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '管理员id',
  `uid` int UNSIGNED NOT NULL COMMENT '用户id',
  `password_hash` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '管理员密码',
  `role` enum('super_admin','admin') CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '管理员权限',
  `created_at` datetime NOT NULL COMMENT '管理员账户创建时间',
  PRIMARY KEY (`admin_id`) USING BTREE,
  UNIQUE INDEX `user_id`(`uid` ASC) USING BTREE,
  INDEX `role`(`role` ASC) USING BTREE,
  CONSTRAINT `foreign_uid` FOREIGN KEY (`uid`) REFERENCES `users` (`uid`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE = InnoDB AUTO_INCREMENT = 2 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_unicode_ci ROW_FORMAT = Dynamic;

SET FOREIGN_KEY_CHECKS = 1;

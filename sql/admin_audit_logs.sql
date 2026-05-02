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

 Date: 02/05/2026 21:54:46
*/

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- ----------------------------
-- Table structure for admin_audit_logs
-- ----------------------------
DROP TABLE IF EXISTS `admin_audit_logs`;
CREATE TABLE `admin_audit_logs`  (
  `log_id` bigint UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '日志id',
  `request_id` varchar(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '请求唯一标识(同一请求内的日志可互相关联)',
  `operator_admin_id` int UNSIGNED NOT NULL COMMENT '操作者管理员id',
  `operator_role` enum('super_admin','admin') CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '操作者权限',
  `action` varchar(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '动作名',
  `resource_type` varchar(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '资源类型',
  `result` enum('success','denied','failed') CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '操作结果',
  `created_at` datetime NOT NULL COMMENT '操作时间',
  `payload_text` longtext CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '完整日志内容',
  PRIMARY KEY (`log_id`) USING BTREE,
  INDEX `admin_role`(`operator_role` ASC) USING BTREE,
  INDEX `idx_created_at`(`created_at` ASC) USING BTREE,
  INDEX `idx_operator_time`(`created_at` ASC) USING BTREE,
  INDEX `idx_action_time`(`action` ASC, `created_at` ASC) USING BTREE,
  INDEX `idx_resource_time`(`resource_type` ASC, `created_at` ASC) USING BTREE,
  INDEX `idx_request_id`(`request_id` ASC) USING BTREE,
  INDEX `admin_id`(`operator_admin_id` ASC) USING BTREE,
  CONSTRAINT `admin_id` FOREIGN KEY (`operator_admin_id`) REFERENCES `admin_accounts` (`admin_id`) ON DELETE RESTRICT ON UPDATE RESTRICT,
  CONSTRAINT `admin_role` FOREIGN KEY (`operator_role`) REFERENCES `admin_accounts` (`role`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE = InnoDB AUTO_INCREMENT = 108 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_unicode_ci ROW_FORMAT = Dynamic;

SET FOREIGN_KEY_CHECKS = 1;

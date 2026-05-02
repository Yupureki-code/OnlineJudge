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

 Date: 02/05/2026 21:54:56
*/

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- ----------------------------
-- Table structure for cache_metrics_snapshots
-- ----------------------------
DROP TABLE IF EXISTS `cache_metrics_snapshots`;
CREATE TABLE `cache_metrics_snapshots`  (
  `id` bigint UNSIGNED NOT NULL AUTO_INCREMENT,
  `action_type` tinyint UNSIGNED NOT NULL COMMENT '0=question,1=auth,2=comment,3=solution,4=user',
  `total_requests` bigint UNSIGNED NOT NULL DEFAULT 0,
  `cache_hits` bigint UNSIGNED NOT NULL DEFAULT 0,
  `db_fallbacks` bigint UNSIGNED NOT NULL DEFAULT 0,
  `total_ms` bigint UNSIGNED NOT NULL DEFAULT 0,
  `created_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`) USING BTREE,
  INDEX `idx_action_time`(`action_type` ASC, `created_at` ASC) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 34 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_0900_ai_ci COMMENT = '缓存指标聚合快照' ROW_FORMAT = Dynamic;

SET FOREIGN_KEY_CHECKS = 1;

-- Base schema for OnlineJudge project
-- Source: OMNI.md database table definitions
-- Run this first before any migration scripts

CREATE DATABASE IF NOT EXISTS myoj
  DEFAULT CHARACTER SET utf8mb4
  DEFAULT COLLATE utf8mb4_unicode_ci;

USE myoj;

-- 用户基本数据
CREATE TABLE IF NOT EXISTS users (
  `uid` int unsigned NOT NULL AUTO_INCREMENT COMMENT '用户Id',
  `name` varchar(20) COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '用户名称',
  `password_hash` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '用户密码',
  `email` varchar(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '用户邮箱',
  `create_time` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '账号创建时间',
  `last_login` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '用户上次登陆时间',
  `password_algo` varchar(32) COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '密码哈希算法',
  `password_update_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '密码上次更新时间',
  PRIMARY KEY (`uid`),
  UNIQUE KEY `users_unique` (`name`),
  UNIQUE KEY `users_unique_1` (`email`)
) ENGINE=InnoDB AUTO_INCREMENT=1 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='用户基本数据';

-- 题目属性
CREATE TABLE IF NOT EXISTS questions (
  `id` varchar(5) COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '题目id',
  `title` varchar(128) COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '题目标题',
  `desc` text COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '题目描述',
  `star` varchar(5) COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '题目难度',
  `create_time` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '题目创建时间',
  `update_time` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '题目更新时间',
  `cpu_limit` tinyint NOT NULL COMMENT '时间限制',
  `memory_limit` int NOT NULL COMMENT '内存限制',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='题目属性';

-- 题目的测试用例
CREATE TABLE IF NOT EXISTS tests (
  `id` int NOT NULL AUTO_INCREMENT,
  `question_id` char(5) COLLATE utf8mb4_unicode_ci NOT NULL,
  `in` text COLLATE utf8mb4_unicode_ci NOT NULL,
  `out` text COLLATE utf8mb4_unicode_ci NOT NULL,
  `is_sample` tinyint(1) NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`),
  KEY `question_id` (`question_id`),
  CONSTRAINT `tests_ibfk_1` FOREIGN KEY (`question_id`) REFERENCES `questions` (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=1 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='题目的测试用例';

-- 管理员账户
CREATE TABLE IF NOT EXISTS admin_accounts (
  `admin_id` int unsigned NOT NULL AUTO_INCREMENT COMMENT '管理员id',
  `password_hash` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '管理员密码',
  `uid` int unsigned NOT NULL COMMENT '管理员账户所绑定的普通用户账户',
  `role` enum('super_admin','admin') CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '管理员权限',
  `created_at` datetime NOT NULL COMMENT '管理员账户创建时间',
  PRIMARY KEY (`admin_id`),
  KEY `user_uid` (`uid`),
  KEY `role` (`role`),
  CONSTRAINT `user_uid` FOREIGN KEY (`uid`) REFERENCES `users` (`uid`)
) ENGINE=InnoDB AUTO_INCREMENT=1 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 审计日志
CREATE TABLE IF NOT EXISTS admin_audit_logs (
  `log_id` bigint unsigned NOT NULL AUTO_INCREMENT COMMENT '日志id',
  `request_id` varchar(64) COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '请求唯一标识(同一请求内的日志可互相关联)',
  `operator_admin_id` int unsigned NOT NULL COMMENT '操作者管理员id',
  `operator_uid` int unsigned NOT NULL COMMENT '操作者用户id',
  `operator_role` enum('super_admin','admin') COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '操作者权限',
  `action` varchar(64) COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '动作名',
  `resource_type` varchar(64) COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '资源类型',
  `result` enum('success','denied','failed') COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '操作结果',
  `created_at` datetime NOT NULL COMMENT '操作时间',
  `payload_text` longtext COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '完整日志内容',
  PRIMARY KEY (`log_id`),
  KEY `admin_role` (`operator_role`),
  KEY `idx_created_at` (`created_at`) USING BTREE,
  KEY `idx_operator_time` (`operator_uid`,`created_at`) USING BTREE,
  KEY `idx_action_time` (`action`,`created_at`) USING BTREE,
  KEY `idx_resource_time` (`resource_type`,`created_at`) USING BTREE,
  KEY `idx_request_id` (`request_id`) USING BTREE,
  KEY `admin_id` (`operator_admin_id`) USING BTREE,
  CONSTRAINT `admin_id` FOREIGN KEY (`operator_admin_id`) REFERENCES `admin_accounts` (`admin_id`),
  CONSTRAINT `admin_role` FOREIGN KEY (`operator_role`) REFERENCES `admin_accounts` (`role`),
  CONSTRAINT `admin_uid` FOREIGN KEY (`operator_uid`) REFERENCES `admin_accounts` (`uid`)
) ENGINE=InnoDB AUTO_INCREMENT=1 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 题解表
CREATE TABLE IF NOT EXISTS solutions (
  `id` int unsigned NOT NULL AUTO_INCREMENT COMMENT '题解id',
  `question_id` varchar(5) COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '对应的题目id',
  `user_id` int unsigned NOT NULL COMMENT '作者uid',
  `title` varchar(30) COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '标题',
  `content_md` text COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '题解内容',
  `like_count` int unsigned NOT NULL DEFAULT '0' COMMENT '点赞数',
  `favorite_count` int unsigned NOT NULL DEFAULT '0' COMMENT '收藏数',
  `comment_count` int unsigned NOT NULL DEFAULT '0' COMMENT '评论数',
  `status` enum('rejected','pending','approved') COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '状态',
  `created_at` datetime NOT NULL COMMENT '创建时间',
  `updated_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
  PRIMARY KEY (`id`),
  KEY `question_id` (`question_id`) USING BTREE,
  KEY `user_id` (`user_id`) USING BTREE,
  CONSTRAINT `question_id_fk` FOREIGN KEY (`question_id`) REFERENCES `questions` (`id`),
  CONSTRAINT `user_id_fk` FOREIGN KEY (`user_id`) REFERENCES `users` (`uid`)
) ENGINE=InnoDB AUTO_INCREMENT=1 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 题解行为(点赞/收藏/评论)
CREATE TABLE IF NOT EXISTS solution_actions (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `solution_id` int unsigned NOT NULL,
  `user_id` int unsigned NOT NULL,
  `action_type` enum('like','comment','favorite') COLLATE utf8mb4_unicode_ci NOT NULL,
  `created_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  KEY `solutionid` (`solution_id`) USING BTREE,
  KEY `userid` (`user_id`) USING BTREE,
  CONSTRAINT `solutionid_fk` FOREIGN KEY (`solution_id`) REFERENCES `solutions` (`id`),
  CONSTRAINT `userid_fk` FOREIGN KEY (`user_id`) REFERENCES `users` (`uid`)
) ENGINE=InnoDB AUTO_INCREMENT=1 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 题解评论
CREATE TABLE IF NOT EXISTS solution_comments (
  `id` int unsigned AUTO_INCREMENT PRIMARY KEY,
  `solution_id` int unsigned NOT NULL,
  `user_id` int unsigned NOT NULL,
  `content` TEXT NOT NULL,
  `is_edited` TINYINT(1) DEFAULT 0,
  `parent_id` int unsigned DEFAULT NULL COMMENT '父评论ID，NULL=顶级评论',
  `reply_to_user_id` int unsigned DEFAULT NULL COMMENT '被回复用户ID',
  `like_count` int unsigned NOT NULL DEFAULT 0 COMMENT '点赞数',
  `favorite_count` int unsigned NOT NULL DEFAULT 0 COMMENT '收藏数',
  `created_at` DATETIME DEFAULT CURRENT_TIMESTAMP,
  `updated_at` DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  INDEX `idx_solution_id` (`solution_id`),
  INDEX `idx_user_id` (`user_id`),
  INDEX `idx_parent_id` (`parent_id`),
  CONSTRAINT `fk_comment_solution` FOREIGN KEY (`solution_id`) REFERENCES `solutions`(`id`) ON DELETE CASCADE,
  CONSTRAINT `fk_comment_user` FOREIGN KEY (`user_id`) REFERENCES `users`(`uid`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 用户提交记录
CREATE TABLE IF NOT EXISTS user_submits (
  `user_id` int unsigned NOT NULL COMMENT '用户id',
  `question_id` varchar(5) COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '题目Id',
  `result_json` text COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '判题结果json串',
  `is_pass` bit(1) NOT NULL COMMENT '是否通过(1通过/0未通过)',
  `action_time` datetime NOT NULL COMMENT '动作日期',
  KEY `uid` (`user_id`) USING BTREE,
  KEY `qid` (`question_id`) USING BTREE,
  CONSTRAINT `qid_fk` FOREIGN KEY (`question_id`) REFERENCES `questions` (`id`),
  CONSTRAINT `uid_fk` FOREIGN KEY (`user_id`) REFERENCES `users` (`uid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

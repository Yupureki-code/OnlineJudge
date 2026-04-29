-- 迁移脚本: 添加嵌套评论支持和评论互动表
-- 日期: 2026-04-28
-- 描述: 为 solution_comments 表增加 parent_id/reply_to_user_id/like_count/favorite_count 字段,
--       并创建 comment_actions 表用于记录用户对评论的点赞/收藏互动

-- 为 solution_comments 表增加嵌套评论相关字段
ALTER TABLE solution_comments
    ADD COLUMN  parent_id INT UNSIGNED DEFAULT NULL COMMENT '父评论ID，NULL=顶级评论' AFTER content,
    ADD COLUMN  reply_to_user_id INT UNSIGNED DEFAULT NULL COMMENT '被回复用户ID,用于@引用' AFTER parent_id,
    ADD COLUMN  like_count INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '点赞数' AFTER reply_to_user_id,
    ADD COLUMN  favorite_count INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '收藏数' AFTER like_count;

-- 为 parent_id 创建索引，支持按父评论查询子评论
ALTER TABLE solution_comments
    ADD INDEX idx_parent_id (parent_id);

-- 为 reply_to_user_id 创建索引
ALTER TABLE solution_comments
    ADD INDEX  idx_reply_user (reply_to_user_id);

-- 创建评论操作记录表（点赞、收藏）
CREATE TABLE IF NOT EXISTS comment_actions (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY COMMENT '主键',
    comment_id int UNSIGNED NOT NULL COMMENT '评论ID',
    user_id INT UNSIGNED NOT NULL COMMENT '用户ID',
    action_type ENUM('like', 'favorite') NOT NULL COMMENT '操作类型：like=点赞，favorite=收藏',
    created_at DATETIME NOT NULL COMMENT '创建时间',
    INDEX idx_comment_id (comment_id) COMMENT '评论ID索引',
    INDEX idx_user_action (user_id, action_type) COMMENT '用户操作联合索引',
    UNIQUE KEY uk_user_comment_action (user_id, comment_id, action_type) COMMENT '防止重复点赞/收藏',
    CONSTRAINT fk_comment_actions_comment FOREIGN KEY (comment_id) REFERENCES solution_comments(id) ON DELETE CASCADE,
    CONSTRAINT fk_comment_actions_user FOREIGN KEY (user_id) REFERENCES users(uid) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='评论操作记录表（点赞、收藏）';
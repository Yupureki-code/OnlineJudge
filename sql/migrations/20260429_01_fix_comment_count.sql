-- 修正所有题解评论数为仅顶级评论（parent_id IS NULL OR = 0）
-- 与 CreateComment/DeleteComment 的计数逻辑保持一致
UPDATE solutions s 
SET comment_count = (
    SELECT COUNT(*) FROM solution_comments 
    WHERE solution_id = s.id 
    AND (parent_id IS NULL OR parent_id = 0)
);

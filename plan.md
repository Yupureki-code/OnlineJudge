OnlineJudge
基于C++-基于分布式架构的在线OJ平台

准备工作:
阅读目录下的OMNI.md,明确项目基础构成和开发规范
随后仔细扫描项目构成,可参考AGENTS.md，明确模块功能，理清业务逻辑
最后阅读下面我所给出的需求，明确需求后，撰写开发详细文档

## 功能需求

### 1.UI调整

#### 1.1 网站壁纸更换

- **要求**:将oj_admin的网站壁纸更换成wwwroot/pictures/preview.jpg

#### 1.2 整体UI调整

- **现状**:oj_admin的UI布局过于简陋，难看，是简单的纯色组合
- **要求**:把整体的UI布局调整，风格跟oj_server主网站一直，其毛玻璃，色彩，模糊，动态效果相统一

### 2.缓存命中信息修复

- **现状**:oj_admin中可显示列表缓存命中率，详情缓存命中率，缓存运行概况等的信息。但是经过我的测试，该功能似乎没有实现
- **要求**:修复该功能，要求:显示oj_server的缓存信息，方便管理员观察延迟情况，为后续的优化工作做好准备。oj_model中包含了CacheMetric缓存指标类，记录着各种数据库增删查改的信息，包括延迟，你的任务是让oj_admin从oj_server中获取这个缓存指标，随后显示到oj_admin网页中，因此你需要在oj_server设置相关的API,model设置相关的接口函数。同时你需要扫描model中相关涉及到数据库操作的代码，检查是否都有缓存记录的过程，查漏补缺。

### 3.题目板块调整

#### 3.1 题目编辑板块UI调整

- **现状**:在题目编辑中的题目描述的板块中，即<textarea id="g-desc'placeholder="题目描还</textarea>，该板块大小过小，请扩大该板块，直至整个板块的下边界与屏幕最下方对齐

#### 3.2 测试用例调整

- **现状**:题目不仅包括题目描述，还包括对应的测试用例
- **要求**:听我的指示，添加对测试用例编辑/新增的功能:在对应题目的编辑题目的板块中，在上方添加"编辑测试用例"的按钮，点击该按钮后，该板块变成编辑测试用例的板块。一个测试用路包括:测试用例id,题目id,输入文本，输出文本，是否是样例。因此你需要在界面中规范地展示这些信息，以供良好地编辑。同时在编辑板块内，加入滚轮，如果文本过长，则可用鼠标滚轮滚动展示。
测试用例表:
CREATE TABLE `tests` (
  `id` tinyint NOT NULL AUTO_INCREMENT,
  `question_id` char(5) COLLATE utf8mb4_unicode_ci NOT NULL,
  `in` text COLLATE utf8mb4_unicode_ci NOT NULL,
  `out` text COLLATE utf8mb4_unicode_ci NOT NULL,
  `is_sample` tinyint(1) NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`,`question_id`),
  KEY `question_id` (`question_id`),
  CONSTRAINT `tests_ibfk_1` FOREIGN KEY (`question_id`) REFERENCES `questions` (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=5 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='题目的测试用例'
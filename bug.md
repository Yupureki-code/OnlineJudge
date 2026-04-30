OnlineJudge
基于C++-基于分布式架构的在线OJ平台

说明:
该文档为执行完plan.md中的方案后，经本人测试，发现存在的bug，在此提出并要求解决
当修改完成后，你需要进行详细的检查，检查到底修改正确与否，可参考我的检查建议

# 1.SPA导航栏布局问题

# **现状**:上方的SPA导航栏(<div class="header" style="position:relative;z-index:3oooo;">)样式不统一:1.个人中心的导航栏过宽;2.题库页面中，网站logo(img src="./pictures/icon.png" type="icon" width="4opx" height="4opx">)和头像(<img src="/pictures/head.jpg" width="40" height="40" style="border-radiu
s:5o%;display:block;object-fit: cover;">)的左右间距和其他页面不统一。
# **要求**:将整个网站，所有页面的SPA导航栏样式统一成主页(index.html)的样式

# 2.题解评论数统计错误

- **现状**:如果将某个题解下面的评论全部删除。在题解列表显示该题解的评论数不变
- **要求**:网页正确动态地显示题解的评论数

# 3.提示信息添加

- **现状**:评论区发表评论，在一级评论下发表嵌套评论，回复嵌套评论后，没有提示信息
- **要求**:做出上述动作后，网站需要给出提示信息:"发布成功!"或者"发布失败!"

# 4.发布题解文本编辑器更换

- **现状**:发布题解的页面中，使用的文本编辑器是vditor,
- **要求**:将其换成editor.md，官网:https://pandao.github.io/editor.md/




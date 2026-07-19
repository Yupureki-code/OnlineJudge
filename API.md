# OnlineJudge API 文档

**Base URL**: `http://localhost:8080`  
**Content-Type**: `application/json` (除文件上传外)  
**认证方式**: Cookie `oj_session_id`（7天有效期，HttpOnly，Secure，SameSite=Lax）

---

## 一、认证接口

### 1.1 发送邮箱验证码

```
POST /api/auth/send_code
```

**Request**
```json
{
  "email": "user@example.com"
}
```

**Response 200**
```json
{
  "success": true,
  "retry_after_seconds": 60
}
```

**错误码**

| 状态码 | error_code | 说明 |
|--------|------------|------|
| 400 | - | 邮箱格式不合法 |
| 429 | TOO_MANY_REQUESTS | 60秒冷却期内重复发送 |
| 429 | EMAIL_DAILY_LIMIT | 该邮箱当日发送已达20次上限 |
| 429 | IP_DAILY_LIMIT | 该IP当日发送已达50次上限 |

---

### 1.2 邮箱验证码登录/注册

```
POST /api/auth/verify_code
```

**Request**
```json
{
  "email": "user@example.com",
  "code": "123456",
  "name": "用户名",
  "password": "Pass1234"
}
```

| 字段 | 必填 | 说明 |
|------|------|------|
| email | 是 | 邮箱地址 |
| code | 是 | 6位数字验证码 |
| name | 否 | 新注册时的用户名 |
| password | 否 | 设置密码（8-72字符，需含字母+数字） |

**Response 200**
```json
{
  "success": true,
  "is_new_user": true,
  "user": {
    "uid": 1,
    "name": "用户名",
    "email": "user@example.com",
    "create_time": "2026-01-01 00:00:00",
    "last_login": "2026-01-01 00:00:00"
  }
}
```

**Set-Cookie**: `oj_session_id=<session_id>; Expires=...; Path=/; HttpOnly; Secure; SameSite=Lax`

| 状态码 | error_code | 说明 |
|--------|------------|------|
| 400 | - | 邮箱/验证码格式错误 |
| 400 | INVALID_CODE | 验证码错误 |
| 429 | ATTEMPTS_EXceeded | 验证码尝试超过5次，已失效 |

---

### 1.3 密码登录

```
POST /api/user/password/login
```

**速率限制**: 10次/分钟/IP

**Request**
```json
{
  "email": "user@example.com",
  "password": "Pass1234"
}
```

或使用用户名：
```json
{
  "username": "用户名",
  "password": "Pass1234"
}
```

**Response 200**
```json
{
  "success": true,
  "user": {
    "uid": 1,
    "name": "用户名",
    "email": "user@example.com",
    "create_time": "2026-01-01 00:00:00",
    "last_login": "2026-01-01 00:00:00"
  }
}
```

| 状态码 | error_code | 说明 |
|--------|------------|------|
| 400 | - | 用户名/邮箱或密码格式错误 |
| 400 | LOGIN_FAILED | 用户不存在或密码错误 |
| 429 | RATE_LIMITED | 登录尝试过于频繁 |

---

### 1.4 账户登出

```
POST /api/user/logout
```

**认证**: 无需（未登录时也返回成功）

**Response 200**
```json
{
  "success": true
}
```

**Set-Cookie**: `oj_session_id=; Expires=Thu, 01 Jan 1970 ...`（清除Cookie）

---

### 1.5 废弃接口

```
POST /api/user/login
```

**Response 410**
```json
{
  "success": false,
  "error_code": "DEPRECATED",
  "message": "此接口已废弃，请使用 /api/user/password/login 进行密码登录"
}
```

---

## 二、用户接口

### 2.1 获取当前用户信息

```
GET /api/user/info
```

**认证**: 可选（未登录返回 `success: false`）

**Response 200**
```json
{
  "success": true,
  "user": {
    "uid": 1,
    "name": "用户名",
    "email": "user@example.com",
    "avatar_url": "/pictures/avatars/1.jpg",
    "create_time": "2026-01-01 00:00:00"
  }
}
```

---

### 2.2 获取用户统计

```
GET /api/user/stats
```

**认证**: 必须

**Response 200**
```json
{
  "success": true,
  "total_submitted": 100,
  "total_passed": 50,
  "pass_rate": 0.5
}
```

| 状态码 | 说明 |
|--------|------|
| 401 | 未登录 |

---

### 2.3 创建用户

```
POST /api/user/create
```

**Request**
```json
{
  "name": "用户名",
  "email": "user@example.com"
}
```

**Response 200**
```json
{
  "success": true,
  "created": true
}
```

**Set-Cookie**: 新用户创建成功后自动设置session

| 状态码 | 说明 |
|--------|------|
| 400 | 用户名为空或超过64字符 |
| 400 | 邮箱格式不合法 |

---

### 2.4 检查用户是否存在

```
POST /api/user/check
```

**Request**
```json
{
  "email": "user@example.com"
}
```

**Response 200**
```json
{
  "success": true,
  "exists": true,
  "has_password": true
}
```

---

### 2.5 获取指定用户信息

```
POST /api/user/get
```

**Request**
```json
{
  "email": "user@example.com"
}
```

**Response 200**
```json
{
  "success": true,
  "uid": 1,
  "name": "用户名",
  "email": "user@example.com",
  "create_time": "2026-01-01 00:00:00",
  "last_login": "2026-01-01 00:00:00"
}
```

---

### 2.6 上传头像

```
POST /api/user/avatar
Content-Type: multipart/form-data
```

**认证**: 必须

**Request**: 表单字段 `avatar`（文件），支持 JPEG/PNG/GIF/WebP，最大2MB

**Response 200**
```json
{
  "success": true,
  "avatar_url": "/pictures/avatars/1.jpg"
}
```

| 状态码 | error_code | 说明 |
|--------|------------|------|
| 400 | NO_FILE | 未上传文件 |
| 400 | INVALID_TYPE | 不支持的文件类型 |
| 400 | FILE_TOO_LARGE | 文件超过2MB |

---

### 2.7 删除头像

```
DELETE /api/user/avatar
```

**认证**: 必须

**Response 200**
```json
{
  "success": true
}
```

---

### 2.8 修改用户名

```
POST /api/user/name
```

**认证**: 必须

**Request**
```json
{
  "name": "新用户名"
}
```

**Response 200**
```json
{
  "success": true,
  "name": "新用户名"
}
```

| 状态码 | error_code | 说明 |
|--------|------------|------|
| 400 | INVALID_NAME | 用户名为空或超过20字符 |
| 409 | NAME_TAKEN | 用户名已被占用 |

---

## 三、密码管理接口

### 3.1 设置密码

```
POST /api/user/password/set
```

**认证**: 必须

**Request**
```json
{
  "password": "NewPass1234"
}
```

**Response 200**
```json
{
  "success": true
}
```

| 状态码 | 说明 |
|--------|------|
| 400 | 密码不符合要求（8-72字符，含字母+数字） |

---

### 3.2 更改密码

```
POST /api/user/password/change
```

**认证**: 必须

**Request**
```json
{
  "code": "123456",
  "new_password": "NewPass1234"
}
```

**Response 200**
```json
{
  "success": true
}
```

| 状态码 | error_code | 说明 |
|--------|------------|------|
| 400 | - | 验证码/密码格式错误 |
| 429 | ATTEMPTS_EXCEEDED | 验证码尝试超过5次 |

---

### 3.3 发送安全验证码

```
POST /api/user/security/send_code
```

**认证**: 必须

**Response 200**
```json
{
  "success": true,
  "retry_after_seconds": 60
}
```

---

### 3.4 更改邮箱

```
POST /api/user/email/change
```

**认证**: 必须

**Request**
```json
{
  "code": "123456",
  "new_email": "new@example.com"
}
```

**Response 200**: 更改成功后重新设置session cookie
```json
{
  "success": true,
  "user": {
    "uid": 1,
    "name": "用户名",
    "email": "new@example.com",
    "create_time": "2026-01-01 00:00:00",
    "last_login": "2026-01-01 00:00:00"
  }
}
```

---

### 3.5 注销账户

```
POST /api/user/account/delete
```

**认证**: 必须

**Request**
```json
{
  "code": "123456"
}
```

**Response 200**: 注销成功后清除session cookie
```json
{
  "success": true
}
```

---

## 四、题目接口

### 4.1 获取题目基本信息

```
GET /api/question/{id}
```

**Response 200**
```json
{
  "success": true,
  "question": {
    "number": "1",
    "title": "两数之和",
    "star": "简单"
  }
}
```

| 状态码 | error_code | 说明 |
|--------|------------|------|
| 404 | QUESTION_NOT_FOUND | 题目不存在 |

---

### 4.2 获取题目示例测试用例

```
GET /api/question/{id}/sample_tests
```

**Response 200**
```json
{
  "success": true,
  "tests": [
    {
      "in": "1 2",
      "out": "3"
    }
  ]
}
```

| 状态码 | error_code | 说明 |
|--------|------------|------|
| 404 | QUESTION_NOT_FOUND | 题目不存在 |

---

### 4.3 查询题目通过状态

```
GET /api/question/{id}/pass_status
```

**认证**: 可选（未登录返回 `passed: false, logged_in: false`）

**Response 200**
```json
{
  "success": true,
  "passed": true,
  "logged_in": true
}
```

---

### 4.4 运行单次测试

```
POST /api/question/{id}/test
```

**认证**: 可选

**Request**
```json
{
  "code": "#include <iostream>\nint main(){int a,b;std::cin>>a>>b;std::cout<<a+b;return 0;}",
  "test_type": "sample",
  "test_case_id": 0,
  "custom_input": ""
}
```

| 字段 | 必填 | 说明 |
|------|------|------|
| code | 是 | C++ 源代码 |
| test_type | 否 | `"sample"`（默认）或 `"custom"` |
| test_case_id | 否 | 指定测试用例ID（默认0=第一个） |
| custom_input | 否 | 自定义输入（test_type="custom"时使用） |

**Response 200**
```json
{
  "success": true,
  "status": "OK",
  "desc": "",
  "stdout": "3",
  "stderr": ""
}
```

| 状态码 | error_code | 说明 |
|--------|------------|------|
| 400 | - | 缺少代码 |
| 404 | QUESTION_NOT_FOUND | 题目不存在 |
| 404 | TEST_NOT_FOUND | 测试用例不存在 |

---

### 4.5 获取用户提交记录

```
GET /api/question/{id}/submits
```

**认证**: 可选（未登录返回 guest 信息）

**Response 200**
```json
{
  "success": true,
  "submits": [
    {
      "id": 1,
      "status": "AC AC AC",
      "is_pass": true,
      "created_at": "2026-01-01 00:00:00"
    }
  ]
}
```

---

### 4.6 提交判题

```
POST /judge/{id}
Content-Type: application/json 或 application/x-www-form-urlencoded
```

**Request (JSON)**
```json
{
  "code": "#include <iostream>\nint main(){int a,b;std::cin>>a>>b;std::cout<<a+b;return 0;}"
}
```

**Request (Form)**: `code=<源代码>`

**Response**: 302 重定向到 `/judge_result.html?result={url_encoded_json}&id={question_id}`

判题结果JSON格式：
```json
{
  "status": "AC AC AC",
  "desc": "恭喜你!已通过此题",
  "stdout": "",
  "stderr": ""
}
```

状态码含义：`AC`=通过, `WA`=错误, `RUNTIME_ERROR`=超时, `MEMORY_LIMIT`=内存超限, `COMPILE_ERROR`=编译错误, `FPE_ERROR`=浮点异常, `SEGV_ERROR`=段错误, `UNKNOWN`=未知

---

## 五、题解接口

### 5.1 发布题解

```
POST /api/questions/{question_id}/solutions
```

**认证**: 必须（需先通过该题目）

**Request**
```json
{
  "title": "题解标题",
  "content_md": "# 解题思路\n\nMarkdown内容..."
}
```

**Response 200**
```json
{
  "success": true,
  "solution_id": 1,
  "question_id": "1"
}
```

| 状态码 | error_code | 说明 |
|--------|------------|------|
| 400 | - | 标题/内容为空 |
| 401 | UNAUTHORIZED | 未登录 |
| 401 | NOT_PASSED | 未通过该题目 |
| 404 | QUESTION_NOT_FOUND | 题目不存在 |

---

### 5.2 获取题解列表

```
GET /api/questions/{question_id}/solutions?status=approved&sort=latest&page=1&size=10
```

| 参数 | 默认值 | 说明 |
|------|--------|------|
| status | approved | 筛选状态：approved/pending/rejected |
| sort | latest | 排序：latest/hot |
| page | 1 | 页码 |
| size | 10 | 每页数量（1-50） |

**Response 200**
```json
{
  "success": true,
  "total": 100,
  "page": 1,
  "size": 10,
  "solutions": [
    {
      "id": 1,
      "question_id": "1",
      "user_id": 1,
      "title": "题解标题",
      "content_md": "Markdown内容",
      "like_count": 10,
      "favorite_count": 5,
      "comment_count": 3,
      "status": "approved",
      "created_at": "2026-01-01 00:00:00",
      "updated_at": "2026-01-01 00:00:00",
      "author_name": "作者名",
      "author_avatar": "/pictures/avatars/1.jpg"
    }
  ]
}
```

---

### 5.3 获取题解详情

```
GET /api/solutions/{id}
```

**Response 200**
```json
{
  "success": true,
  "solution": {
    "id": 1,
    "question_id": "1",
    "user_id": 1,
    "title": "题解标题",
    "content_md": "Markdown内容",
    "like_count": 10,
    "favorite_count": 5,
    "comment_count": 3,
    "status": "approved",
    "created_at": "2026-01-01 00:00:00",
    "updated_at": "2026-01-01 00:00:00",
    "author_name": "作者名",
    "author_avatar": "/pictures/avatars/1.jpg"
  }
}
```

| 状态码 | error_code | 说明 |
|--------|------------|------|
| 404 | SOLUTION_NOT_FOUND | 题解不存在 |

---

### 5.4 点赞/取消点赞

```
POST /api/solutions/{id}/like
```

**认证**: 必须

**Response 200**
```json
{
  "success": true,
  "liked": true,
  "like_count": 11
}
```

---

### 5.5 收藏/取消收藏

```
POST /api/solutions/{id}/favorite
```

**认证**: 必须

**Response 200**
```json
{
  "success": true,
  "favorited": true,
  "favorite_count": 6
}
```

---

### 5.6 获取用户对题解的互动状态

```
GET /api/solutions/{id}/actions
```

**认证**: 可选（未登录返回 false）

**Response 200**
```json
{
  "success": true,
  "liked": true,
  "favorited": false
}
```

---

## 六、评论接口

### 6.1 获取评论列表

```
GET /api/solutions/{solution_id}/comments?page=1&size=20
```

| 参数 | 默认值 | 说明 |
|------|--------|------|
| page | 1 | 页码 |
| size | 20 | 每页数量（1-1000） |

**Response 200**
```json
{
  "success": true,
  "total": 50,
  "comments": [
    {
      "id": 1,
      "solution_id": 1,
      "user_id": 1,
      "content": "评论内容",
      "is_edited": false,
      "parent_id": 0,
      "reply_to_user_id": 0,
      "reply_to_user_name": "",
      "like_count": 5,
      "favorite_count": 2,
      "author_name": "评论者",
      "author_avatar": "/pictures/avatars/1.jpg",
      "created_at": "2026-01-01 00:00:00",
      "updated_at": "2026-01-01 00:00:00",
      "reply_count": 3
    }
  ]
}
```

---

### 6.2 发表评论

```
POST /api/solutions/{solution_id}/comments
```

**认证**: 必须

**Request**
```json
{
  "content": "评论内容（最多1000字符）",
  "parent_id": 0
}
```

| 字段 | 必填 | 说明 |
|------|------|------|
| content | 是 | 评论内容，最多1000字符 |
| parent_id | 否 | 回复的评论ID（0=顶层评论） |

**Response 200**
```json
{
  "success": true,
  "comment": {
    "id": 2,
    "content": "评论内容",
    "created_at": "2026-01-01 00:00:00"
  }
}
```

| 状态码 | error_code | 说明 |
|--------|------------|------|
| 400 | - | 内容为空 |
| 401 | UNAUTHORIZED | 未登录 |
| 404 | SOLUTION_NOT_FOUND | 题解不存在 |

---

### 6.3 编辑评论

```
PUT /api/comments/{id}
```

**认证**: 必须（仅评论作者）

**Request**
```json
{
  "content": "修改后的评论内容"
}
```

**Response 200**
```json
{
  "success": true,
  "comment": {
    "id": 1,
    "content": "修改后的评论内容",
    "is_edited": true,
    "updated_at": "2026-01-01 00:00:00"
  }
}
```

| 状态码 | error_code | 说明 |
|--------|------------|------|
| 400 | - | 内容为空 |
| 403 | FORBIDDEN | 非评论作者 |
| 404 | NOT_FOUND | 评论不存在 |

---

### 6.4 删除评论

```
DELETE /api/comments/{id}
```

**认证**: 必须（仅评论作者）

**Response 200**
```json
{
  "success": true
}
```

| 状态码 | error_code | 说明 |
|--------|------------|------|
| 403 | FORBIDDEN | 非评论作者 |
| 404 | NOT_FOUND | 评论不存在 |

---

### 6.5 获取回复列表

```
GET /api/comments/{id}/replies?page=1&size=20
```

**Response 200**
```json
{
  "success": true,
  "total": 5,
  "replies": [
    {
      "id": 2,
      "content": "回复内容",
      "parent_id": 1,
      "reply_to_user_id": 1,
      "reply_to_user_name": "被回复者",
      "author_name": "回复者",
      "author_avatar": "/pictures/avatars/2.jpg",
      "like_count": 1,
      "created_at": "2026-01-01 00:00:00"
    }
  ]
}
```

---

### 6.6 评论点赞/取消点赞

```
POST /api/comments/{id}/like
```

**认证**: 必须

**Response 200**
```json
{
  "success": true,
  "liked": true,
  "like_count": 6
}
```

---

### 6.7 批量获取评论互动状态

```
GET /api/comments/actions?ids=1,2,3
```

**认证**: 可选（未登录返回 false）

**Response 200**
```json
{
  "success": true,
  "actions": {
    "1": { "like": true, "favorite": false },
    "2": { "like": false, "favorite": false },
    "3": { "like": false, "favorite": true }
  }
}
```

---

## 七、缓存管理接口

### 7.1 清除静态页面缓存

```
POST /api/static/cache/invalidate
```

**认证**: 必须

**Request**
```json
{
  "page": "index.html"
}
```

或批量：
```json
{
  "pages": ["index.html", "all_questions.html"]
}
```

**Response 200**
```json
{
  "success": true,
  "operator": "user@example.com",
  "invalidated_pages": ["index.html"],
  "count": 1
}
```

---

## 八、页面路由（返回HTML）

| 路由 | 说明 |
|------|------|
| `GET /` | 首页 |
| `GET /all_questions?page=&size=` | 题库列表 |
| `GET /about` | 关于页面 |
| `GET /user/profile` | 个人中心 |
| `GET /user/settings` | 账户设置 |
| `GET /questions/{id}` | 题目详情 |
| `GET /questions/{id}/solutions/new` | 发布题解页 |
| `GET /solutions/{id}` | 题解详情页 |
| `GET /judge_result.html?result=&id=` | 判题结果页 |

所有页面均通过服务端渲染注入用户信息（`SERVER_USER_INFO` 全局变量）。

---

## 九、错误码汇总

| error_code | HTTP状态码 | 说明 |
|------------|------------|------|
| INVALID_JSON | 400 | 请求体非合法JSON |
| UNAUTHORIZED | 401 | 未登录或session已过期 |
| DEPRECATED | 410 | 接口已废弃 |
| RATE_LIMITED | 429 | 请求频率超限 |
| TOO_MANY_REQUESTS | 429 | 验证码发送冷却中 |
| EMAIL_DAILY_LIMIT | 429 | 邮箱每日发送上限 |
| IP_DAILY_LIMIT | 429 | IP每日发送上限 |
| ATTEMPTS_EXCEEDED | 429 | 验证码尝试次数超限 |
| QUESTION_NOT_FOUND | 404 | 题目不存在 |
| SOLUTION_NOT_FOUND | 404 | 题解不存在 |
| COMMENT_NOT_FOUND | 404 | 评论不存在 |
| NOT_FOUND | 404 | 资源不存在 |
| NOT_PASSED | 401 | 未通过该题目 |
| FORBIDDEN | 403 | 无权操作 |
| INVALID_NAME | 400 | 用户名不合法 |
| NAME_TAKEN | 409 | 用户名已被占用 |
| INVALID_TYPE | 400 | 文件类型不支持 |
| FILE_TOO_LARGE | 400 | 文件超过大小限制 |
| NO_FILE | 400 | 未上传文件 |
| INVALID_ID | 400 | ID参数不合法 |
| DB_ERROR | 500 | 数据库错误 |

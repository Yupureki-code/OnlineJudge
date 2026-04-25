(function () {
    var state = {
        usersPage: 1,
        usersSize: 20,
        usersKeyword: "",
        questionsPage: 1,
        questionsSize: 20,
        questionsKeyword: "",
        lastQuestionsListPath: "/admin/questions"
    };

    function escapeHtml(v) {
        return String(v || "")
            .replace(/&/g, "&amp;")
            .replace(/</g, "&lt;")
            .replace(/>/g, "&gt;")
            .replace(/\"/g, "&quot;")
            .replace(/'/g, "&#39;");
    }

    function formatRate(value) {
        var num = Number(value || 0);
        return num.toFixed(1) + "%";
    }

    async function getJson(url) {
        var resp = await fetch(url, { credentials: "include" });
        var data = {};
        try { data = await resp.json(); } catch (e) { data = { success: false, error_code: "BAD_JSON" }; }
        return { ok: resp.ok, status: resp.status, data: data };
    }

    async function postJson(url, body) {
        var resp = await fetch(url, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            credentials: "include",
            body: JSON.stringify(body || {})
        });
        var data = {};
        try { data = await resp.json(); } catch (e) { data = { success: false }; }
        return { ok: resp.ok, status: resp.status, data: data };
    }

    async function putJson(url, body) {
        var resp = await fetch(url, {
            method: "PUT",
            headers: { "Content-Type": "application/json" },
            credentials: "include",
            body: JSON.stringify(body || {})
        });
        var data = {};
        try { data = await resp.json(); } catch (e) { data = { success: false }; }
        return { ok: resp.ok, status: resp.status, data: data };
    }

    async function deleteJson(url) {
        var resp = await fetch(url, {
            method: "DELETE",
            credentials: "include"
        });
        var data = {};
        try { data = await resp.json(); } catch (e) { data = { success: false }; }
        return { ok: resp.ok, status: resp.status, data: data };
    }

    function renderError(message) {
        return '<div class="error">' + escapeHtml(message) + '</div>';
    }

    function redirectToLogin() {
        window.location.href = "/admin/login";
    }

    function isAuthFailure(result) {
        return result && (result.status === 401 || result.status === 404);
    }

    function setUserHint() {
        var userEl = document.getElementById("admin-user");
        var metaEl = document.getElementById("admin-user-meta");
        var user = window.SERVER_USER_INFO || {};
        if (user.isLoggedIn) {
            userEl.textContent = "欢迎管理员ID:" + (user.admin_id || "-") + "!";
            if (metaEl) {
                metaEl.textContent = "角色: " + (user.role || "admin") + " | 绑定用户: " + (user.name || user.email || "Unknown");
            }
        } else {
            userEl.textContent = "未登录";
            if (metaEl) {
                metaEl.textContent = "";
            }
        }
    }

    function updateNavActive(pathname) {
        var navs = document.querySelectorAll(".nav-btn[data-path]");
        for (var i = 0; i < navs.length; i++) {
            var itemPath = navs[i].getAttribute("data-path");
            var isActive = itemPath === "/admin"
                ? pathname === "/admin"
                : pathname.indexOf(itemPath) === 0;
            navs[i].classList.toggle("active", isActive);
        }
    }

    function navigateTo(path, replace) {
        if (replace) {
            window.history.replaceState({}, "", path);
        } else {
            window.history.pushState({}, "", path);
        }
        renderCurrentRoute();
    }

    function readUsersStateFromLocation() {
        var params = new URLSearchParams(window.location.search);
        state.usersPage = Math.max(1, parseInt(params.get("page") || "1", 10) || 1);
        state.usersSize = Math.max(1, parseInt(params.get("size") || "20", 10) || 20);
        state.usersKeyword = (params.get("q") || "").trim();
    }

    function readQuestionsStateFromLocation() {
        var params = new URLSearchParams(window.location.search);
        state.questionsPage = Math.max(1, parseInt(params.get("page") || "1", 10) || 1);
        state.questionsSize = Math.max(1, parseInt(params.get("size") || "20", 10) || 20);
        state.questionsKeyword = (params.get("q") || "").trim();
    }

    function buildQuestionsListPath(overrides) {
        var next = overrides || {};
        var params = new URLSearchParams();
        var page = next.page !== undefined ? next.page : state.questionsPage;
        var size = next.size !== undefined ? next.size : state.questionsSize;
        var keyword = next.keyword !== undefined ? next.keyword : state.questionsKeyword;

        params.set("page", String(page));
        params.set("size", String(size));
        if (keyword) {
            params.set("q", keyword);
        }
        return "/admin/questions?" + params.toString();
    }

    function getQuestionReturnPath() {
        var params = new URLSearchParams(window.location.search);
        var returnPath = params.get("return") || state.lastQuestionsListPath || "/admin/questions";
        if (returnPath.indexOf("/admin/questions") !== 0) {
            return "/admin/questions";
        }
        return returnPath;
    }

    function questionEditorPath(number) {
        var returnPath = encodeURIComponent(state.lastQuestionsListPath || buildQuestionsListPath());
        if (number) {
            return "/admin/questions/" + encodeURIComponent(number) + "?return=" + returnPath;
        }
        return "/admin/questions/new?return=" + returnPath;
    }

    function renderOverviewCards(data) {
        return [
            '<div class="metric-grid">',
            '<div class="metric"><div class="k">题目总数</div><div class="v">' + (data.question_count || 0) + '</div><div class="s">题库规模</div></div>',
            '<div class="metric"><div class="k">用户总数</div><div class="v">' + (data.user_count || 0) + '</div><div class="s">已注册账户</div></div>',
            '<div class="metric"><div class="k">管理员总数</div><div class="v">' + (data.admin_count || 0) + '</div><div class="s">后台可登录账户</div></div>',
            '<div class="metric"><div class="k">列表缓存命中率</div><div class="v">' + formatRate(data.cache && data.cache.list_hit_rate) + '</div><div class="s">请求 ' + ((data.cache && data.cache.list_requests) || 0) + '</div></div>',
            '<div class="metric"><div class="k">详情缓存命中率</div><div class="v">' + formatRate(data.cache && data.cache.detail_hit_rate) + '</div><div class="s">请求 ' + ((data.cache && data.cache.detail_requests) || 0) + '</div></div>',
            '<div class="metric"><div class="k">审计日志总量</div><div class="v">' + (data.recent_audit_total || 0) + '</div><div class="s">最近操作可追溯</div></div>',
            '</div>'
        ].join('');
    }

    function renderOverviewTable(title, subtitle, headerHtml, bodyHtml) {
        return [
            '<section class="panel">',
            '<div class="page-head"><div><h3>' + title + '</h3><div class="page-subtitle">' + subtitle + '</div></div></div>',
            '<table class="table-compact"><thead>' + headerHtml + '</thead><tbody>' + bodyHtml + '</tbody></table>',
            '</section>'
        ].join('');
    }

    async function renderOverview() {
        var content = document.getElementById("content");
        content.innerHTML = "加载中...";

        var result = await getJson("/api/admin/overview");
        if (isAuthFailure(result)) {
            redirectToLogin();
            return;
        }
        if (!result.ok || !result.data.success) {
            content.innerHTML = renderError(result.status === 403 ? "当前账号没有管理权限" : "读取总览失败");
            return;
        }

        var data = result.data.data || {};
        var cache = data.cache || {};
        var roleData = data.admin_roles || {};
        var recentUsers = data.recent_users || [];
        var recentQuestions = data.recent_questions || [];
        var recentAudits = data.recent_audits || [];

        var recentUsersBody = recentUsers.length ? recentUsers.map(function (user) {
            return '<tr><td>' + escapeHtml(user.uid) + '</td><td>' + escapeHtml(user.name) + '</td><td>' + escapeHtml(user.email) + '</td><td>' + escapeHtml(user.create_time) + '</td></tr>';
        }).join('') : '<tr><td colspan="4" class="empty">暂无用户数据</td></tr>';

        var recentQuestionsBody = recentQuestions.length ? recentQuestions.map(function (question) {
            return '<tr><td>#' + escapeHtml(question.number) + '</td><td>' + escapeHtml(question.title) + '</td><td>' + escapeHtml(question.star) + '</td><td>' + escapeHtml(question.update_time) + '</td></tr>';
        }).join('') : '<tr><td colspan="4" class="empty">暂无题目数据</td></tr>';

        var recentAuditsBody = recentAudits.length ? recentAudits.map(function (log) {
            return '<tr>' +
                '<td>' + escapeHtml(log.action) + '</td>' +
                '<td>' + escapeHtml(log.operator_admin_id) + '</td>' +
                '<td class="status-' + escapeHtml(log.result) + '">' + escapeHtml(log.result) + '</td>' +
                '<td>' + escapeHtml(log.resource_type) + '</td>' +
                '</tr>';
        }).join('') : '<tr><td colspan="4" class="empty">暂无审计日志</td></tr>';

        content.innerHTML = [
            '<div class="page-head"><div><h2>仪表盘</h2><div class="page-subtitle">围绕题库、缓存与后台操作的管理概览</div></div></div>',
            renderOverviewCards(data),
            '<div class="overview-grid">',
            '<div class="stack">',
            '<section class="panel">',
            '<h3>缓存运行概况</h3>',
            '<div class="page-subtitle">用于观察主站题库页与详情页的命中情况</div>',
            '<div class="pill-row">',
            '<span class="pill">列表命中 ' + formatRate(cache.list_hit_rate) + '</span>',
            '<span class="pill">详情命中 ' + formatRate(cache.detail_hit_rate) + '</span>',
            '<span class="pill">静态页命中 ' + formatRate(cache.html_static_hit_rate) + '</span>',
            '<span class="pill">列表HTML命中 ' + formatRate(cache.html_list_hit_rate) + '</span>',
            '<span class="pill">详情HTML命中 ' + formatRate(cache.html_detail_hit_rate) + '</span>',
            '</div>',
            '<table class="table-compact">',
            '<thead><tr><th>链路</th><th>请求</th><th>Miss</th><th>DB回退</th><th>平均耗时</th></tr></thead>',
            '<tbody>',
            '<tr><td>题库列表</td><td>' + (cache.list_requests || 0) + '</td><td>' + (cache.list_misses || 0) + '</td><td>' + (cache.list_db_fallbacks || 0) + '</td><td>' + Number(cache.list_avg_ms || 0).toFixed(2) + ' ms</td></tr>',
            '<tr><td>题目详情</td><td>' + (cache.detail_requests || 0) + '</td><td>' + (cache.detail_misses || 0) + '</td><td>' + (cache.detail_db_fallbacks || 0) + '</td><td>' + Number(cache.detail_avg_ms || 0).toFixed(2) + ' ms</td></tr>',
            '</tbody></table>',
            '</section>',
            renderOverviewTable('最近注册用户', '按注册时间倒序展示最近 5 条', '<tr><th>UID</th><th>用户名</th><th>邮箱</th><th>注册时间</th></tr>', recentUsersBody),
            renderOverviewTable('最近更新题目', '帮助快速定位近期维护的题目', '<tr><th>题号</th><th>标题</th><th>难度</th><th>最近更新时间</th></tr>', recentQuestionsBody),
            '</div>',
            '<div class="stack">',
            '<section class="panel">',
            '<h3>管理员构成</h3>',
            '<div class="page-subtitle">当前后台权限结构</div>',
            '<div class="pill-row">',
            '<span class="pill">超级管理员 ' + (roleData.super_admin || 0) + '</span>',
            '<span class="pill">普通管理员 ' + (roleData.admin || 0) + '</span>',
            '</div>',
            '</section>',
            renderOverviewTable('最近审计日志', '展示最近 5 条后台操作结果', '<tr><th>动作</th><th>管理员ID</th><th>结果</th><th>资源</th></tr>', recentAuditsBody),
            '</div>',
            '</div>'
        ].join('');
    }

    async function renderUsers() {
        var content = document.getElementById("content");
        content.innerHTML = "加载中...";

        readUsersStateFromLocation();
        var url = "/api/admin/users?page=" + state.usersPage + "&size=" + state.usersSize + "&q=" + encodeURIComponent(state.usersKeyword);
        var result = await getJson(url);
        if (isAuthFailure(result)) {
            redirectToLogin();
            return;
        }
        if (!result.ok || !result.data.success) {
            content.innerHTML = renderError(result.status === 403 ? "当前账号没有管理权限" : "读取用户列表失败");
            return;
        }

        var data = result.data.data || {};
        var rows = data.rows || [];
        var html = [];
        html.push('<div class="page-head"><div><h2>用户管理</h2><div class="page-subtitle">按用户名或邮箱检索用户账号</div></div></div>');
        html.push('<div class="toolbar">');
        html.push('<input id="users-search" type="search" placeholder="按用户名或邮箱搜索" value="' + escapeHtml(state.usersKeyword) + '">');
        html.push('<button class="action" id="users-search-btn">查询</button>');
        html.push('</div>');
        html.push('<table><thead><tr><th>UID</th><th>用户名</th><th>邮箱</th><th>注册时间</th><th>最近登录</th></tr></thead><tbody>');
        for (var i = 0; i < rows.length; i++) {
            var r = rows[i];
            html.push('<tr>');
            html.push('<td>' + escapeHtml(r.uid) + '</td>');
            html.push('<td>' + escapeHtml(r.name) + '</td>');
            html.push('<td>' + escapeHtml(r.email) + '</td>');
            html.push('<td>' + escapeHtml(r.create_time) + '</td>');
            html.push('<td>' + escapeHtml(r.last_login) + '</td>');
            html.push('</tr>');
        }
        if (!rows.length) {
            html.push('<tr><td colspan="5" class="empty">没有匹配的用户</td></tr>');
        }
        html.push('</tbody></table>');
        html.push('<div class="pagination">第 ' + (data.page || 1) + ' 页，每页 ' + (data.size || 20) + ' 条，总计 ' + (data.total_count || 0) + ' 条</div>');
        content.innerHTML = html.join('');

        document.getElementById("users-search-btn").onclick = function () {
            var keywordInput = document.getElementById("users-search");
            var nextKeyword = (keywordInput.value || "").trim();
            navigateTo("/admin/users?page=1&size=" + state.usersSize + (nextKeyword ? "&q=" + encodeURIComponent(nextKeyword) : ""));
        };
    }

    async function renderQuestionsList() {
        var content = document.getElementById("content");
        content.innerHTML = "加载中...";

        readQuestionsStateFromLocation();
        state.lastQuestionsListPath = buildQuestionsListPath();
        var url = "/api/admin/questions?page=" + state.questionsPage + "&size=" + state.questionsSize + "&q=" + encodeURIComponent(state.questionsKeyword);
        var result = await getJson(url);
        if (isAuthFailure(result)) {
            redirectToLogin();
            return;
        }
        if (!result.ok || !result.data.success) {
            content.innerHTML = renderError(result.status === 403 ? "当前账号没有管理权限" : "读取题目列表失败");
            return;
        }

        var data = result.data.data || {};
        var rows = data.rows || [];
        var html = [];
        html.push('<div class="page-head"><div><h2>题目列表</h2><div class="page-subtitle">列表页只负责检索、跳转编辑和删除</div></div><div class="actions-inline"><button class="action" id="questions-new-btn">新增题目</button></div></div>');
        html.push('<div class="toolbar">');
        html.push('<input id="questions-search" type="search" placeholder="按题号或标题搜索" value="' + escapeHtml(state.questionsKeyword) + '">');
        html.push('<button class="action" id="questions-search-btn">查询</button>');
        html.push('</div>');
        html.push('<table><thead><tr><th>题号</th><th>标题</th><th>难度</th><th>时限</th><th>内存</th><th>操作</th></tr></thead><tbody>');
        for (var i = 0; i < rows.length; i++) {
            var q = rows[i];
            html.push('<tr>');
            html.push('<td>' + escapeHtml(q.number) + '</td>');
            html.push('<td>' + escapeHtml(q.title) + '</td>');
            html.push('<td>' + escapeHtml(q.star) + '</td>');
            html.push('<td>' + escapeHtml(q.cpu_limit) + '</td>');
            html.push('<td>' + escapeHtml(q.memory_limit) + '</td>');
            html.push('<td><div class="actions-inline"><button class="action questions-edit-btn" data-number="' + escapeHtml(q.number) + '">编辑</button><button class="ghost questions-delete-btn" data-number="' + escapeHtml(q.number) + '">删除</button></div></td>');
            html.push('</tr>');
        }
        if (!rows.length) {
            html.push('<tr><td colspan="6" class="empty">没有匹配的题目</td></tr>');
        }
        html.push('</tbody></table>');
        html.push('<div class="pagination">第 ' + (data.page || 1) + ' 页，每页 ' + (data.size || 20) + ' 条，总计 ' + (data.total_count || 0) + ' 条</div>');
        content.innerHTML = html.join('');

        document.getElementById("questions-search-btn").onclick = function () {
            var keywordInput = document.getElementById("questions-search");
            navigateTo(buildQuestionsListPath({ page: 1, keyword: (keywordInput.value || "").trim() }));
        };

        document.getElementById("questions-new-btn").onclick = function () {
            state.lastQuestionsListPath = buildQuestionsListPath();
            navigateTo(questionEditorPath(""));
        };

        var editBtns = document.querySelectorAll(".questions-edit-btn");
        for (var k = 0; k < editBtns.length; k++) {
            editBtns[k].onclick = function () {
                state.lastQuestionsListPath = buildQuestionsListPath();
                navigateTo(questionEditorPath(this.getAttribute("data-number")));
            };
        }

        var deleteBtns = document.querySelectorAll(".questions-delete-btn");
        for (var j = 0; j < deleteBtns.length; j++) {
            deleteBtns[j].onclick = async function () {
                var number = this.getAttribute("data-number");
                if (!window.confirm("确认删除题目 #" + number + " ?")) {
                    return;
                }
                var resultDelete = await deleteJson("/api/admin/questions/" + encodeURIComponent(number));
                if (isAuthFailure(resultDelete)) {
                    redirectToLogin();
                    return;
                }
                if (!resultDelete.ok || !resultDelete.data.success) {
                    window.alert("删除失败: " + ((resultDelete.data && (resultDelete.data.message || resultDelete.data.error_code)) || "未知错误"));
                    return;
                }
                renderQuestionsList();
            };
        }
    }

    async function renderQuestionEditor(number) {
        var content = document.getElementById("content");
        var isEditMode = !!number;
        var returnPath = getQuestionReturnPath();
        var data = {};

        content.innerHTML = "加载中...";

        if (isEditMode) {
            var detailResult = await getJson("/api/admin/questions/" + encodeURIComponent(number));
            if (isAuthFailure(detailResult)) {
                redirectToLogin();
                return;
            }
            if (!detailResult.ok || !detailResult.data.success) {
                content.innerHTML = renderError("读取题目详情失败");
                return;
            }
            data = detailResult.data.data || {};
        }

        content.innerHTML = [
            '<div class="page-head"><div><h2>' + (isEditMode ? ('编辑题目 #' + escapeHtml(number)) : '新增题目') + '</h2><div class="page-subtitle">编辑页只负责单题维护，保存后可返回列表</div></div><div class="actions-inline"><button class="ghost" id="questions-back-btn">返回列表</button></div></div>',
            '<section class="panel">',
            '<div class="form-grid">',
            '<div class="field-stack"><label for="q-number">题号</label><input id="q-number" type="text" placeholder="题号(数字)" value="' + escapeHtml(data.number || '') + '" ' + (isEditMode ? 'disabled' : '') + '></div>',
            '<div class="field-stack"><label for="q-title">标题</label><input id="q-title" type="text" placeholder="标题" value="' + escapeHtml(data.title || '') + '"></div>',
            '<div class="field-stack"><label for="q-star">难度</label><input id="q-star" type="text" placeholder="简单/中等/困难" value="' + escapeHtml(data.star || '') + '"></div>',
            '<div class="field-stack"><label for="q-cpu">CPU限制</label><input id="q-cpu" type="number" min="1" placeholder="CPU限制" value="' + escapeHtml(data.cpu_limit || '') + '"></div>',
            '<div class="field-stack"><label for="q-mem">内存限制</label><input id="q-mem" type="number" min="1" placeholder="内存限制" value="' + escapeHtml(data.memory_limit || '') + '"></div>',
            '</div>',
            '<div class="field-stack"><label for="q-desc">题目描述</label><textarea id="q-desc" placeholder="题目描述">' + escapeHtml(data.desc || '') + '</textarea></div>',
            '<div class="actions-row"><div id="q-op-msg" class="muted"></div><div class="actions-inline"><button class="action" id="questions-save-btn">' + (isEditMode ? '更新题目' : '保存题目') + '</button></div></div>',
            '</section>'
        ].join('');

        document.getElementById("questions-back-btn").onclick = function () {
            navigateTo(returnPath);
        };

        document.getElementById("questions-save-btn").onclick = async function () {
            var opMsg = document.getElementById("q-op-msg");
            var payload = {
                number: (document.getElementById("q-number").value || "").trim(),
                title: (document.getElementById("q-title").value || "").trim(),
                star: (document.getElementById("q-star").value || "").trim(),
                cpu_limit: Number(document.getElementById("q-cpu").value || 0),
                memory_limit: Number(document.getElementById("q-mem").value || 0),
                desc: document.getElementById("q-desc").value || ""
            };

            if (!/^\d+$/.test(payload.number)) {
                opMsg.textContent = "题号必须是纯数字";
                opMsg.className = "muted status-failed";
                return;
            }
            if (!payload.title) {
                opMsg.textContent = "标题不能为空";
                opMsg.className = "muted status-failed";
                return;
            }

            var resultSave = isEditMode
                ? await putJson("/api/admin/questions/" + encodeURIComponent(number), payload)
                : await postJson("/api/admin/questions", payload);

            if (isAuthFailure(resultSave)) {
                redirectToLogin();
                return;
            }
            if (!resultSave.ok || !resultSave.data.success) {
                opMsg.textContent = "保存失败: " + ((resultSave.data && (resultSave.data.message || resultSave.data.error_code)) || "未知错误");
                opMsg.className = "muted status-failed";
                return;
            }

            opMsg.textContent = isEditMode ? "更新成功，正在返回列表..." : "保存成功，正在进入编辑页...";
            opMsg.className = "muted status-success";
            if (isEditMode) {
                navigateTo(returnPath);
            } else {
                navigateTo(questionEditorPath(payload.number));
            }
        };
    }

    async function renderCurrentRoute() {
        var pathname = window.location.pathname;
        updateNavActive(pathname);

        if (pathname === "/admin") {
            renderOverview();
            return;
        }
        if (pathname === "/admin/users") {
            renderUsers();
            return;
        }
        if (pathname === "/admin/questions") {
            renderQuestionsList();
            return;
        }
        if (pathname === "/admin/questions/new") {
            renderQuestionEditor("");
            return;
        }

        var match = pathname.match(/^\/admin\/questions\/(\d+)$/);
        if (match) {
            renderQuestionEditor(match[1]);
            return;
        }

        navigateTo("/admin", true);
    }

    function bindEvents() {
        var navs = document.querySelectorAll(".nav-btn[data-path]");
        for (var i = 0; i < navs.length; i++) {
            navs[i].onclick = function () {
                navigateTo(this.getAttribute("data-path"));
            };
        }

        window.addEventListener("popstate", renderCurrentRoute);

        var logoutBtn = document.getElementById("logout-btn");
        logoutBtn.onclick = async function () {
            await postJson("/api/admin/auth/logout", {});
            redirectToLogin();
        };
    }

    function init() {
        setUserHint();
        bindEvents();
        renderCurrentRoute();
    }

    document.addEventListener("DOMContentLoaded", init);
})();

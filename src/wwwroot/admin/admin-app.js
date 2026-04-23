(function () {
    var state = {
        tab: "overview",
        usersPage: 1,
        usersSize: 20,
        usersKeyword: "",
        questionsPage: 1,
        questionsSize: 20,
        questionsKeyword: ""
    };

    function escapeHtml(v) {
        return String(v || "")
            .replace(/&/g, "&amp;")
            .replace(/</g, "&lt;")
            .replace(/>/g, "&gt;")
            .replace(/\"/g, "&quot;")
            .replace(/'/g, "&#39;");
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

    function renderError(message) {
        return '<div class="error">' + escapeHtml(message) + '</div>';
    }

    function setUserHint() {
        var userEl = document.getElementById("admin-user");
        var user = window.SERVER_USER_INFO || {};
        if (user.isLoggedIn) {
            userEl.textContent = "管理员: " + (user.name || user.email || "Unknown") + " (uid=" + (user.uid || "-") + ")";
        } else {
            userEl.textContent = "未登录";
        }
    }

    async function renderOverview() {
        var content = document.getElementById("content");
        content.innerHTML = "加载中...";

        var result = await getJson("/api/admin/overview");
        if (!result.ok || !result.data.success) {
            if (result.status === 401) {
                content.innerHTML = renderError("请先登录，再访问管理后台");
                return;
            }
            if (result.status === 403) {
                content.innerHTML = renderError("当前账号没有管理权限（临时规则：uid=1）");
                return;
            }
            content.innerHTML = renderError("读取总览失败");
            return;
        }

        var data = result.data.data || {};
        var cache = data.cache || {};
        content.innerHTML = [
            '<div class="cards">',
            '<div class="card"><div class="label">题目总数</div><div class="value">' + (data.question_count || 0) + '</div></div>',
            '<div class="card"><div class="label">用户总数</div><div class="value">' + (data.user_count || 0) + '</div></div>',
            '<div class="card"><div class="label">列表缓存命中/请求</div><div class="value">' + (cache.list_hits || 0) + ' / ' + (cache.list_requests || 0) + '</div></div>',
            '<div class="card"><div class="label">详情缓存命中/请求</div><div class="value">' + (cache.detail_hits || 0) + ' / ' + (cache.detail_requests || 0) + '</div></div>',
            '</div>',
            '<div>这是第一批后台骨架，后续将扩展提交记录、题解审核、系统配置和操作日志。</div>'
        ].join("");
    }

    async function renderUsers() {
        var content = document.getElementById("content");
        content.innerHTML = "加载中...";

        var url = "/api/admin/users?page=" + state.usersPage + "&size=" + state.usersSize + "&q=" + encodeURIComponent(state.usersKeyword);
        var result = await getJson(url);
        if (!result.ok || !result.data.success) {
            content.innerHTML = renderError(result.status === 403 ? "当前账号没有管理权限" : "读取用户列表失败");
            return;
        }

        var data = result.data.data || {};
        var rows = data.rows || [];
        var html = [];
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
        html.push('</tbody></table>');
        html.push('<div class="pagination">第 ' + (data.page || 1) + ' 页，每页 ' + (data.size || 20) + ' 条，总计 ' + (data.total_count || 0) + ' 条</div>');
        content.innerHTML = html.join('');

        var searchBtn = document.getElementById("users-search-btn");
        searchBtn.onclick = function () {
            var keywordInput = document.getElementById("users-search");
            state.usersKeyword = (keywordInput.value || "").trim();
            state.usersPage = 1;
            renderUsers();
        };
    }

    async function renderQuestions() {
        var content = document.getElementById("content");
        content.innerHTML = "加载中...";

        var url = "/api/admin/questions?page=" + state.questionsPage + "&size=" + state.questionsSize + "&q=" + encodeURIComponent(state.questionsKeyword);
        var result = await getJson(url);
        if (!result.ok || !result.data.success) {
            content.innerHTML = renderError(result.status === 403 ? "当前账号没有管理权限" : "读取题目列表失败");
            return;
        }

        var data = result.data.data || {};
        var rows = data.rows || [];
        var html = [];
        html.push('<div class="toolbar">');
        html.push('<input id="questions-search" type="search" placeholder="按题号或标题搜索" value="' + escapeHtml(state.questionsKeyword) + '">');
        html.push('<button class="action" id="questions-search-btn">查询</button>');
        html.push('</div>');
        html.push('<table><thead><tr><th>题号</th><th>标题</th><th>难度</th><th>时限</th><th>内存</th></tr></thead><tbody>');
        for (var i = 0; i < rows.length; i++) {
            var q = rows[i];
            html.push('<tr>');
            html.push('<td>' + escapeHtml(q.number) + '</td>');
            html.push('<td>' + escapeHtml(q.title) + '</td>');
            html.push('<td>' + escapeHtml(q.star) + '</td>');
            html.push('<td>' + escapeHtml(q.cpu_limit) + '</td>');
            html.push('<td>' + escapeHtml(q.memory_limit) + '</td>');
            html.push('</tr>');
        }
        html.push('</tbody></table>');
        html.push('<div class="pagination">第 ' + (data.page || 1) + ' 页，每页 ' + (data.size || 20) + ' 条，总计 ' + (data.total_count || 0) + ' 条</div>');
        content.innerHTML = html.join('');

        var searchBtn = document.getElementById("questions-search-btn");
        searchBtn.onclick = function () {
            var keywordInput = document.getElementById("questions-search");
            state.questionsKeyword = (keywordInput.value || "").trim();
            state.questionsPage = 1;
            renderQuestions();
        };
    }

    function activateTab(tab) {
        state.tab = tab;
        var navs = document.querySelectorAll(".nav-btn");
        for (var i = 0; i < navs.length; i++) {
            navs[i].classList.toggle("active", navs[i].dataset.tab === tab);
        }
        if (tab === "overview") {
            renderOverview();
            return;
        }
        if (tab === "users") {
            renderUsers();
            return;
        }
        if (tab === "questions") {
            renderQuestions();
            return;
        }
    }

    function bindEvents() {
        var navs = document.querySelectorAll(".nav-btn");
        for (var i = 0; i < navs.length; i++) {
            navs[i].onclick = function () {
                activateTab(this.dataset.tab);
            };
        }

        var logoutBtn = document.getElementById("logout-btn");
        logoutBtn.onclick = async function () {
            await postJson("/api/auth/logout", {});
            window.location.href = "/";
        };
    }

    function init() {
        setUserHint();
        bindEvents();
        activateTab("overview");
    }

    init();
})();

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
        var resp = await OJApi.fetch(url, { credentials: "include" });
        var data = {};
        try { data = await resp.json(); } catch (e) { data = { success: false, error_code: "BAD_JSON" }; }
        return { ok: resp.ok, status: resp.status, data: data };
    }

    async function postJson(url, body) {
        var resp = await OJApi.fetch(url, {
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
        var resp = await OJApi.fetch(url, {
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
        var resp = await OJApi.fetch(url, {
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
            '<div class="metric"><div class="k">题目总数</div><div class="v">' + (data.total_questions || 0) + '</div><div class="s">题库规模</div></div>',
            '<div class="metric"><div class="k">用户总数</div><div class="v">' + (data.total_users || 0) + '</div><div class="s">已注册账户</div></div>',
            '<div class="metric"><div class="k">题解总数</div><div class="v">' + (data.total_solutions || 0) + '</div><div class="s">公开题解</div></div>',
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

        var result = await getJson("/admin/api/overview");
        if (isAuthFailure(result)) {
            redirectToLogin();
            return;
        }
        if (!result.ok || !result.data.success) {
            content.innerHTML = renderError(result.status === 403 ? "当前账号没有管理权限" : "读取总览失败");
            return;
        }

        var data = result.data || {};

        content.innerHTML = [
            '<div class="page-head"><div><h2>仪表盘</h2><div class="page-subtitle">服务数据与运行诊断</div></div></div>',
            renderOverviewCards(data),
            '<div class="diagnostic-grid">',
            diagnosticPanel('log', '服务日志', '查看 log/ 下各服务日志的尾部 2000 行'),
            diagnosticPanel('latency', '延迟记录', '查看 log/latency/ 下的 CSV 延迟记录'),
            '</div>'
        ].join('');
        initializeDiagnostic('log');
        initializeDiagnostic('latency');
    }

    function diagnosticPanel(kind, title, subtitle) {
        return '<section class="panel diagnostic-panel">' +
            '<div class="diagnostic-head"><div><h3>' + title + '</h3><div class="page-subtitle">' + subtitle + '</div></div>' +
            '<button class="action" id="diagnostic-refresh-' + kind + '">刷新</button></div>' +
            '<div class="diagnostic-files" id="diagnostic-files-' + kind + '">加载中...</div>' +
            '<pre class="diagnostic-content" id="diagnostic-content-' + kind + '">请选择文件</pre>' +
            '</section>';
    }

    async function loadDiagnosticContent(kind, file) {
        var output = document.getElementById('diagnostic-content-' + kind);
        if (!output || !file) return;
        output.textContent = '加载中...';
        var result = await getJson('/admin/api/diagnostics/content?kind=' + encodeURIComponent(kind) + '&file=' + encodeURIComponent(file));
        if (!result.ok || !result.data.success) {
            output.textContent = '读取失败: ' + (result.data.error_code || result.data.message || result.status);
            return;
        }
        output.textContent = result.data.content || '(空文件)';
    }

    async function initializeDiagnostic(kind) {
        var list = document.getElementById('diagnostic-files-' + kind);
        if (!list) return;
        var result = await getJson('/admin/api/diagnostics/files?kind=' + encodeURIComponent(kind));
        if (!result.ok || !result.data.success) {
            list.textContent = '文件列表读取失败';
            return;
        }
        var files = result.data.files || [];
        list.innerHTML = files.length ? files.map(function(file, index) {
            return '<label><input type="radio" name="diagnostic-' + kind + '" value="' + escapeHtml(file) + '"' + (index === 0 ? ' checked' : '') + '> ' + escapeHtml(file) + '</label>';
        }).join('') : '<span class="empty">暂无文件</span>';
        list.onchange = function(event) {
            if (event.target && event.target.type === 'radio') loadDiagnosticContent(kind, event.target.value);
        };
        var refresh = document.getElementById('diagnostic-refresh-' + kind);
        if (refresh) refresh.onclick = function() {
            var selected = list.querySelector('input[type="radio"]:checked');
            if (selected) loadDiagnosticContent(kind, selected.value);
        };
        if (files.length) loadDiagnosticContent(kind, files[0]);
    }

    async function renderUsers() {
        var content = document.getElementById("content");
        content.innerHTML = "加载中...";

        readUsersStateFromLocation();
        var url = "/admin/api/users?page=" + state.usersPage + "&page_size=" + state.usersSize + "&search=" + encodeURIComponent(state.usersKeyword);
        var result = await getJson(url);
        if (isAuthFailure(result)) {
            redirectToLogin();
            return;
        }
        if (!result.ok || !result.data.success) {
            content.innerHTML = renderError(result.status === 403 ? "当前账号没有管理权限" : "读取用户列表失败");
            return;
        }

        var data = result.data || {};
        var rows = data.users || [];
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
        html.push('<div class="pagination">第 ' + (data.page_number || 1) + ' 页，每页 ' + (data.page_size || 20) + ' 条，总计 ' + (data.total || 0) + ' 条</div>');
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
        var url = "/admin/api/questions?page=" + state.questionsPage + "&page_size=" + state.questionsSize + "&search=" + encodeURIComponent(state.questionsKeyword);
        var result = await getJson(url);
        if (isAuthFailure(result)) {
            redirectToLogin();
            return;
        }
        if (!result.ok || !result.data.success) {
            content.innerHTML = renderError(result.status === 403 ? "当前账号没有管理权限" : "读取题目列表失败");
            return;
        }

        var data = result.data || {};
        var rows = data.questions || [];
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
        html.push('<div class="pagination">第 ' + (data.page_number || 1) + ' 页，每页 ' + (data.page_size || 20) + ' 条，总计 ' + (data.total || 0) + ' 条</div>');
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
                var resultDelete = await deleteJson("/admin/api/questions/" + encodeURIComponent(number));
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
            var detailResult = await getJson("/admin/api/questions/" + encodeURIComponent(number));
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
            (isEditMode ? '<div class="toolbar" style="margin-bottom:10px;"><button class="action q-tab-btn active" data-tab="info">编辑基本信息</button><button class="ghost q-tab-btn" data-tab="tests">编辑测试用例</button></div>' : ''),
            '<section class="panel q-tab-panel" id="q-tab-info">',
            '<div class="form-grid">',
            '<div class="field-stack"><label for="q-number">题号</label><input id="q-number" type="text" placeholder="题号(数字)" value="' + escapeHtml(data.number || '') + '" ' + (isEditMode ? 'disabled' : '') + '></div>',
            '<div class="field-stack"><label for="q-title">标题</label><input id="q-title" type="text" placeholder="标题" value="' + escapeHtml(data.title || '') + '"></div>',
            '<div class="field-stack"><label for="q-star">难度</label><input id="q-star" type="text" placeholder="简单/中等/困难" value="' + escapeHtml(data.star || '') + '"></div>',
            '<div class="field-stack"><label for="q-cpu">CPU限制</label><input id="q-cpu" type="number" min="1" placeholder="CPU限制" value="' + escapeHtml(data.cpu_limit || '') + '"></div>',
            '<div class="field-stack"><label for="q-mem">内存限制</label><input id="q-mem" type="number" min="1" placeholder="内存限制" value="' + escapeHtml(data.memory_limit || '') + '"></div>',
            '</div>',
            '<div class="field-stack"><label for="q-desc">题目描述</label><textarea id="q-desc" placeholder="题目描述">' + escapeHtml(data.desc || '') + '</textarea></div>',
            '<div class="actions-row"><div id="q-op-msg" class="muted"></div><div class="actions-inline"><button class="action" id="questions-save-btn">' + (isEditMode ? '更新题目' : '保存题目') + '</button></div></div>',
            '</section>',
            (isEditMode ? '<section class="panel q-tab-panel" id="q-tab-tests" style="display:none;"><div class="toolbar"><button class="action" id="tests-add-btn">新增测试用例</button></div><div class="test-table-wrap"><table><thead><tr><th>ID</th><th>输入</th><th>输出</th><th>样例</th><th>操作</th></tr></thead><tbody id="tests-tbody"><tr><td colspan="5" class="empty">加载中...</td></tr></tbody></table></div><div id="test-edit-form" class="panel" style="display:none; margin-top:12px;"><h4 id="test-edit-title">编辑测试用例</h4><div class="field-stack"><label>输入</label><textarea id="test-input" rows="4" placeholder="测试用例输入"></textarea></div><div class="field-stack"><label>输出</label><textarea id="test-output" rows="4" placeholder="测试用例输出"></textarea></div><div class="field-stack"><label><input type="checkbox" id="test-is-sample"> 作为样例数据</label></div><input type="hidden" id="test-form-test-id" value=""><div class="actions-row"><div></div><div class="actions-inline"><button class="ghost" id="test-cancel-btn">取消</button><button class="action" id="test-save-btn">保存测试用例</button></div></div></div></section>' : '')
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
                ? await putJson("/admin/api/questions/" + encodeURIComponent(number), payload)
                : await postJson("/admin/api/questions", payload);

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

        if (isEditMode) {
            var tabBtns = document.querySelectorAll(".q-tab-btn");
            for (var t = 0; t < tabBtns.length; t++) {
                tabBtns[t].onclick = function () {
                    var tabName = this.getAttribute("data-tab");
                    var allTabs = document.querySelectorAll(".q-tab-btn");
                    var allPanels = document.querySelectorAll(".q-tab-panel");
                    for (var a = 0; a < allTabs.length; a++) {
                        allTabs[a].classList.toggle("active", allTabs[a].getAttribute("data-tab") === tabName);
                    }
                    for (var b = 0; b < allPanels.length; b++) {
                        allPanels[b].style.display = allPanels[b].id === "q-tab-" + tabName ? "" : "none";
                    }
                    if (tabName === "tests") refreshTests(number);
                };
            }

            var addBtn = document.getElementById("tests-add-btn");
            if (addBtn) addBtn.onclick = function () { showTestEditForm(number, null); };

            var saveBtn = document.getElementById("test-save-btn");
            if (saveBtn) saveBtn.onclick = function () { saveTest(number); };

            var cancelBtn = document.getElementById("test-cancel-btn");
            if (cancelBtn) cancelBtn.onclick = hideTestEditForm;

            refreshTests(number);
        }
    }

    async function loadTests(questionId) {
        var result = await getJson("/admin/api/questions/" + encodeURIComponent(questionId) + "/test-cases");
        if (isAuthFailure(result)) { redirectToLogin(); return []; }
        if (!result.ok || !result.data.success) { return []; }
        return result.data.tests || [];
    }

    function renderTests(tests, questionId) {
        var tbody = document.getElementById("tests-tbody");
        if (!tbody) return;

        if (!tests || !tests.length) {
            tbody.innerHTML = '<tr><td colspan="5" class="empty">暂无测试用例</td></tr>';
            return;
        }

        var rows = [];
        for (var i = 0; i < tests.length; i++) {
            var t = tests[i];
            var previewIn = String(t["in"] || t.input || "").substring(0, 50);
            var previewOut = String(t["out"] || t.output || "").substring(0, 50);
            if (String(t["in"] || t.input || "").length > 50) previewIn += "...";
            if (String(t["out"] || t.output || "").length > 50) previewOut += "...";
            rows.push('<tr>');
            rows.push('<td>' + escapeHtml(t.id) + '</td>');
            rows.push('<td><code>' + escapeHtml(previewIn) + '</code></td>');
            rows.push('<td><code>' + escapeHtml(previewOut) + '</code></td>');
            rows.push('<td>' + (t.is_sample ? '<span class="pill">样例</span>' : '-') + '</td>');
            rows.push('<td><div class="actions-inline"><button class="action test-edit-btn" data-test-id="' + t.id + '">编辑</button><button class="ghost test-delete-btn" data-test-id="' + t.id + '">删除</button></div></td>');
            rows.push('</tr>');
        }
        tbody.innerHTML = rows.join('');

        var editBtns = tbody.querySelectorAll(".test-edit-btn");
        for (var j = 0; j < editBtns.length; j++) {
            editBtns[j].onclick = function () {
                var testId = this.getAttribute("data-test-id");
                var testData = null;
                for (var k = 0; k < tests.length; k++) {
                    if (String(tests[k].id) === testId) { testData = tests[k]; break; }
                }
                if (testData) showTestEditForm(questionId, testData);
            };
        }

        var deleteBtns = tbody.querySelectorAll(".test-delete-btn");
        for (var m = 0; m < deleteBtns.length; m++) {
            deleteBtns[m].onclick = function () {
                deleteTest(this.getAttribute("data-test-id"), questionId);
            };
        }
    }

    function showTestEditForm(questionId, testData) {
        var form = document.getElementById("test-edit-form");
        if (!form) return;
        form.style.display = "block";
        var titleEl = document.getElementById("test-edit-title");
        if (titleEl) titleEl.textContent = testData ? "编辑测试用例 #" + testData.id : "新增测试用例";
        var inputEl = document.getElementById("test-input");
        if (inputEl) inputEl.value = testData ? (testData["in"] || testData.input || "") : "";
        var outputEl = document.getElementById("test-output");
        if (outputEl) outputEl.value = testData ? (testData["out"] || testData.output || "") : "";
        var sampleEl = document.getElementById("test-is-sample");
        if (sampleEl) sampleEl.checked = testData ? (testData.is_sample || false) : false;
        var testIdEl = document.getElementById("test-form-test-id");
        if (testIdEl) testIdEl.value = testData ? testData.id : "";
    }

    function hideTestEditForm() {
        var form = document.getElementById("test-edit-form");
        if (form) form.style.display = "none";
    }

    async function deleteTest(testId, questionId) {
        if (!window.confirm("确认删除测试用例 #" + testId + " ?")) return;
        var result = await deleteJson("/admin/api/test-cases/" + testId);
        if (isAuthFailure(result)) { redirectToLogin(); return; }
        if (!result.ok || !result.data.success) {
            window.alert("删除失败: " + ((result.data && result.data.error) || "未知错误"));
            return;
        }
        refreshTests(questionId);
    }

    async function saveTest(questionId) {
        var testIdEl = document.getElementById("test-form-test-id");
        var testId = testIdEl ? testIdEl.value : "";
        var payload = {
            "in": (document.getElementById("test-input") || {}).value || "",
            "out": (document.getElementById("test-output") || {}).value || "",
            is_sample: (document.getElementById("test-is-sample") || {}).checked || false
        };

        var result;
        if (testId) {
            result = await putJson("/admin/api/test-cases/" + testId, payload);
        } else {
            var existingTests = await loadTests(questionId);
            payload.ordinal = (existingTests ? existingTests.length : 0) + 1;
            result = await postJson("/admin/api/questions/" + encodeURIComponent(questionId) + "/test-cases", payload);
        }

        if (isAuthFailure(result)) { redirectToLogin(); return; }
        if (!result.ok || !result.data.success) {
            window.alert("保存失败: " + ((result.data && result.data.error) || "未知错误"));
            return;
        }
        hideTestEditForm();
        refreshTests(questionId);
    }

    async function refreshTests(questionId) {
        var tests = await loadTests(questionId);
        if (tests !== null) renderTests(tests, questionId);
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

        var match = pathname.match(/^\/admin\/questions\/([^/]+)$/);
        if (match) {
            renderQuestionEditor(decodeURIComponent(match[1]));
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
            await postJson("/admin/api/auth/logout", {});
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

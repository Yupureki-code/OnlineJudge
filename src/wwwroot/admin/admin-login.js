(function () {
    function postJson(url, body) {
        return fetch(url, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            credentials: "include",
            body: JSON.stringify(body || {})
        }).then(async function (resp) {
            var data = {};
            try {
                data = await resp.json();
            } catch (e) {
                data = { success: false, error_code: "BAD_JSON" };
            }
            return { ok: resp.ok, status: resp.status, data: data };
        });
    }

    var idInput = document.getElementById("admin-id") || document.getElementById("login-admin-id");
    var passwordInput = document.getElementById("admin-password") || document.getElementById("login-admin-password");
    var submitBtn = document.getElementById("login-submit") || document.getElementById("login-admin-submit");
    var msg = document.getElementById("login-msg") || document.getElementById("login-admin-msg");

    if (!idInput || !passwordInput || !submitBtn || !msg) {
        return;
    }

    function setMessage(text, type) {
        msg.className = "msg" + (type ? " " + type : "");
        if (msg.classList.contains("login-msg")) {
            msg.className = "login-msg" + (type ? " " + type : "");
        }
        msg.textContent = text;
    }

    async function login() {
        var adminId = Number((idInput.value || "").trim());
        var password = passwordInput.value || "";

        if (!adminId || adminId <= 0) {
            setMessage("请输入合法管理员 ID", "error");
            return;
        }
        if (!password) {
            setMessage("请输入密码", "error");
            return;
        }

        submitBtn.disabled = true;
        setMessage("正在验证身份...", "");

        var result = await postJson("/api/admin/auth/login", {
            admin_id: adminId,
            password: password
        });

        submitBtn.disabled = false;

        if (result.ok && result.data && result.data.success) {
            setMessage("登录成功，正在进入控制台...", "success");
            window.location.href = "/admin";
            return;
        }

        var errMap = {
            INVALID_CREDENTIALS: "管理员 ID 或密码错误",
            LOAD_USER_FAILED: "管理员关联用户信息异常",
            BAD_REQUEST: "请求格式不正确"
        };
        setMessage(errMap[result.data && result.data.error_code] || "登录失败", "error");
    }

    submitBtn.addEventListener("click", login);
    idInput.addEventListener("keydown", function (event) {
        if (event.key === "Enter") {
            passwordInput.focus();
        }
    });
    passwordInput.addEventListener("keydown", function (event) {
        if (event.key === "Enter") {
            login();
        }
    });
})();
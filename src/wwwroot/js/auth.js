// Unified auth script: login (password/code) + register (email code).
(function () {
    const state = {
        currentUser: null,
        sendCodeCooldownTimer: null,
        sendCodeCooldownLeft: 0,
        pendingRouteAfterLogin: ""
    };

    function trim(value) {
        return (value || "").trim();
    }

    function isValidEmail(email) {
        return /^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}$/.test(email);
    }

    function isValidCode(code) {
        return /^\d{6}$/.test(code);
    }

    function isStrongPassword(password) {
        if (!password || password.length < 8 || password.length > 72) {
            return false;
        }
        return /[A-Za-z]/.test(password) && /\d/.test(password);
    }

    function mapSendCodeError(errorCode, status) {
        if (status === 429 || errorCode === "TOO_MANY_REQUESTS") {
            return "请求过于频繁，请稍后再试";
        }
        if (errorCode === "EMAIL_DAILY_LIMIT") {
            return "该邮箱今日发送次数已达上限";
        }
        if (errorCode === "IP_DAILY_LIMIT") {
            return "当前网络今日发送次数已达上限";
        }
        if (errorCode === "MAIL_SEND_FAILED") {
            return "验证码发送失败，请稍后重试";
        }
        return "发送验证码失败，请稍后重试";
    }

    function mapVerifyCodeError(errorCode, status) {
        if (status === 429 || errorCode === "ATTEMPTS_EXCEEDED") {
            return "验证失败次数过多，请重新获取验证码";
        }
        if (errorCode === "CODE_MISMATCH") {
            return "验证码错误，请检查后重试";
        }
        if (errorCode === "CODE_EXPIRED") {
            return "验证码已过期，请重新获取";
        }
        if (errorCode === "REGISTER_NAME_REQUIRED") {
            return "新用户注册请填写用户名";
        }
        if (errorCode === "WEAK_PASSWORD") {
            return "密码至少8位且同时包含字母和数字";
        }
        if (errorCode === "CREATE_USER_FAILED") {
            return "注册失败，请稍后重试";
        }
        if (errorCode === "PASSWORD_HASH_FAILED" || errorCode === "UPDATE_PASSWORD_FAILED") {
            return "密码设置失败，请稍后重试";
        }
        return "验证失败，请稍后重试";
    }

    function mapPasswordLoginError(errorCode) {
        if (errorCode === "USER_NOT_FOUND") {
            return "用户名或邮箱不存在，请确认后重试";
        }
        if (errorCode === "PASSWORD_NOT_SET") {
            return "该账号尚未设置密码，请使用邮箱验证码登录后在个人中心设置密码";
        }
        if (errorCode === "PASSWORD_MISMATCH") {
            return "密码错误";
        }
        return "登录失败，请稍后重试";
    }

    async function getJson(url) {
        const response = await fetch(url, {
            method: "GET",
            credentials: "include"
        });

        let payload = {};
        try {
            payload = await response.json();
        } catch (e) {
            payload = { success: false, error: "响应解析失败" };
        }

        return {
            ok: response.ok,
            status: response.status,
            data: payload
        };
    }

    async function postJson(url, body) {
        const response = await fetch(url, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            credentials: "include",
            body: JSON.stringify(body || {})
        });

        let payload = {};
        try {
            payload = await response.json();
        } catch (e) {
            payload = { success: false, error: "响应解析失败" };
        }

        return {
            ok: response.ok,
            status: response.status,
            data: payload
        };
    }

    async function fetchCurrentUser() {
        if (typeof SERVER_USER_INFO !== "undefined" && SERVER_USER_INFO && SERVER_USER_INFO.isLoggedIn) {
            return {
                name: SERVER_USER_INFO.name || "用户",
                email: SERVER_USER_INFO.email || "",
                create_time: SERVER_USER_INFO.create_time || "",
                avatar_url: SERVER_USER_INFO.avatar_url || ""
            };
        }

        const result = await getJson("/api/user/info");
        if (result.ok && result.data && result.data.success && result.data.user) {
            return result.data.user;
        }

        return null;
    }

    function clearCooldown() {
        if (state.sendCodeCooldownTimer) {
            clearInterval(state.sendCodeCooldownTimer);
            state.sendCodeCooldownTimer = null;
        }
        state.sendCodeCooldownLeft = 0;
    }

    function closeAuthModal() {
        clearCooldown();
        const modal = document.getElementById("auth-modal");
        if (modal) {
            modal.remove();
        }
    }

    function setAuthMessage(message, isError) {
        const msg = document.getElementById("auth-message");
        if (!msg) return;
        msg.style.color = isError ? "#ffb0b0" : "#b8f3be";
        msg.textContent = message || "";
    }

    function setRegMessage(message, isError) {
        const msg = document.getElementById("reg-message");
        if (!msg) return;
        msg.style.color = isError ? "#ffb0b0" : "#b8f3be";
        msg.textContent = message || "";
    }

    function setAuthBusy(isBusy) {
        const btns = document.querySelectorAll("#auth-modal button");
        btns.forEach(function (b) {
            if (b.id === "auth-close") return;
            b.disabled = isBusy;
        });
        updateSendCodeButton();
    }

    function updateSendCodeButtons() {
        const sendBtns = document.querySelectorAll(".auth-send-code-btn");
        sendBtns.forEach(function (btn) {
            if (state.sendCodeCooldownLeft > 0) {
                btn.disabled = true;
                btn.textContent = "重新发送(" + state.sendCodeCooldownLeft + "s)";
            } else {
                btn.disabled = false;
                btn.textContent = "发送验证码";
            }
        });
    }

    function updateSendCodeButton() {
        updateSendCodeButtons();
    }

    function startCooldown(seconds) {
        clearCooldown();
        state.sendCodeCooldownLeft = Math.max(0, Number(seconds) || 0);
        updateSendCodeButtons();

        if (state.sendCodeCooldownLeft <= 0) {
            return;
        }

        state.sendCodeCooldownTimer = setInterval(function () {
            state.sendCodeCooldownLeft -= 1;
            if (state.sendCodeCooldownLeft <= 0) {
                clearCooldown();
            }
            updateSendCodeButtons();
        }, 1000);
    }

    function switchAuthMainTab(tabName) {
        const tabs = document.querySelectorAll(".auth-main-tab-btn");
        tabs.forEach(function (btn) {
            btn.classList.toggle("active", btn.dataset.tab === tabName);
            btn.style.opacity = btn.classList.contains("active") ? "1" : "0.7";
        });

        const loginPanel = document.getElementById("auth-login-panel");
        const regPanel = document.getElementById("auth-register-panel");
        if (loginPanel) loginPanel.style.display = tabName === "login" ? "flex" : "none";
        if (regPanel) regPanel.style.display = tabName === "register" ? "flex" : "none";

        setAuthMessage("", false);
        setRegMessage("", false);
    }

    function switchLoginMode(modeName) {
        const passwordPanel = document.getElementById("login-password-panel");
        const codePanel = document.getElementById("login-code-panel");
        if (passwordPanel) passwordPanel.style.display = modeName === "password" ? "flex" : "none";
        if (codePanel) codePanel.style.display = modeName === "code" ? "flex" : "none";
        setAuthMessage("", false);
    }

    function buildAuthModal() {
        closeAuthModal();

        const modal = document.createElement("div");
        modal.id = "auth-modal";
        modal.style.cssText = [
            "position: fixed",
            "inset: 0",
            "z-index: 2147483646",
            "display: flex",
            "align-items: center",
            "justify-content: center",
            "background: rgba(0,0,0,0.72)",
            "backdrop-filter: blur(4px)"
        ].join(";");

        const panel = document.createElement("div");
        panel.style.cssText = [
            "width: min(92vw, 480px)",
            "position: relative",
            "z-index: 2147483647",
            "padding: 24px",
            "border-radius: 14px",
            "background: rgba(40, 52, 40, 0.96)",
            "border: 1px solid rgba(244, 246, 240, 0.2)",
            "box-shadow: 0 10px 40px rgba(0,0,0,0.35)",
            "color: #F4F6F0"
        ].join(";");

        panel.innerHTML = [
            '<div style="display:flex;gap:0;margin-bottom:16px;">',
            '<button class="auth-main-tab-btn active" data-tab="login" style="flex:1;padding:10px;border-radius:8px 0 0 8px;border:1px solid rgba(244,246,240,0.2);background:rgba(255,255,255,0.12);color:#F4F6F0;cursor:pointer;font-size:14px;">登录</button>',
            '<button class="auth-main-tab-btn" data-tab="register" style="flex:1;padding:10px;border-radius:0 8px 8px 0;border:1px solid rgba(244,246,240,0.2);border-left:none;background:rgba(255,255,255,0.06);color:#F4F6F0;cursor:pointer;opacity:0.7;font-size:14px;">注册</button>',
            '</div>',

            // ==== 登录面板 ====
            '<div id="auth-login-panel" style="display:flex;flex-direction:column;gap:10px;">',

            // 用户名/邮箱 + 密码模式
            '<div id="login-password-panel" style="display:flex;flex-direction:column;gap:10px;">',
            '<input id="login-username" type="text" placeholder="用户名 / 邮箱" style="padding:10px 12px;border-radius:8px;border:1px solid rgba(244,246,240,0.25);background:rgba(255,255,255,0.06);color:#F4F6F0;" />',
            '<input id="login-password" type="password" placeholder="密码" style="padding:10px 12px;border-radius:8px;border:1px solid rgba(244,246,240,0.25);background:rgba(255,255,255,0.06);color:#F4F6F0;" />',
            '<button id="login-password-btn" style="padding:10px 12px;border-radius:8px;border:none;background:#2d7cf0;color:#fff;cursor:pointer;">登录</button>',
            '<div style="font-size:12px;color:rgba(244,246,240,0.65);text-align:center;cursor:pointer;text-decoration:underline;" id="switch-to-code-login">邮箱验证码登陆</div>',
            '</div>',

            // 邮箱 + 验证码模式
            '<div id="login-code-panel" style="display:none;flex-direction:column;gap:10px;">',
            '<input id="login-code-email" type="email" placeholder="邮箱地址" style="padding:10px 12px;border-radius:8px;border:1px solid rgba(244,246,240,0.25);background:rgba(255,255,255,0.06);color:#F4F6F0;" />',
            '<div style="display:flex;gap:8px;">',
            '<input id="login-code-value" type="text" maxlength="6" inputmode="numeric" placeholder="请输入6位验证码" style="flex:1;padding:10px 12px;border-radius:8px;border:1px solid rgba(244,246,240,0.25);background:rgba(255,255,255,0.06);color:#F4F6F0;" />',
            '<button class="auth-send-code-btn" data-target="login" style="padding:10px 12px;border-radius:8px;border:none;background:#2d7cf0;color:#fff;cursor:pointer;white-space:nowrap;">发送验证码</button>',
            '</div>',
            '<button id="login-code-btn" style="padding:10px 12px;border-radius:8px;border:none;background:#4caf50;color:#fff;cursor:pointer;">验证码登录</button>',
            '<div style="font-size:12px;color:rgba(244,246,240,0.65);text-align:center;cursor:pointer;text-decoration:underline;" id="switch-to-password-login">用户名/邮箱登陆</div>',
            '</div>',

            '<div id="auth-message" style="min-height:18px;font-size:12px;color:#b8f3be;"></div>',
            '</div>',

            // ==== 注册面板 ====
            '<div id="auth-register-panel" style="display:none;flex-direction:column;gap:10px;">',
            '<input id="reg-email" type="email" placeholder="邮箱地址" style="padding:10px 12px;border-radius:8px;border:1px solid rgba(244,246,240,0.25);background:rgba(255,255,255,0.06);color:#F4F6F0;" />',
            '<div style="display:flex;gap:8px;">',
            '<input id="reg-code-value" type="text" maxlength="6" inputmode="numeric" placeholder="请输入6位验证码" style="flex:1;padding:10px 12px;border-radius:8px;border:1px solid rgba(244,246,240,0.25);background:rgba(255,255,255,0.06);color:#F4F6F0;" />',
            '<button class="auth-send-code-btn" data-target="reg" style="padding:10px 12px;border-radius:8px;border:none;background:#2d7cf0;color:#fff;cursor:pointer;white-space:nowrap;">发送验证码</button>',
            '</div>',
            '<input id="reg-username" type="text" placeholder="用户名" style="padding:10px 12px;border-radius:8px;border:1px solid rgba(244,246,240,0.25);background:rgba(255,255,255,0.06);color:#F4F6F0;" />',
            '<input id="reg-password" type="password" placeholder="密码（至少8位，包含字母和数字）" style="padding:10px 12px;border-radius:8px;border:1px solid rgba(244,246,240,0.25);background:rgba(255,255,255,0.06);color:#F4F6F0;" />',
            '<input id="reg-password-confirm" type="password" placeholder="再次确认密码" style="padding:10px 12px;border-radius:8px;border:1px solid rgba(244,246,240,0.25);background:rgba(255,255,255,0.06);color:#F4F6F0;" />',
            '<button id="reg-submit-btn" style="padding:10px 12px;border-radius:8px;border:none;background:#4caf50;color:#fff;cursor:pointer;">注册</button>',
            '<div id="reg-message" style="min-height:18px;font-size:12px;color:#b8f3be;"></div>',
            '</div>',

            // 关闭按钮
            '<div style="display:flex;gap:10px;justify-content:flex-end;margin-top:8px;">',
            '<button id="auth-close" style="padding:8px 12px;border-radius:8px;border:1px solid rgba(244,246,240,0.25);background:transparent;color:#F4F6F0;cursor:pointer;">取消</button>',
            '</div>'
        ].join("");

        modal.appendChild(panel);
        document.body.appendChild(modal);

        // 主选项卡切换
        panel.querySelectorAll(".auth-main-tab-btn").forEach(function (btn) {
            btn.addEventListener("click", function () {
                switchAuthMainTab(btn.dataset.tab);
            });
        });

        // 登录面板内的模式切换
        const switchToCode = panel.querySelector("#switch-to-code-login");
        const switchToPassword = panel.querySelector("#switch-to-password-login");
        if (switchToCode) switchToCode.addEventListener("click", function () { switchLoginMode("code"); });
        if (switchToPassword) switchToPassword.addEventListener("click", function () { switchLoginMode("password"); });

        // 关闭按钮
        const closeBtn = panel.querySelector("#auth-close");
        if (closeBtn) closeBtn.addEventListener("click", closeAuthModal);
        modal.addEventListener("click", function (event) {
            if (event.target === modal) closeAuthModal();
        });

        // ==== 发送验证码（共用） ====
        panel.querySelectorAll(".auth-send-code-btn").forEach(function (btn) {
            btn.addEventListener("click", function () {
                const target = btn.dataset.target;
                const emailId = target === "reg" ? "reg-email" : "login-code-email";
                const email = trim(panel.querySelector("#" + emailId)?.value);
                onSendCode(email, target === "reg" ? "reg-message" : "auth-message");
            });
        });

        // ==== 登录：用户名/邮箱 + 密码 ====
        const loginPasswordBtn = panel.querySelector("#login-password-btn");
        if (loginPasswordBtn) {
            loginPasswordBtn.addEventListener("click", function () {
                const username = trim(panel.querySelector("#login-username")?.value);
                const password = panel.querySelector("#login-password")?.value || "";
                onPasswordLogin(username, password);
            });
        }

        // ==== 登录：邮箱 + 验证码 ====
        const loginCodeBtn = panel.querySelector("#login-code-btn");
        if (loginCodeBtn) {
            loginCodeBtn.addEventListener("click", function () {
                const email = trim(panel.querySelector("#login-code-email")?.value);
                const code = trim(panel.querySelector("#login-code-value")?.value);
                onCodeLogin(email, code);
            });
        }

        // ==== 注册 ====
        const regSubmitBtn = panel.querySelector("#reg-submit-btn");
        if (regSubmitBtn) {
            regSubmitBtn.addEventListener("click", function () {
                const email = trim(panel.querySelector("#reg-email")?.value);
                const code = trim(panel.querySelector("#reg-code-value")?.value);
                const username = trim(panel.querySelector("#reg-username")?.value);
                const password = panel.querySelector("#reg-password")?.value || "";
                const passwordConfirm = panel.querySelector("#reg-password-confirm")?.value || "";
                onRegister(email, code, username, password, passwordConfirm);
            });
        }

        // Enter 键处理
        panel.addEventListener("keydown", function (event) {
            if (event.key !== "Enter") return;
            event.preventDefault();

            const loginPanel = document.getElementById("auth-login-panel");
            const regPanel = document.getElementById("auth-register-panel");
            const loginVisible = loginPanel && loginPanel.style.display !== "none";
            const regVisible = regPanel && regPanel.style.display !== "none";

            if (regVisible) {
                panel.querySelector("#reg-submit-btn")?.click();
                return;
            }
            if (loginVisible) {
                const passwordPanel = document.getElementById("login-password-panel");
                const codePanel = document.getElementById("login-code-panel");
                if (passwordPanel && passwordPanel.style.display !== "none") {
                    panel.querySelector("#login-password-btn")?.click();
                } else if (codePanel && codePanel.style.display !== "none") {
                    panel.querySelector("#login-code-btn")?.click();
                }
            }
        });

        // 限制验证码输入为数字
        panel.querySelectorAll('input[inputmode="numeric"]').forEach(function (input) {
            input.addEventListener("input", function () {
                input.value = input.value.replace(/\D/g, "").slice(0, 6);
            });
        });

        updateSendCodeButtons();
        switchAuthMainTab("login");
        switchLoginMode("password");
        return modal;
    }

    async function onSendCode(email, msgId) {
        setAuthMessage("", false);
        setRegMessage("", false);
        if (!isValidEmail(email)) {
            if (msgId === "reg-message") setRegMessage("请输入合法邮箱", true);
            else setAuthMessage("请输入合法邮箱", true);
            return;
        }

        setAuthBusy(true);
        try {
            const result = await postJson("/api/auth/send_code", { email: email });
            if (result.ok && result.data && result.data.success) {
                startCooldown(Number(result.data.retry_after_seconds || 60));
                if (msgId === "reg-message") setRegMessage("验证码已发送，请查收邮箱", false);
                else setAuthMessage("验证码已发送，请查收邮箱", false);
                return;
            }
            const errorMsg = mapSendCodeError(result.data?.error_code, result.status);
            if (msgId === "reg-message") setRegMessage(errorMsg, true);
            else setAuthMessage(errorMsg, true);
        } catch (e) {
            if (msgId === "reg-message") setRegMessage("网络异常，请稍后重试", true);
            else setAuthMessage("网络异常，请稍后重试", true);
        } finally {
            setAuthBusy(false);
        }
    }

    async function onPasswordLogin(username, password) {
        if (!username) {
            setAuthMessage("请输入用户名或邮箱", true);
            return;
        }
        if (!password) {
            setAuthMessage("请输入密码", true);
            return;
        }

        setAuthBusy(true);
        setAuthMessage("正在登录...", false);
        try {
            const result = await postJson("/api/user/password/login", {
                username: username,
                password: password
            });

            if (result.ok && result.data && result.data.success) {
                state.currentUser = result.data.user || null;
                closeAuthModal();
                await refreshAuthUI();
                window.dispatchEvent(new CustomEvent("oj-auth-changed", {
                    detail: { user: state.currentUser, isNewUser: false }
                }));
                if (state.pendingRouteAfterLogin) {
                    window.location.hash = state.pendingRouteAfterLogin;
                    state.pendingRouteAfterLogin = "";
                }
                return;
            }

            setAuthMessage(mapPasswordLoginError(result.data?.error_code), true);
        } catch (e) {
            setAuthMessage("网络异常，请稍后重试", true);
        } finally {
            setAuthBusy(false);
        }
    }

    async function onCodeLogin(email, code) {
        if (!isValidEmail(email)) {
            setAuthMessage("请输入合法邮箱", true);
            return;
        }
        if (!isValidCode(code)) {
            setAuthMessage("验证码应为6位数字", true);
            return;
        }

        setAuthBusy(true);
        setAuthMessage("正在登录...", false);
        try {
            const result = await postJson("/api/auth/verify_code", {
                email: email,
                code: code,
                name: "",
                password: ""
            });

            if (result.ok && result.data && result.data.success) {
                state.currentUser = result.data.user || null;
                closeAuthModal();
                await refreshAuthUI();
                window.dispatchEvent(new CustomEvent("oj-auth-changed", {
                    detail: { user: state.currentUser, isNewUser: !!result.data.is_new_user }
                }));
                if (state.pendingRouteAfterLogin) {
                    window.location.hash = state.pendingRouteAfterLogin;
                    state.pendingRouteAfterLogin = "";
                }
                return;
            }

            setAuthMessage(mapVerifyCodeError(result.data?.error_code, result.status), true);
        } catch (e) {
            setAuthMessage("网络异常，请稍后重试", true);
        } finally {
            setAuthBusy(false);
        }
    }

    async function onRegister(email, code, name, password, passwordConfirm) {
        if (!isValidEmail(email)) {
            setRegMessage("请输入合法邮箱", true);
            return;
        }
        if (!isValidCode(code)) {
            setRegMessage("验证码应为6位数字", true);
            return;
        }
        if (!name) {
            setRegMessage("请填写用户名", true);
            return;
        }
        if (!isStrongPassword(password)) {
            setRegMessage("密码至少8位且包含字母和数字", true);
            return;
        }
        if (password !== passwordConfirm) {
            setRegMessage("两次输入的密码不一致", true);
            return;
        }

        setAuthBusy(true);
        setRegMessage("正在注册...", false);
        try {
            const result = await postJson("/api/auth/verify_code", {
                email: email,
                code: code,
                name: name,
                password: password
            });

            if (result.ok && result.data && result.data.success) {
                state.currentUser = result.data.user || null;
                closeAuthModal();
                await refreshAuthUI();
                window.dispatchEvent(new CustomEvent("oj-auth-changed", {
                    detail: { user: state.currentUser, isNewUser: true }
                }));
                if (state.pendingRouteAfterLogin) {
                    window.location.hash = state.pendingRouteAfterLogin;
                    state.pendingRouteAfterLogin = "";
                }
                return;
            }

            setRegMessage(mapVerifyCodeError(result.data?.error_code, result.status), true);
        } catch (e) {
            setRegMessage("网络异常，请稍后重试", true);
        } finally {
            setAuthBusy(false);
        }
    }

    function openAuthModal(options) {
        const modal = buildAuthModal();
        const usernameInput = modal.querySelector("#login-username");
        if (options && options.fromRoute) {
            state.pendingRouteAfterLogin = options.fromRoute;
        }
        if (usernameInput) {
            usernameInput.focus();
        }
    }

    async function logoutAndRefresh() {
        try {
            await postJson("/api/user/logout", {});
        } catch (e) {}
        window.location.reload();
    }

    function renderLoggedOut(userAuth, userProfile) {
        if (userAuth) {
            userAuth.style.display = "block";
        }
        if (userProfile) {
            userProfile.style.display = "none";
            userProfile.innerHTML = "";
        }

        const signInButton = document.getElementById("sign-in-button");
        if (signInButton) {
            signInButton.onclick = function () {
                openAuthModal();
            };
        }
    }

    function renderLoggedIn(userAuth, userProfile, user) {
        if (userAuth) {
            userAuth.style.display = "none";
        }
        if (!userProfile) {
            return;
        }

        const header = userProfile.closest(".header");
        if (header) {
            header.style.position = "relative";
            header.style.zIndex = "30000";
        }

        userProfile.style.display = "flex";
        userProfile.style.alignItems = "center";
        userProfile.innerHTML = "";

        const avatarWrapper = document.createElement("div");
        avatarWrapper.className = "avatar-wrapper";
        avatarWrapper.style.cssText = "position: relative; margin-left: 10px; cursor: pointer;";

        const avatar = document.createElement("img");
        const avatarSrc = (user.avatar_url && user.avatar_url.length > 0) ? user.avatar_url : "/pictures/head.jpg";
        avatar.src = avatarSrc;
        avatar.width = 40;
        avatar.height = 40;
        avatar.style.cssText = "border-radius:50%;display:block;object-fit:cover;";

        const dropdown = document.createElement("div");
        dropdown.style.cssText = [
            "position:absolute",
            "top:55px",
            "left:50%",
            "transform:translateX(-50%)",
            "background-color: rgba(60, 80, 60, 0.95)",
            "border-radius:16px",
            "box-shadow:0 8px 32px rgba(0,0,0,0.3)",
            "padding:18px",
            "width:220px",
            "z-index:30001",
            "border:1px solid rgba(244,246,240,0.15)",
            "display:none"
        ].join(";");

        dropdown.innerHTML = [
            '<div style="display:flex;flex-direction:column;align-items:center;margin-bottom:14px;">',
            '<img src="' + avatarSrc + '" width="56" height="56" style="border-radius:50%;margin-bottom:10px;border:2px solid rgba(244,246,240,0.2);object-fit:cover;" />',
            '<div style="font-size:15px;font-weight:500;color:#F4F6F0;">' + (user.name || "用户") + '</div>',
            '<div style="font-size:12px;color:rgba(244,246,240,0.7);margin-top:4px;">' + (user.email || "") + '</div>',
            '</div>',
            '<div style="display:flex;flex-direction:column;gap:8px;">',
            '<button id="go-profile-btn" style="padding:9px 14px;border:1px solid rgba(244,246,240,0.2);border-radius:8px;background:rgba(255,255,255,0.08);color:#F4F6F0;cursor:pointer;">个人中心</button>',
            '<button id="go-settings-btn" style="padding:9px 14px;border:1px solid rgba(244,246,240,0.2);border-radius:8px;background:rgba(255,255,255,0.08);color:#F4F6F0;cursor:pointer;">账户设置</button>',
            '<button id="sign-out-btn" style="padding:9px 14px;border:1px solid rgba(244,67,54,0.35);border-radius:8px;background:rgba(244,67,54,0.1);color:#F44336;cursor:pointer;">退出登录</button>',
            '</div>'
        ].join("");

        avatarWrapper.appendChild(avatar);
        avatarWrapper.appendChild(dropdown);
        userProfile.appendChild(avatarWrapper);

        let hideTimer = null;
        function showDropdown() {
            if (hideTimer) { clearTimeout(hideTimer); hideTimer = null; }
            dropdown.style.display = "block";
        }
        function hideDropdownDelay() {
            hideTimer = setTimeout(function () { dropdown.style.display = "none"; }, 120);
        }

        avatar.addEventListener("mouseenter", showDropdown);
        avatar.addEventListener("mouseleave", hideDropdownDelay);
        dropdown.addEventListener("mouseenter", showDropdown);
        dropdown.addEventListener("mouseleave", hideDropdownDelay);

        const profileBtn = dropdown.querySelector("#go-profile-btn");
        const settingsBtn = dropdown.querySelector("#go-settings-btn");
        const signOutBtn = dropdown.querySelector("#sign-out-btn");

        if (profileBtn) {
            profileBtn.onclick = function () {
                if (document.getElementById("page-content")) {
                    window.location.hash = "#profile";
                    return;
                }
                window.location.href = "/user/profile";
            };
        }
        if (settingsBtn) {
            settingsBtn.onclick = function () { window.location.href = "/user/settings"; };
        }
        if (signOutBtn) {
            signOutBtn.onclick = logoutAndRefresh;
        }
    }

    async function refreshAuthUI() {
        const userAuth = document.getElementById("user-auth");
        const userProfile = document.getElementById("user-profile");

        let user = state.currentUser;
        if (!user) {
            try { user = await fetchCurrentUser(); } catch (e) { user = null; }
        }
        state.currentUser = user;

        if (!user) { renderLoggedOut(userAuth, userProfile); return null; }
        renderLoggedIn(userAuth, userProfile, user);
        return user;
    }

    function bindProfileLogout() {
        const logoutBtn = document.getElementById("logout-btn");
        if (logoutBtn) logoutBtn.addEventListener("click", logoutAndRefresh);
    }

    async function requireLogin(options) {
        const user = await refreshAuthUI();
        if (user) return true;
        openAuthModal(options || {});
        return false;
    }

    window.addEventListener("oj-auth-required", function (event) {
        const fromRoute = event?.detail?.fromRoute || "";
        openAuthModal({ fromRoute: fromRoute });
    });

    document.addEventListener("DOMContentLoaded", async function () {
        await refreshAuthUI();
        bindProfileLogout();
    });

    window.OJAuth = {
        openAuthModal: openAuthModal,
        refreshAuthUI: refreshAuthUI,
        requireLogin: requireLogin
    };
})();

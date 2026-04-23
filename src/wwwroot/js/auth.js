// Unified auth script: password login + email-code login/register.
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
            return "账号不存在，请先使用邮箱验证码注册";
        }
        if (errorCode === "PASSWORD_NOT_SET") {
            return "该账号尚未设置密码，请使用验证码登录后在个人中心设置密码";
        }
        if (errorCode === "PASSWORD_MISMATCH") {
            return "邮箱或密码错误";
        }
        return "密码登录失败，请稍后重试";
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
                create_time: SERVER_USER_INFO.create_time || ""
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

    function getActiveAuthTab() {
        const activeBtn = document.querySelector(".auth-tab-btn.active");
        return activeBtn ? activeBtn.dataset.tab : "code";
    }

    function setAuthBusy(isBusy) {
        const sendBtn = document.getElementById("auth-send-code");
        const verifyBtn = document.getElementById("auth-verify-code");
        const passwordLoginBtn = document.getElementById("auth-password-login");
        const closeBtn = document.getElementById("auth-close");

        if (verifyBtn) verifyBtn.disabled = isBusy;
        if (passwordLoginBtn) passwordLoginBtn.disabled = isBusy;
        if (closeBtn) closeBtn.disabled = isBusy;

        if (sendBtn && state.sendCodeCooldownLeft <= 0) {
            sendBtn.disabled = isBusy;
        }
    }

    function updateSendCodeButton() {
        const sendBtn = document.getElementById("auth-send-code");
        if (!sendBtn) return;

        if (state.sendCodeCooldownLeft > 0) {
            sendBtn.disabled = true;
            sendBtn.textContent = "重新发送(" + state.sendCodeCooldownLeft + "s)";
            return;
        }

        sendBtn.disabled = false;
        sendBtn.textContent = "发送验证码";
    }

    function startCooldown(seconds) {
        clearCooldown();
        state.sendCodeCooldownLeft = Math.max(0, Number(seconds) || 0);
        updateSendCodeButton();

        if (state.sendCodeCooldownLeft <= 0) {
            return;
        }

        state.sendCodeCooldownTimer = setInterval(function () {
            state.sendCodeCooldownLeft -= 1;
            if (state.sendCodeCooldownLeft <= 0) {
                clearCooldown();
            }
            updateSendCodeButton();
        }, 1000);
    }

    function switchAuthTab(tabName) {
        const tabs = document.querySelectorAll(".auth-tab-btn");
        tabs.forEach(function (btn) {
            btn.classList.toggle("active", btn.dataset.tab === tabName);
            btn.style.opacity = btn.classList.contains("active") ? "1" : "0.7";
        });

        const passwordPanel = document.getElementById("auth-password-panel");
        const codePanel = document.getElementById("auth-code-panel");
        if (passwordPanel) {
            passwordPanel.style.display = tabName === "password" ? "flex" : "none";
        }
        if (codePanel) {
            codePanel.style.display = tabName === "code" ? "flex" : "none";
        }

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
            "width: min(92vw, 500px)",
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
            '<h3 style="margin:0 0 10px 0;font-weight:500;">登录 / 注册</h3>',
            '<p style="margin:0 0 16px 0;font-size:12px;color:rgba(244,246,240,.75);">支持密码登录，也支持邮箱验证码登录（新用户将自动注册）</p>',
            '<div style="display:flex;gap:8px;margin-bottom:14px;">',
            '<button class="auth-tab-btn active" data-tab="code" style="flex:1;padding:9px 10px;border-radius:8px;border:1px solid rgba(244,246,240,0.2);background:rgba(255,255,255,0.12);color:#F4F6F0;cursor:pointer;">邮箱验证码</button>',
            '<button class="auth-tab-btn" data-tab="password" style="flex:1;padding:9px 10px;border-radius:8px;border:1px solid rgba(244,246,240,0.2);background:rgba(255,255,255,0.06);color:#F4F6F0;cursor:pointer;opacity:0.7;">邮箱密码</button>',
            '</div>',

            '<div id="auth-code-panel" style="display:flex;flex-direction:column;gap:10px;">',
            '<input id="auth-code-email" type="email" placeholder="请输入邮箱" style="padding:10px 12px;border-radius:8px;border:1px solid rgba(244,246,240,0.25);background:rgba(255,255,255,0.06);color:#F4F6F0;" />',
            '<div style="display:flex;gap:8px;">',
            '<input id="auth-code-value" type="text" maxlength="6" inputmode="numeric" placeholder="请输入6位验证码" style="flex:1;padding:10px 12px;border-radius:8px;border:1px solid rgba(244,246,240,0.25);background:rgba(255,255,255,0.06);color:#F4F6F0;" />',
            '<button id="auth-send-code" style="padding:10px 12px;border-radius:8px;border:none;background:#2d7cf0;color:#fff;cursor:pointer;white-space:nowrap;">发送验证码</button>',
            '</div>',
            '<input id="auth-register-name" type="text" placeholder="新用户注册用户名（必填）" style="padding:10px 12px;border-radius:8px;border:1px solid rgba(244,246,240,0.25);background:rgba(255,255,255,0.06);color:#F4F6F0;" />',
            '<input id="auth-register-password" type="password" placeholder="新用户注册密码（至少8位，字母+数字）" style="padding:10px 12px;border-radius:8px;border:1px solid rgba(244,246,240,0.25);background:rgba(255,255,255,0.06);color:#F4F6F0;" />',
            '<input id="auth-register-password-confirm" type="password" placeholder="确认注册密码" style="padding:10px 12px;border-radius:8px;border:1px solid rgba(244,246,240,0.25);background:rgba(255,255,255,0.06);color:#F4F6F0;" />',
            '<div style="font-size:12px;color:rgba(244,246,240,0.65);">说明：已有账号只需邮箱+验证码即可登录。新账号需额外填写用户名和密码。</div>',
            '<button id="auth-verify-code" style="padding:10px 12px;border-radius:8px;border:none;background:#4caf50;color:#fff;cursor:pointer;">验证码登录 / 注册</button>',
            '</div>',

            '<div id="auth-password-panel" style="display:none;flex-direction:column;gap:10px;">',
            '<input id="auth-password-email" type="email" placeholder="请输入邮箱" style="padding:10px 12px;border-radius:8px;border:1px solid rgba(244,246,240,0.25);background:rgba(255,255,255,0.06);color:#F4F6F0;" />',
            '<input id="auth-password-value" type="password" placeholder="请输入密码" style="padding:10px 12px;border-radius:8px;border:1px solid rgba(244,246,240,0.25);background:rgba(255,255,255,0.06);color:#F4F6F0;" />',
            '<button id="auth-password-login" style="padding:10px 12px;border-radius:8px;border:none;background:#2d7cf0;color:#fff;cursor:pointer;">密码登录</button>',
            '</div>',

            '<div id="auth-message" style="min-height:18px;font-size:12px;color:#b8f3be;margin-top:12px;"></div>',
            '<div style="display:flex;gap:10px;justify-content:flex-end;margin-top:10px;">',
            '<button id="auth-close" style="padding:8px 12px;border-radius:8px;border:1px solid rgba(244,246,240,0.25);background:transparent;color:#F4F6F0;cursor:pointer;">取消</button>',
            '</div>'
        ].join("");

        modal.appendChild(panel);
        document.body.appendChild(modal);

        const closeBtn = panel.querySelector("#auth-close");
        const sendBtn = panel.querySelector("#auth-send-code");
        const verifyBtn = panel.querySelector("#auth-verify-code");
        const passwordLoginBtn = panel.querySelector("#auth-password-login");
        const codeInput = panel.querySelector("#auth-code-value");

        function readCodeInputs() {
            return {
                email: trim(panel.querySelector("#auth-code-email")?.value),
                code: trim(panel.querySelector("#auth-code-value")?.value),
                name: trim(panel.querySelector("#auth-register-name")?.value),
                password: panel.querySelector("#auth-register-password")?.value || "",
                confirmPassword: panel.querySelector("#auth-register-password-confirm")?.value || ""
            };
        }

        function readPasswordInputs() {
            return {
                email: trim(panel.querySelector("#auth-password-email")?.value),
                password: panel.querySelector("#auth-password-value")?.value || ""
            };
        }

        async function onSendCode() {
            const input = readCodeInputs();
            if (!isValidEmail(input.email)) {
                setAuthMessage("请输入合法邮箱", true);
                return;
            }

            setAuthBusy(true);
            setAuthMessage("", false);

            try {
                const result = await postJson("/api/auth/send_code", { email: input.email });
                if (result.ok && result.data && result.data.success) {
                    const retryAfter = Number(result.data.retry_after_seconds || 60);
                    startCooldown(retryAfter);
                    setAuthMessage("验证码已发送，请查收邮箱", false);
                    if (codeInput) codeInput.focus();
                    return;
                }

                const errorMsg = mapSendCodeError(result.data?.error_code, result.status);
                setAuthMessage(errorMsg, true);
            } catch (e) {
                setAuthMessage("网络异常，请稍后重试", true);
            } finally {
                setAuthBusy(false);
                updateSendCodeButton();
            }
        }

        async function onVerifyCode() {
            const input = readCodeInputs();
            if (!isValidEmail(input.email)) {
                setAuthMessage("请输入合法邮箱", true);
                return;
            }
            if (!isValidCode(input.code)) {
                setAuthMessage("验证码应为6位数字", true);
                return;
            }

            if (input.password || input.confirmPassword || input.name) {
                if (!input.name) {
                    setAuthMessage("注册时请填写用户名", true);
                    return;
                }
                if (!isStrongPassword(input.password)) {
                    setAuthMessage("注册密码至少8位且包含字母和数字", true);
                    return;
                }
                if (input.password !== input.confirmPassword) {
                    setAuthMessage("两次输入的注册密码不一致", true);
                    return;
                }
            }

            setAuthBusy(true);
            setAuthMessage("正在验证...", false);

            try {
                const result = await postJson("/api/auth/verify_code", {
                    email: input.email,
                    code: input.code,
                    name: input.name,
                    password: input.password
                });

                if (result.ok && result.data && result.data.success) {
                    state.currentUser = result.data.user || null;
                    setAuthMessage("登录成功", false);
                    closeAuthModal();
                    await refreshAuthUI();
                    window.dispatchEvent(new CustomEvent("oj-auth-changed", {
                        detail: {
                            user: state.currentUser,
                            isNewUser: !!result.data.is_new_user
                        }
                    }));

                    if (state.pendingRouteAfterLogin) {
                        window.location.hash = state.pendingRouteAfterLogin;
                        state.pendingRouteAfterLogin = "";
                    }
                    return;
                }

                const errorMsg = mapVerifyCodeError(result.data?.error_code, result.status);
                setAuthMessage(errorMsg, true);
            } catch (e) {
                setAuthMessage("网络异常，请稍后重试", true);
            } finally {
                setAuthBusy(false);
                updateSendCodeButton();
            }
        }

        async function onPasswordLogin() {
            const input = readPasswordInputs();
            if (!isValidEmail(input.email)) {
                setAuthMessage("请输入合法邮箱", true);
                return;
            }
            if (!input.password) {
                setAuthMessage("请输入密码", true);
                return;
            }

            setAuthBusy(true);
            setAuthMessage("正在登录...", false);
            try {
                const result = await postJson("/api/user/password/login", {
                    email: input.email,
                    password: input.password
                });

                if (result.ok && result.data && result.data.success) {
                    state.currentUser = result.data.user || null;
                    closeAuthModal();
                    await refreshAuthUI();
                    window.dispatchEvent(new CustomEvent("oj-auth-changed", {
                        detail: {
                            user: state.currentUser,
                            isNewUser: false
                        }
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
                updateSendCodeButton();
            }
        }

        if (closeBtn) {
            closeBtn.addEventListener("click", closeAuthModal);
        }

        modal.addEventListener("click", function (event) {
            if (event.target === modal) {
                closeAuthModal();
            }
        });

        panel.querySelectorAll(".auth-tab-btn").forEach(function (btn) {
            btn.addEventListener("click", function () {
                switchAuthTab(btn.dataset.tab);
            });
        });

        if (sendBtn) {
            sendBtn.addEventListener("click", onSendCode);
        }

        if (verifyBtn) {
            verifyBtn.addEventListener("click", onVerifyCode);
        }

        if (passwordLoginBtn) {
            passwordLoginBtn.addEventListener("click", onPasswordLogin);
        }

        panel.addEventListener("keydown", function (event) {
            if (event.key !== "Enter") {
                return;
            }

            const activeTab = getActiveAuthTab();
            if (activeTab === "password") {
                event.preventDefault();
                onPasswordLogin();
                return;
            }

            const targetId = event.target && event.target.id;
            if (targetId === "auth-code-email") {
                event.preventDefault();
                onSendCode();
                return;
            }

            event.preventDefault();
            onVerifyCode();
        });

        if (codeInput) {
            codeInput.addEventListener("input", function () {
                codeInput.value = codeInput.value.replace(/\D/g, "").slice(0, 6);
            });
        }

        updateSendCodeButton();
        switchAuthTab("code");
        return modal;
    }

    function openAuthModal(options) {
        const modal = buildAuthModal();
        const emailInput = modal.querySelector("#auth-code-email");

        if (options && options.fromRoute) {
            state.pendingRouteAfterLogin = options.fromRoute;
        }

        if (emailInput) {
            emailInput.focus();
        }
    }

    async function logoutAndRefresh() {
        try {
            await postJson("/api/user/logout", {});
        } catch (e) {
            // Ignore network errors during logout and still clear UI with reload.
        }
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
            // Keep header stacking context above page content so the avatar dropdown is visible.
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
        avatar.src = "/pictures/head.jpg";
        avatar.width = 40;
        avatar.height = 40;
        avatar.style.cssText = "border-radius:50%;display:block;";

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
            '<img src="/pictures/head.jpg" width="56" height="56" style="border-radius:50%;margin-bottom:10px;border:2px solid rgba(244,246,240,0.2);" />',
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
            if (hideTimer) {
                clearTimeout(hideTimer);
                hideTimer = null;
            }
            dropdown.style.display = "block";
        }
        function hideDropdownDelay() {
            hideTimer = setTimeout(function () {
                dropdown.style.display = "none";
            }, 120);
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
                // In SPA app, route to hash page so profile content comes from spa/pages/profile.html.
                if (document.getElementById("page-content")) {
                    window.location.hash = "#profile";
                    return;
                }
                window.location.href = "/user/profile";
            };
        }
        if (settingsBtn) {
            settingsBtn.onclick = function () {
                window.location.href = "/user/settings";
            };
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
            try {
                user = await fetchCurrentUser();
            } catch (e) {
                user = null;
            }
        }

        state.currentUser = user;

        if (!user) {
            renderLoggedOut(userAuth, userProfile);
            return null;
        }

        renderLoggedIn(userAuth, userProfile, user);
        return user;
    }

    function bindProfileLogout() {
        const logoutBtn = document.getElementById("logout-btn");
        if (logoutBtn) {
            logoutBtn.addEventListener("click", logoutAndRefresh);
        }
    }

    async function requireLogin(options) {
        const user = await refreshAuthUI();
        if (user) {
            return true;
        }
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

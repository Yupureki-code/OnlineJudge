// Unified auth script based on email verification code APIs.
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
        if (errorCode === "CODE_NOT_FOUND") {
            return "验证码不存在或已过期，请重新获取";
        }
        if (errorCode === "CODE_MISMATCH") {
            return "验证码错误，请检查后重试";
        }
        if (errorCode === "CODE_EXPIRED") {
            return "验证码已过期，请重新获取";
        }
        if (errorCode === "USER_CREATE_FAILED") {
            return "自动创建账号失败，请稍后重试";
        }
        return "验证失败，请稍后重试";
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

    function setAuthBusy(isBusy) {
        const sendBtn = document.getElementById("auth-send-code");
        const verifyBtn = document.getElementById("auth-verify-code");
        const closeBtn = document.getElementById("auth-close");
        if (verifyBtn) verifyBtn.disabled = isBusy;
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

    function buildAuthModal() {
        closeAuthModal();

        const modal = document.createElement("div");
        modal.id = "auth-modal";
        modal.style.cssText = [
            "position: fixed",
            "inset: 0",
            "z-index: 10001",
            "display: flex",
            "align-items: center",
            "justify-content: center",
            "background: rgba(0,0,0,0.72)",
            "backdrop-filter: blur(4px)"
        ].join(";");

        const panel = document.createElement("div");
        panel.style.cssText = [
            "width: min(92vw, 460px)",
            "padding: 24px",
            "border-radius: 14px",
            "background: rgba(40, 52, 40, 0.96)",
            "border: 1px solid rgba(244, 246, 240, 0.2)",
            "box-shadow: 0 10px 40px rgba(0,0,0,0.35)",
            "color: #F4F6F0"
        ].join(";");

        panel.innerHTML = [
            '<h3 style="margin:0 0 10px 0;font-weight:500;">邮箱验证码登录</h3>',
            '<p style="margin:0 0 16px 0;font-size:12px;color:rgba(244,246,240,.75);">首次使用该邮箱会自动创建账号</p>',
            '<div style="display:flex;flex-direction:column;gap:10px;">',
            '<input id="auth-email" type="email" placeholder="请输入邮箱" style="padding:10px 12px;border-radius:8px;border:1px solid rgba(244,246,240,0.25);background:rgba(255,255,255,0.06);color:#F4F6F0;" />',
            '<div style="display:flex;gap:8px;">',
            '<input id="auth-code" type="text" maxlength="6" inputmode="numeric" placeholder="请输入6位验证码" style="flex:1;padding:10px 12px;border-radius:8px;border:1px solid rgba(244,246,240,0.25);background:rgba(255,255,255,0.06);color:#F4F6F0;" />',
            '<button id="auth-send-code" style="padding:10px 12px;border-radius:8px;border:none;background:#2d7cf0;color:#fff;cursor:pointer;white-space:nowrap;">发送验证码</button>',
            '</div>',
            '<input id="auth-name" type="text" placeholder="首次注册用户名（可选）" style="padding:10px 12px;border-radius:8px;border:1px solid rgba(244,246,240,0.25);background:rgba(255,255,255,0.06);color:#F4F6F0;" />',
            '<div id="auth-message" style="min-height:18px;font-size:12px;color:#b8f3be;"></div>',
            '<div style="display:flex;gap:10px;justify-content:flex-end;">',
            '<button id="auth-close" style="padding:8px 12px;border-radius:8px;border:1px solid rgba(244,246,240,0.25);background:transparent;color:#F4F6F0;cursor:pointer;">取消</button>',
            '<button id="auth-verify-code" style="padding:8px 12px;border-radius:8px;border:none;background:#4caf50;color:#fff;cursor:pointer;">登录 / 注册</button>',
            '</div>',
            '</div>'
        ].join("");

        modal.appendChild(panel);
        document.body.appendChild(modal);

        const closeBtn = panel.querySelector("#auth-close");
        const sendBtn = panel.querySelector("#auth-send-code");
        const verifyBtn = panel.querySelector("#auth-verify-code");
        const emailInput = panel.querySelector("#auth-email");
        const codeInput = panel.querySelector("#auth-code");

        function readInputs() {
            const email = trim(panel.querySelector("#auth-email")?.value);
            const code = trim(panel.querySelector("#auth-code")?.value);
            const name = trim(panel.querySelector("#auth-name")?.value);
            return { email, code, name };
        }

        async function onSendCode() {
            const input = readInputs();
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
            const input = readInputs();
            if (!isValidEmail(input.email)) {
                setAuthMessage("请输入合法邮箱", true);
                return;
            }
            if (!isValidCode(input.code)) {
                setAuthMessage("验证码应为6位数字", true);
                return;
            }

            setAuthBusy(true);
            setAuthMessage("正在验证...", false);

            try {
                const result = await postJson("/api/auth/verify_code", {
                    email: input.email,
                    code: input.code,
                    name: input.name
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

        if (closeBtn) {
            closeBtn.addEventListener("click", closeAuthModal);
        }
        modal.addEventListener("click", function (event) {
            if (event.target === modal) {
                closeAuthModal();
            }
        });
        if (sendBtn) {
            sendBtn.addEventListener("click", onSendCode);
        }
        if (verifyBtn) {
            verifyBtn.addEventListener("click", onVerifyCode);
        }
        if (emailInput) {
            emailInput.addEventListener("keydown", function (event) {
                if (event.key === "Enter") {
                    event.preventDefault();
                    onSendCode();
                }
            });
        }
        if (codeInput) {
            codeInput.addEventListener("keydown", function (event) {
                if (event.key === "Enter") {
                    event.preventDefault();
                    onVerifyCode();
                }
            });
            codeInput.addEventListener("input", function () {
                codeInput.value = codeInput.value.replace(/\D/g, "").slice(0, 6);
            });
        }

        updateSendCodeButton();
        return modal;
    }

    function openAuthModal(options) {
        const modal = buildAuthModal();
        const emailInput = modal.querySelector("#auth-email");

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
            "z-index:10001",
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
        const signOutBtn = dropdown.querySelector("#sign-out-btn");

        if (profileBtn) {
            profileBtn.onclick = function () {
                window.location.href = "/user/profile";
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

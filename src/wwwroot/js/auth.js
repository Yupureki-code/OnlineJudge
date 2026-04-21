// Unified auth script based on backend session APIs.

(function () {
    function trim(value) {
        return (value || "").trim();
    }

    function isValidEmail(email) {
        return /^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}$/.test(email);
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

        return { ok: response.ok, status: response.status, data: payload };
    }

    async function fetchCurrentUser() {
        try {
            const response = await fetch("/api/user/info", { credentials: "include" });
            const data = await response.json();
            if (data && data.success && data.user) {
                return data.user;
            }
        } catch (e) {
            // ignore network errors here; UI falls back to logged-out state
        }

        if (typeof SERVER_USER_INFO !== "undefined" && SERVER_USER_INFO && SERVER_USER_INFO.isLoggedIn) {
            return {
                name: SERVER_USER_INFO.name || "用户",
                email: SERVER_USER_INFO.email || "",
                create_time: SERVER_USER_INFO.create_time || ""
            };
        }

        return null;
    }

    function closeAuthModal() {
        const modal = document.getElementById("auth-modal");
        if (modal) {
            modal.remove();
        }
    }

    function openAuthModal(onAuthSuccess) {
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
            "background: rgba(0,0,0,0.7)",
            "backdrop-filter: blur(4px)"
        ].join(";");

        const panel = document.createElement("div");
        panel.style.cssText = [
            "width: min(90vw, 420px)",
            "padding: 24px",
            "border-radius: 14px",
            "background: rgba(40, 52, 40, 0.95)",
            "border: 1px solid rgba(244, 246, 240, 0.2)",
            "box-shadow: 0 10px 40px rgba(0,0,0,0.35)",
            "color: #F4F6F0"
        ].join(";");

        panel.innerHTML = `
            <h3 style="margin-bottom:14px;font-weight:500;">邮箱登录 / 注册</h3>
            <div style="display:flex;flex-direction:column;gap:10px;">
                <input id="auth-email" type="email" placeholder="请输入邮箱" style="padding:10px 12px;border-radius:8px;border:1px solid rgba(244,246,240,0.25);background:rgba(255,255,255,0.05);color:#F4F6F0;" />
                <input id="auth-name" type="text" placeholder="注册时填写用户名（可选）" style="padding:10px 12px;border-radius:8px;border:1px solid rgba(244,246,240,0.25);background:rgba(255,255,255,0.05);color:#F4F6F0;" />
                <div id="auth-error" style="min-height:18px;color:#ff8f8f;font-size:12px;"></div>
                <div style="display:flex;gap:10px;justify-content:flex-end;">
                    <button id="auth-cancel" style="padding:8px 12px;border-radius:8px;border:1px solid rgba(244,246,240,0.25);background:transparent;color:#F4F6F0;cursor:pointer;">取消</button>
                    <button id="auth-login" style="padding:8px 12px;border-radius:8px;border:none;background:#4caf50;color:#fff;cursor:pointer;">邮箱登录</button>
                    <button id="auth-register" style="padding:8px 12px;border-radius:8px;border:none;background:#2d7cf0;color:#fff;cursor:pointer;">注册并登录</button>
                </div>
            </div>
        `;

        modal.appendChild(panel);
        document.body.appendChild(modal);

        const emailInput = panel.querySelector("#auth-email");
        const nameInput = panel.querySelector("#auth-name");
        const errorEl = panel.querySelector("#auth-error");
        const cancelBtn = panel.querySelector("#auth-cancel");
        const loginBtn = panel.querySelector("#auth-login");
        const registerBtn = panel.querySelector("#auth-register");

        function setError(msg) {
            errorEl.textContent = msg || "";
        }

        function getInput() {
            const email = trim(emailInput.value);
            const name = trim(nameInput.value);
            if (!isValidEmail(email)) {
                setError("请输入合法邮箱");
                return null;
            }
            return { email, name };
        }

        cancelBtn.addEventListener("click", closeAuthModal);
        modal.addEventListener("click", (e) => {
            if (e.target === modal) {
                closeAuthModal();
            }
        });

        loginBtn.addEventListener("click", async () => {
            setError("");
            const input = getInput();
            if (!input) {
                return;
            }

            const result = await postJson("/api/user/login", { email: input.email });
            if (result.ok && result.data && result.data.exists) {
                closeAuthModal();
                if (typeof onAuthSuccess === "function") {
                    onAuthSuccess();
                }
                return;
            }

            if (result.ok && result.data && result.data.exists === false) {
                setError("用户不存在，请使用“注册并登录”");
                return;
            }

            setError((result.data && result.data.error) || "登录失败，请稍后重试");
        });

        registerBtn.addEventListener("click", async () => {
            setError("");
            const input = getInput();
            if (!input) {
                return;
            }

            if (!input.name) {
                setError("注册需要用户名");
                return;
            }

            const createRes = await postJson("/api/user/create", { name: input.name, email: input.email });
            if (createRes.ok && createRes.data && createRes.data.created) {
                closeAuthModal();
                if (typeof onAuthSuccess === "function") {
                    onAuthSuccess();
                }
                return;
            }

            setError((createRes.data && createRes.data.error) || "注册失败，可能邮箱已存在");
        });
    }

    async function logoutAndRefresh() {
        try {
            await postJson("/api/user/logout", {});
        } catch (e) {
            // ignore and continue with refresh
        }
        window.location.reload();
    }

    function renderLoggedOut(userAuth, userProfile, onLogin) {
        if (userAuth) {
            userAuth.style.display = "block";
        }
        if (userProfile) {
            userProfile.style.display = "none";
            userProfile.innerHTML = "";
        }

        const signInButton = document.getElementById("sign-in-button");
        if (signInButton) {
            signInButton.onclick = onLogin;
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

        dropdown.innerHTML = `
            <div style="display:flex;flex-direction:column;align-items:center;margin-bottom:14px;">
                <img src="/pictures/head.jpg" width="56" height="56" style="border-radius:50%;margin-bottom:10px;border:2px solid rgba(244,246,240,0.2);" />
                <div style="font-size:15px;font-weight:500;color:#F4F6F0;">${user.name || "用户"}</div>
                <div style="font-size:12px;color:rgba(244,246,240,0.7);margin-top:4px;">${user.email || ""}</div>
            </div>
            <div style="display:flex;flex-direction:column;gap:8px;">
                <button id="go-profile-btn" style="padding:9px 14px;border:1px solid rgba(244,246,240,0.2);border-radius:8px;background:rgba(255,255,255,0.08);color:#F4F6F0;cursor:pointer;">个人中心</button>
                <button id="sign-out-btn" style="padding:9px 14px;border:1px solid rgba(244,67,54,0.35);border-radius:8px;background:rgba(244,67,54,0.1);color:#F44336;cursor:pointer;">退出登录</button>
            </div>
        `;

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
            hideTimer = setTimeout(() => {
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
        const user = await fetchCurrentUser();

        if (!user) {
            renderLoggedOut(userAuth, userProfile, function () {
                openAuthModal(refreshAuthUI);
            });
            return;
        }

        renderLoggedIn(userAuth, userProfile, user);
    }

    async function bindProfileLogout() {
        const logoutBtn = document.getElementById("logout-btn");
        if (logoutBtn) {
            logoutBtn.addEventListener("click", logoutAndRefresh);
        }
    }

    document.addEventListener("DOMContentLoaded", async function () {
        await refreshAuthUI();
        await bindProfileLogout();
    });
})();
// Shared Session Authentication Script
// Replaces Clerk-based auth with backend session APIs.

(function () {
    function apiPost(url, body) {
        return fetch(url, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            credentials: "include",
            body: JSON.stringify(body || {})
        }).then((res) => res.json());
    }

    function apiGet(url) {
        return fetch(url, {
            method: "GET",
            credentials: "include"
        }).then((res) => res.json());
    }

    function createAuthModal() {
        let modal = document.getElementById("auth-modal");
        if (modal) return modal;

        modal = document.createElement("div");
        modal.id = "auth-modal";
        modal.style.cssText = [
            "position: fixed",
            "top: 0",
            "left: 0",
            "width: 100%",
            "height: 100%",
            "display: none",
            "justify-content: center",
            "align-items: center",
            "background: rgba(0,0,0,0.75)",
            "backdrop-filter: blur(6px)",
            "z-index: 10010"
        ].join(";");

        const panel = document.createElement("div");
        panel.style.cssText = [
            "width: min(92vw, 420px)",
            "border-radius: 16px",
            "padding: 22px",
            "background: rgba(60, 80, 60, 0.95)",
            "border: 1px solid rgba(244, 246, 240, 0.15)",
            "box-shadow: 0 10px 35px rgba(0,0,0,0.35)",
            "color: #F4F6F0"
        ].join(";");

        panel.innerHTML = [
            '<h3 style="margin: 0 0 14px 0; font-size: 18px; font-weight: 500;">邮箱登录 / 注册</h3>',
            '<div style="display:flex; flex-direction:column; gap:10px;">',
            '<input id="auth-email" type="email" placeholder="请输入邮箱" style="padding:10px 12px; border-radius:10px; border:1px solid rgba(244,246,240,.25); background: rgba(255,255,255,.06); color:#F4F6F0;">',
            '<input id="auth-name" type="text" placeholder="注册时填写用户名（登录可留空）" style="padding:10px 12px; border-radius:10px; border:1px solid rgba(244,246,240,.25); background: rgba(255,255,255,.06); color:#F4F6F0;">',
            '<div id="auth-msg" style="min-height:18px; color:#ffd26f; font-size:12px;"></div>',
            '<div style="display:flex; gap:10px; justify-content:flex-end;">',
            '<button id="auth-cancel" style="padding:8px 14px; border-radius:8px; border:1px solid rgba(244,246,240,.25); background: rgba(255,255,255,.08); color:#F4F6F0; cursor:pointer;">取消</button>',
            '<button id="auth-login" style="padding:8px 14px; border-radius:8px; border:0; background: rgba(76,175,80,.85); color:#fff; cursor:pointer;">邮箱登录</button>',
            '<button id="auth-register" style="padding:8px 14px; border-radius:8px; border:0; background: rgba(33,150,243,.85); color:#fff; cursor:pointer;">注册并登录</button>',
            "</div>",
            "</div>"
        ].join("");

        modal.appendChild(panel);
        document.body.appendChild(modal);
        return modal;
    }

    function readAuthInputs() {
        const emailInput = document.getElementById("auth-email");
        const nameInput = document.getElementById("auth-name");
        const msg = document.getElementById("auth-msg");
        const email = (emailInput?.value || "").trim();
        const name = (nameInput?.value || "").trim();

        if (!email) {
            if (msg) msg.textContent = "请先输入邮箱";
            return null;
        }

        return { email, name, msg };
    }

    async function logoutAndRefresh() {
        try {
            await fetch("/api/user/logout", {
                method: "POST",
                credentials: "include"
            });
        } catch (e) {
            console.error("退出登录失败:", e);
        }
        window.location.reload();
    }

    function renderGuest(userAuth, userProfile) {
        if (userAuth) userAuth.style.display = "block";
        if (userProfile) {
            userProfile.style.display = "none";
            userProfile.innerHTML = "";
        }
    }

    function renderLoggedIn(userAuth, userProfile, user) {
        if (!userProfile) return;
        if (userAuth) userAuth.style.display = "none";

        userProfile.style.display = "flex";
        userProfile.style.alignItems = "center";
        userProfile.innerHTML = "";

        const chatIcon = document.createElement("a");
        chatIcon.href = "#";
        chatIcon.innerHTML = '<img src="/pictures/message.png" width="40" height="40">';
        userProfile.appendChild(chatIcon);

        const avatarWrapper = document.createElement("div");
        avatarWrapper.style.cssText = "position: relative; margin-left: 10px; cursor: pointer;";

        const avatar = document.createElement("img");
        avatar.src = "/pictures/head.jpg";
        avatar.width = 40;
        avatar.height = 40;
        avatar.style.cssText = "border-radius: 50%; display: block;";
        avatar.addEventListener("click", function () {
            window.location.href = "/user/profile";
        });

        const dropdown = document.createElement("div");
        dropdown.style.cssText = [
            "position: absolute",
            "top: 55px",
            "left: 50%",
            "transform: translateX(-50%)",
            "background-color: rgba(60, 80, 60, 0.95)",
            "border-radius: 16px",
            "box-shadow: 0 8px 32px rgba(0,0,0,0.3)",
            "padding: 16px",
            "width: 220px",
            "z-index: 10001",
            "border: 1px solid rgba(244, 246, 240, 0.15)",
            "backdrop-filter: blur(10px)",
            "display: none"
        ].join(";");

        dropdown.innerHTML = [
            '<div style="display:flex; flex-direction:column; align-items:center; margin-bottom:12px;">',
            '<img src="/pictures/head.jpg" width="60" height="60" style="border-radius:50%; margin-bottom:10px; border:2px solid rgba(244,246,240,.2);">',
            `<div style="font-size:16px; font-weight:500; color:#F4F6F0;">${user.name || "用户"}</div>`,
            `<div style="font-size:12px; color:rgba(244,246,240,.7); margin-top:4px;">${user.email || ""}</div>`,
            "</div>",
            '<hr style="margin: 10px 0; border: none; border-top: 1px solid rgba(244,246,240,.1);">',
            '<div style="display:flex; flex-direction:column; gap:8px;">',
            '<button id="go-profile-btn" style="padding:10px 16px; border:1px solid rgba(244,246,240,.2); border-radius:8px; background: rgba(255,255,255,.08); color:#F4F6F0; cursor:pointer;">个人中心</button>',
            '<button id="sign-out-btn" style="padding:10px 16px; border:1px solid rgba(244,67,54,.3); border-radius:8px; background: rgba(244,67,54,.1); color:#F44336; cursor:pointer;">退出登录</button>',
            "</div>"
        ].join("");

        avatarWrapper.appendChild(avatar);
        avatarWrapper.appendChild(dropdown);
        userProfile.appendChild(avatarWrapper);

        avatarWrapper.addEventListener("mouseenter", function () {
            dropdown.style.display = "block";
        });
        avatarWrapper.addEventListener("mouseleave", function () {
            dropdown.style.display = "none";
        });

        const profileBtn = dropdown.querySelector("#go-profile-btn");
        if (profileBtn) {
            profileBtn.addEventListener("click", function () {
                window.location.href = "/user/profile";
            });
        }

        const signOutBtn = dropdown.querySelector("#sign-out-btn");
        if (signOutBtn) {
            signOutBtn.addEventListener("click", logoutAndRefresh);
        }
    }

    async function refreshAuthUI() {
        const userAuth = document.getElementById("user-auth");
        const userProfile = document.getElementById("user-profile");
        if (!userAuth && !userProfile) return;

        // 优先用服务端注入，减少首屏闪烁。
        if (typeof SERVER_USER_INFO !== "undefined" && SERVER_USER_INFO.isLoggedIn) {
            renderLoggedIn(userAuth, userProfile, SERVER_USER_INFO);
            return;
        }

        try {
            const info = await apiGet("/api/user/info");
            if (info && info.success && info.user) {
                renderLoggedIn(userAuth, userProfile, info.user);
            } else {
                renderGuest(userAuth, userProfile);
            }
        } catch (e) {
            console.error("获取登录态失败:", e);
            renderGuest(userAuth, userProfile);
        }
    }

    function bindSignIn() {
        const signInButton = document.getElementById("sign-in-button");
        if (!signInButton) return;

        signInButton.addEventListener("click", function () {
            const modal = createAuthModal();
            modal.style.display = "flex";

            const cancelBtn = document.getElementById("auth-cancel");
            const loginBtn = document.getElementById("auth-login");
            const registerBtn = document.getElementById("auth-register");
            const msg = document.getElementById("auth-msg");

            if (cancelBtn) {
                cancelBtn.onclick = function () {
                    modal.style.display = "none";
                };
            }

            if (loginBtn) {
                loginBtn.onclick = async function () {
                    const form = readAuthInputs();
                    if (!form) return;

                    try {
                        const result = await apiPost("/api/user/login", { email: form.email });
                        if (result && result.exists) {
                            modal.style.display = "none";
                            await refreshAuthUI();
                            window.location.reload();
                            return;
                        }
                        if (msg) msg.textContent = "该邮箱未注册，请先注册";
                    } catch (e) {
                        if (msg) msg.textContent = "登录失败，请稍后重试";
                    }
                };
            }

            if (registerBtn) {
                registerBtn.onclick = async function () {
                    const form = readAuthInputs();
                    if (!form) return;
                    if (!form.name) {
                        if (form.msg) form.msg.textContent = "注册需要填写用户名";
                        return;
                    }

                    try {
                        const result = await apiPost("/api/user/create", {
                            name: form.name,
                            email: form.email
                        });
                        if (result && result.created) {
                            modal.style.display = "none";
                            await refreshAuthUI();
                            window.location.reload();
                            return;
                        }
                        if (form.msg) form.msg.textContent = "注册失败，邮箱可能已存在";
                    } catch (e) {
                        if (form.msg) form.msg.textContent = "注册失败，请稍后重试";
                    }
                };
            }
        });
    }

    function bindProfileLogoutButton() {
        const logoutBtn = document.getElementById("logout-btn");
        if (!logoutBtn) return;

        logoutBtn.addEventListener("click", async function () {
            await logoutAndRefresh();
        });
    }

    document.addEventListener("DOMContentLoaded", function () {
        bindSignIn();
        bindProfileLogoutButton();
        refreshAuthUI();
    });
})();

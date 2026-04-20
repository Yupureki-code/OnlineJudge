// Shared Clerk Authentication Script
// This file provides unified authentication handling for all pages

let clerkInitialized = false;
let userAuth, userProfile, modal, signinContainer, closeModalBtn;
let userCreated = false;
let isCreatingUser = false;

// 检查用户是否存在于数据库
async function checkUserExists(email) {
    try {
        const response = await fetch('/api/user/check', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ email })
        });
        return await response.json();
    } catch (error) {
        console.error('检查用户失败:', error);
        return { exists: false };
    }
}

// 创建新用户
async function createUser(name, email) {
    try {
        const response = await fetch('/api/user/create', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ name, email })
        });
        return await response.json();
    } catch (error) {
        console.error('创建用户失败:', error);
        return { created: false };
    }
}

// 获取用户信息
async function getUserInfo(email) {
    try {
        const response = await fetch('/api/user/get', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ email })
        });
        return await response.json();
    } catch (error) {
        console.error('获取用户信息失败:', error);
        return { found: false };
    }
}

// 显示用户信息弹窗
function showUserInfoModal() {
    const modal = document.createElement('div');
    modal.className = 'user-info-modal';
    modal.style.cssText = `
        position: fixed;
        top: 0;
        left: 0;
        width: 100%;
        height: 100%;
        background-color: rgba(0, 0, 0, 0.7);
        backdrop-filter: blur(5px);
        display: flex;
        justify-content: center;
        align-items: center;
        z-index: 10001;
    `;

    const modalContent = document.createElement('div');
    modalContent.style.cssText = `
        background-color: rgba(60, 80, 60, 0.95);
        padding: 30px;
        border-radius: 20px;
        width: 400px;
        border: 1px solid rgba(244, 246, 240, 0.15);
        box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3);
        backdrop-filter: blur(10px);
    `;

    modalContent.innerHTML = `
        <h2 style="margin-bottom: 20px; text-align: center; color: #F4F6F0; font-size: 20px; font-weight: 400; letter-spacing: 0.02em;">完善个人信息</h2>
        <form id="user-info-form">
            <div style="margin-bottom: 20px;">
                <label style="display: block; margin-bottom: 8px; color: rgba(244, 246, 240, 0.8); font-size: 14px;">用户名</label>
                <input type="text" id="username" placeholder="请输入用户名" style="width: 100%; padding: 12px; border: 1px solid rgba(244, 246, 240, 0.2); border-radius: 10px; background: rgba(255, 255, 255, 0.05); color: #F4F6F0; font-size: 14px;">
            </div>
            <div style="display: flex; justify-content: space-between;">
                <button type="button" id="cancel-btn" style="padding: 10px 20px; border: 1px solid rgba(244, 246, 240, 0.2); border-radius: 10px; background: rgba(255, 255, 255, 0.05); color: rgba(244, 246, 240, 0.8); cursor: pointer; font-size: 14px; transition: all 0.3s ease;">取消</button>
                <button type="submit" style="padding: 10px 20px; border: none; border-radius: 10px; background: rgba(76, 175, 80, 0.2); color: #4CAF50; cursor: pointer; font-size: 14px; font-weight: 400; transition: all 0.3s ease;">保存</button>
            </div>
        </form>
    `;

    modal.appendChild(modalContent);
    document.body.appendChild(modal);

    document.getElementById('cancel-btn').addEventListener('click', function() {
        document.body.removeChild(modal);
    });

    const cancelBtn = document.getElementById('cancel-btn');
    cancelBtn.addEventListener('mouseenter', function() {
        this.style.background = 'rgba(255, 255, 255, 0.1)';
        this.style.borderColor = 'rgba(244, 246, 240, 0.4)';
    });
    cancelBtn.addEventListener('mouseleave', function() {
        this.style.background = 'rgba(255, 255, 255, 0.05)';
        this.style.borderColor = 'rgba(244, 246, 240, 0.2)';
    });

    const saveBtn = modalContent.querySelector('button[type="submit"]');
    saveBtn.addEventListener('mouseenter', function() {
        this.style.background = 'rgba(76, 175, 80, 0.3)';
        this.style.transform = 'translateY(-2px)';
    });
    saveBtn.addEventListener('mouseleave', function() {
        this.style.background = 'rgba(76, 175, 80, 0.2)';
        this.style.transform = 'translateY(0)';
    });
    saveBtn.addEventListener('click', async function(e) {
        e.preventDefault();
        const usernameInput = document.getElementById('username');
        const name = usernameInput ? usernameInput.value : '';

        if (!name) {
            alert('请输入用户名');
            return;
        }

        let email = null;
        const user = Clerk.user;
        if (user && typeof user === 'object') {
            if (user.primaryEmailAddress && user.primaryEmailAddress.emailAddress) {
                email = user.primaryEmailAddress.emailAddress;
            } else if (user.emailAddresses && user.emailAddresses.length > 0) {
                const firstEmail = user.emailAddresses[0];
                email = firstEmail.emailAddress || firstEmail.email;
            }
        }

        if (!email) {
            alert('无法获取用户邮箱地址，请重试');
            return;
        }

        isCreatingUser = true;
        const result = await createUser(name, email);
        
        if (result && result.created) {
            alert('个人信息保存成功！');
            document.body.removeChild(modal);
            setTimeout(async () => {
                await checkUserExists(email);
                updateAuthUI();
            }, 1000);
        } else {
            alert('个人信息保存失败，请重试');
        }
        isCreatingUser = false;
    });
}

// 更新右上角 UI
async function updateAuthUI() {
    if (!clerkInitialized || !userAuth || !userProfile) return;
    
    const isLoggedIn = typeof SERVER_USER_INFO !== 'undefined' && SERVER_USER_INFO.isLoggedIn || (Clerk.user && Clerk.user.id);
    
    if (isLoggedIn) {
        userAuth.style.display = 'none';
        userProfile.style.display = 'flex';
        userProfile.style.alignItems = 'center';
        
        if (userProfile.querySelector('.avatar-wrapper')) {
            return;
        }
        
        userProfile.innerHTML = '';
        
        const chatIcon = document.createElement('a');
        chatIcon.href = '#';
        chatIcon.innerHTML = '<img src="/pictures/message.png" width="40px" height="40px">';
        userProfile.appendChild(chatIcon);

        const avatarWrapper = document.createElement('div');
        avatarWrapper.className = 'avatar-wrapper';
        avatarWrapper.style.cssText = 'position: relative; margin-left: 10px; cursor: pointer;';

        const avatarContainer = document.createElement('div');
        avatarContainer.style.cssText = 'position: relative; width: 40px; height: 40px;';

        const avatarImg = document.createElement('img');
        avatarImg.src = '/pictures/head.jpg';
        avatarImg.width = 40;
        avatarImg.height = 40;
        avatarImg.style.borderRadius = '50%';
        avatarImg.addEventListener('click', function() {
            window.location.href = './user/profile.html';
        });

        avatarContainer.appendChild(avatarImg);

        const dropdown = document.createElement('div');
        dropdown.className = 'avatar-dropdown';
        dropdown.style.cssText = `
            position: absolute;
            top: 100%;
            left: 50%;
            transform: translateX(-50%);
            margin-top: 12px;
            background-color: rgba(60, 80, 60, 0.95);
            border-radius: 16px;
            box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3);
            padding: 20px;
            width: 220px;
            display: none;
            z-index: 10000;
            border: 1px solid rgba(244, 246, 240, 0.15);
            backdrop-filter: blur(10px);
        `;

        dropdown.innerHTML = `
            <div style="display: flex; flex-direction: column; align-items: center; margin-bottom: 15px;">
                <img src="/pictures/head.jpg" width="60px" height="60px" style="border-radius: 50%; margin-bottom: 12px; border: 2px solid rgba(244, 246, 240, 0.2);">
                <div id="dropdown-username" style="font-size: 16px; font-weight: 500; color: #F4F6F0; letter-spacing: 0.02em;">用户名</div>
            </div>
            <hr style="margin: 12px 0; border: none; border-top: 1px solid rgba(244, 246, 240, 0.1);">
            <div style="display: flex; flex-direction: column; gap: 8px;">
                <button class="dropdown-btn" id="go-profile-btn" style="padding: 10px 16px; border: 1px solid rgba(244, 246, 240, 0.2); border-radius: 8px; background: rgba(255, 255, 255, 0.08); color: #F4F6F0; cursor: pointer; font-size: 13px; font-weight: 400; transition: all 0.3s ease;">个人中心</button>
                <button class="dropdown-btn" id="sign-out-btn" style="padding: 10px 16px; border: 1px solid rgba(244, 67, 54, 0.3); border-radius: 8px; background: rgba(244, 67, 54, 0.1); color: #F44336; cursor: pointer; font-size: 13px; font-weight: 400; transition: all 0.3s ease;">退出登录</button>
            </div>
        `;

        avatarWrapper.appendChild(avatarContainer);
        avatarWrapper.appendChild(dropdown);
        userProfile.appendChild(avatarWrapper);

        const connector = document.createElement('div');
        connector.style.cssText = 'position: absolute; top: 40px; left: 50%; transform: translateX(-50%); width: 200px; height: 10px; background: transparent;';
        avatarWrapper.appendChild(connector);

        avatarWrapper.onmouseenter = function() { dropdown.style.display = 'block'; };
        avatarWrapper.onmouseleave = function() { dropdown.style.display = 'none'; };

        dropdown.querySelector('#sign-out-btn').addEventListener('click', function() {
            Clerk.signOut();
        });

        dropdown.querySelector('#go-profile-btn').addEventListener('click', function() {
            window.location.href = './user/profile.html';
        });

        dropdown.querySelectorAll('.dropdown-btn').forEach(function(btn) {
            btn.addEventListener('mouseenter', function() {
                this.style.background = this.textContent === '退出登录' ? 'rgba(244, 67, 54, 0.2)' : 'rgba(255, 255, 255, 0.15)';
                this.style.transform = 'translateY(-1px)';
            });
            btn.addEventListener('mouseleave', function() {
                this.style.background = this.textContent === '退出登录' ? 'rgba(244, 67, 54, 0.1)' : 'rgba(255, 255, 255, 0.08)';
                this.style.transform = 'translateY(0)';
            });
        });

        try {
            await new Promise(resolve => setTimeout(resolve, 500));
            const user = Clerk.user;
            let email = null;
            if (user) {
                if (user.primaryEmailAddress && user.primaryEmailAddress.emailAddress) {
                    email = user.primaryEmailAddress.emailAddress;
                } else if (user.emailAddresses && user.emailAddresses.length > 0) {
                    const firstEmail = user.emailAddresses[0];
                    email = firstEmail.emailAddress || firstEmail.email;
                }
            }

            if (email) {
                const userInfo = await getUserInfo(email);
                if (userInfo && userInfo.found) {
                    const usernameElement = dropdown.querySelector('#dropdown-username');
                    if (usernameElement) {
                        usernameElement.textContent = userInfo.user.name || '用户名';
                    }
                }

                if (!isCreatingUser) {
                    const result = await checkUserExists(email);
                    if (!result.exists) {
                        const existingModal = document.querySelector('.user-info-modal');
                        if (!existingModal) {
                            showUserInfoModal();
                        }
                    }
                }
            }
        } catch (error) {
            console.error('获取用户信息失败:', error);
        }
    } else {
        userAuth.style.display = 'block';
        userProfile.style.display = 'none';
    }
}

// 显示模态框
async function showModal() {
    if (!clerkInitialized) return;
    signinContainer.innerHTML = '';
    Clerk.mountSignIn(signinContainer);
    modal.style.display = 'flex';
}

// 关闭模态框
function closeModal() {
    modal.style.display = 'none';
    if (Clerk && signinContainer && typeof Clerk.unmountSignIn === 'function') {
        Clerk.unmountSignIn(signinContainer);
        signinContainer.innerHTML = '';
    }
}

// 初始化 Clerk
async function initClerk() {
    if (typeof Clerk === 'undefined') {
        console.error('Clerk SDK 未加载');
        return;
    }

    await Clerk.load({
        ui: { ClerkUI: window.__internal_ClerkUICtor },
    });
    clerkInitialized = true;

    userAuth = document.getElementById('user-auth');
    userProfile = document.getElementById('user-profile');
    modal = document.getElementById('clerk-modal');
    signinContainer = document.getElementById('clerk-signin-container');
    closeModalBtn = document.getElementById('close-modal-btn');

    if (closeModalBtn) {
        closeModalBtn.addEventListener('click', closeModal);
    }

    const signInButton = document.getElementById('sign-in-button');
    if (signInButton) {
        signInButton.addEventListener('click', showModal);
    }

    const signOutButton = document.getElementById('sign-out-button');
    if (signOutButton) {
        signOutButton.addEventListener('click', function() {
            Clerk.signOut();
        });
    }

    Clerk.addListener(({ user }) => {
        updateAuthUI();
        if (typeof updateUserProfileInfo === 'function') {
            updateUserProfileInfo();
        }
    });

    if (typeof SERVER_USER_INFO !== 'undefined' && SERVER_USER_INFO.isLoggedIn) {
        setTimeout(updateAuthUI, 100);
    } else {
        updateAuthUI();
    }
}

// 页面加载完成后初始化
document.addEventListener('DOMContentLoaded', function() {
    if (typeof Clerk !== 'undefined' && Clerk.load) {
        initClerk();
    } else {
        window.addEventListener('load', initClerk);
    }
});

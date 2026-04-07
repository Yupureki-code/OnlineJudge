// 初始化Clerk
(async function() {
  const publishableKey = 'pk_test_cXVpZXQtbWFjYXF1ZS02My5jbGVyay5hY2NvdW50cy5kZXYk';

  if (!publishableKey) {
    throw new Error('Add your VITE_CLERK_PUBLISHABLE_KEY to the .env file');
  }

  // 从publishable key中提取Frontend API domain
  const clerkDomain = atob(publishableKey.split('_')[2]).slice(0, -1);

  // 加载Clerk UI bundle
  await new Promise((resolve, reject) => {
    const script = document.createElement('script');
    script.src = `https://${clerkDomain}/npm/@clerk/ui@1/dist/ui.browser.js`;
    script.async = true;
    script.crossOrigin = 'anonymous';
    script.onload = resolve;
    script.onerror = () => reject(new Error('Failed to load @clerk/ui bundle'));
    document.head.appendChild(script);
  });

  // 加载Clerk JS bundle
  await new Promise((resolve, reject) => {
    const script = document.createElement('script');
    script.src = `https://${clerkDomain}/npm/@clerk/clerk-js@6/dist/clerk.browser.js`;
    script.async = true;
    script.crossOrigin = 'anonymous';
    script.onload = resolve;
    script.onerror = () => reject(new Error('Failed to load @clerk/clerk-js bundle'));
    document.head.appendChild(script);
  });

  // 等待Clerk全局对象可用
  await new Promise((resolve) => {
    const checkClerk = () => {
      if (window.Clerk) {
        resolve();
      } else {
        setTimeout(checkClerk, 100);
      }
    };
    checkClerk();
  });

  // 初始化Clerk实例
  const clerk = window.Clerk;
  await clerk.load({
    ui: { ClerkUI: window.__internal_ClerkUICtor },
  });

  // 全局暴露Clerk实例
  window.Clerk = clerk;

  // 检查登录状态并挂载相应组件
  document.addEventListener('DOMContentLoaded', function() {
    const signInButton = document.getElementById('sign-in-button');
    const signOutButton = document.getElementById('sign-out-button');
    const userAuth = document.getElementById('user-auth');
    const userProfile = document.getElementById('user-profile');
    const signInContainer = document.getElementById('sign-in-container');

    // 检查登录状态
    function checkLoginStatus() {
      if (clerk.user) {
        userAuth.style.display = 'none';
        userProfile.style.display = 'block';
      } else {
        userAuth.style.display = 'block';
        userProfile.style.display = 'none';
      }
    }

    // 登录/注册按钮点击事件
    signInButton.addEventListener('click', function() {
      try {
        if (signInContainer) {
          // 显示登录容器
          signInContainer.style.display = 'flex';
          // 挂载登录组件
          clerk.mountSignIn(signInContainer);
          console.log('挂载登录组件到容器');
        } else {
          console.error('登录容器不存在');
        }
      } catch (error) {
        console.error('打开弹窗失败:', error);
      }
    });

    // 退出登录按钮点击事件
    signOutButton.addEventListener('click', function() {
      try {
        clerk.signOut();
        console.log('退出登录');
      } catch (error) {
        console.error('退出登录失败:', error);
      }
    });

    // 监听用户状态变化
    clerk.addListener(({ user }) => {
      checkLoginStatus();
      if (user) {
        console.log('用户已登录:', user);
        // 登录成功后隐藏登录容器
        if (signInContainer) {
          signInContainer.style.display = 'none';
        }
      } else {
        console.log('用户已退出登录');
      }
    });

    // 初始化登录状态
    checkLoginStatus();
  });
})();

const SPA = {
    pages: {},
    currentPage: null,

    async canAccessRoute(pageName, originalHash) {
        const protectedRoutes = ["profile"];
        if (!protectedRoutes.includes(pageName)) {
            return true;
        }

        try {
            const response = await fetch('/api/user/info', { credentials: 'include' });
            const data = await response.json();
            if (data && data.success && data.user) {
                return true;
            }
        } catch (error) {
            console.error('登录态检查失败:', error);
        }

        window.dispatchEvent(new CustomEvent('oj-auth-required', {
            detail: { fromRoute: originalHash || '#home' }
        }));
        return false;
    },

    async loadPage(hash) {
        const pageContent = document.getElementById('page-content');
        if (!pageContent) return;

        const route = hash.replace('#', '') || 'home';
        
        let queryParams = {};
        const qIndex = route.indexOf('?');
        if (qIndex !== -1) {
            const qs = route.substring(qIndex + 1);
            qs.split('&').forEach(function(pair) {
                const parts = pair.split('=');
                if (parts.length === 2) {
                    queryParams[decodeURIComponent(parts[0])] = decodeURIComponent(parts[1]);
                }
            });
        }
        
        const routeParts = route.split('/');
        const pageName = routeParts[0];
        const pageParams = routeParts.slice(1);

        const canAccess = await this.canAccessRoute(pageName, hash || '#home');
        if (!canAccess) {
            this.currentPage = 'home';
            window.location.hash = '#home';
            return;
        }
        
        const baseRoute = pageName + (pageParams.length > 0 ? '/' + pageParams.join('/') : '');
        if (this.currentPage === baseRoute) return;
        this.currentPage = baseRoute;

        pageContent.style.opacity = '0';
        
        const cacheKey = baseRoute;
        const pageHTML = this.pages[cacheKey];
        if (pageHTML) {
            pageContent.innerHTML = pageHTML;
            this.executePageScripts(pageContent, pageParams);
        } else {
            try {
                const response = await fetch(`./spa/pages/${pageName}.html`);
                if (response.ok) {
                    const html = await response.text();
                    this.pages[cacheKey] = html;
                    pageContent.innerHTML = html;
                    this.executePageScripts(pageContent, pageParams);
                } else {
                    pageContent.innerHTML = '<h1 style="color: white; text-align: center; margin-top: 100px;">404 - 页面未找到</h1>';
                }
            } catch (error) {
                console.error('加载页面失败:', error);
                pageContent.innerHTML = '<h1 style="color: white; text-align: center; margin-top: 100px;">加载失败</h1>';
            }
        }

        setTimeout(() => {
            pageContent.style.opacity = '1';
        }, 50);

        window.scrollTo(0, 0);
    },

    executePageScripts(container, params = []) {
        const scripts = container.querySelectorAll('script');
        const inlineScripts = [];
        const externalScripts = [];
        
        scripts.forEach(script => {
            if (script.src) {
                externalScripts.push(script.src);
            } else {
                inlineScripts.push(script.textContent);
            }
        });
        
        const loadExternalScripts = () => {
            return new Promise((resolve) => {
                if (externalScripts.length === 0) {
                    resolve();
                    return;
                }
                
                let loadedCount = 0;
                const checkComplete = () => {
                    loadedCount++;
                    if (loadedCount === externalScripts.length) {
                        setTimeout(resolve, 100);
                    }
                };
                
                externalScripts.forEach(src => {
                    let existingScript = document.querySelector(`script[src="${src}"]`);
                    
                    if (existingScript && typeof ace !== 'undefined') {
                        checkComplete();
                        return;
                    }
                    
                    if (existingScript) {
                        existingScript.remove();
                    }
                    
                    const newScript = document.createElement('script');
                    newScript.src = src;
                    newScript.onload = checkComplete;
                    newScript.onerror = () => {
                        console.error('Failed to load script:', src);
                        checkComplete();
                    };
                    document.head.appendChild(newScript);
                });
            });
        };
        
        loadExternalScripts().then(() => {
            inlineScripts.forEach(code => {
                try {
                    eval(code);
                } catch (e) {
                    console.error('Script execution error:', e);
                }
            });
            
            if (typeof window.initPage === 'function') {
                window.initPage({ ...queryParams, _pathParams: params });
            }
        });
    },

    init() {
        this.createRainLines();
        this.handleRoute();
        
        window.addEventListener('hashchange', () => this.handleRoute());
        
        document.addEventListener('click', (e) => {
            const anchor = e.target.closest('a');
            if (anchor && anchor.getAttribute('href')?.startsWith('#')) {
                e.preventDefault();
                const hash = anchor.getAttribute('href');
                window.location.hash = hash;
            }
        });
    },

    handleRoute() {
        const hash = window.location.hash || '#home';
        this.loadPage(hash);
    },

    createRainLines() {
        const rainLines = document.getElementById('rainLines');
        if (!rainLines) return;
        
        const lineCount = 50;
        for (let i = 0; i < lineCount; i++) {
            const line = document.createElement('div');
            line.className = 'rain-line';
            line.style.left = Math.random() * 100 + '%';
            line.style.height = (Math.random() * 100 + 50) + 'px';
            line.style.animationDuration = (Math.random() * 2 + 1) + 's';
            line.style.animationDelay = Math.random() * 5 + 's';
            rainLines.appendChild(line);
        }
    },

    registerPage(route, html) {
        this.pages[route] = html;
    }
};

document.addEventListener('DOMContentLoaded', () => {
    SPA.init();
});

const SPA = {
    pages: {},
    currentPage: null,

    async loadPage(hash) {
        const pageContent = document.getElementById('page-content');
        if (!pageContent) return;

        const route = hash.replace('#', '') || 'home';
        
        if (this.currentPage === route) return;
        this.currentPage = route;

        pageContent.style.opacity = '0';
        
        const pageHTML = this.pages[route];
        if (pageHTML) {
            pageContent.innerHTML = pageHTML;
            this.executePageScripts(pageContent);
        } else {
            const routeParts = route.split('/');
            const pageName = routeParts[0];
            const pageParams = routeParts.slice(1);
            
            try {
                const response = await fetch(`./spa/pages/${pageName}.html`);
                if (response.ok) {
                    const html = await response.text();
                    this.pages[route] = html;
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
        scripts.forEach(script => {
            const newScript = document.createElement('script');
            if (script.src) {
                newScript.src = script.src;
            } else {
                newScript.textContent = script.textContent;
            }
            document.body.appendChild(newScript).remove();
        });

        if (typeof initPage === 'function') {
            initPage(params);
        }
    },

    init() {
        this.createRainLines();
        this.handleRoute();
        
        window.addEventListener('hashchange', () => this.handleRoute());
        
        document.addEventListener('click', (e) => {
            if (e.target.tagName === 'A' && e.target.getAttribute('href')?.startsWith('#')) {
                e.preventDefault();
                const hash = e.target.getAttribute('href');
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

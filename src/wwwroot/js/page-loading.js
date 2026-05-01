(function () {
    var mask = document.getElementById("page-loading-mask");
    if (!mask) {
        return;
    }

    var textNode = mask.querySelector(".page-loading-text");

    function show(message) {
        if (textNode && message) {
            textNode.textContent = message;
        }
        mask.classList.remove("hidden");
    }

    function hide() {
        mask.classList.add("hidden");
    }

    window.__showPageLoading = show;
    window.__hidePageLoading = hide;

    // 个人中心页面不自动隐藏，由页面自己控制
    var isProfilePage = window.location.pathname.includes('/user/profile');
    
    if (!isProfilePage) {
        if (document.readyState === "complete") {
            hide();
        } else {
            document.addEventListener("DOMContentLoaded", hide);
            window.addEventListener("load", hide);
        }

        // Back/forward cache restore does not always trigger load again.
        // Ensure the global mask is cleared whenever a page is shown.
        window.addEventListener("pageshow", hide);
    }

    document.addEventListener("click", function (e) {
        var link = e.target.closest("a");
        if (!link) {
            return;
        }

        if (link.dataset && link.dataset.ajaxNav === "true") {
            return;
        }

        var href = link.getAttribute("href") || "";
        if (!href || href.startsWith("#") || href.startsWith("javascript:")) {
            return;
        }

        if (link.target === "_blank" || e.metaKey || e.ctrlKey || e.shiftKey || e.altKey) {
            return;
        }

        show("加载中...");
    });

    document.addEventListener("submit", function () {
        show("加载中...");
    });
})();

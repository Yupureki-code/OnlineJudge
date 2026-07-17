(function (global) {
    "use strict";

    function text(value) {
        return document.createTextNode(String(value == null ? "" : value));
    }

    function card(question) {
        var link = document.createElement("a");
        link.href = "/questions/" + encodeURIComponent(question.id);
        link.style.cssText = "text-decoration:none;position:relative";
        var node = document.createElement("div");
        node.className = "question-card visible";
        var number = document.createElement("div");
        number.className = "question-number";
        number.appendChild(text(question.id));
        var info = document.createElement("div");
        info.className = "question-info";
        var title = document.createElement("div");
        title.className = "question-title";
        title.appendChild(text(question.title));
        var meta = document.createElement("div");
        meta.className = "question-meta";
        var difficulty = document.createElement("span");
        difficulty.className = "difficulty difficulty-" + String(question.difficulty || "").toLowerCase();
        difficulty.appendChild(text(question.difficulty));
        meta.appendChild(difficulty);
        info.appendChild(title);
        info.appendChild(meta);
        var arrow = document.createElement("div");
        arrow.className = "arrow-icon";
        arrow.textContent = "\u2192";
        node.appendChild(number);
        node.appendChild(info);
        node.appendChild(arrow);
        link.appendChild(node);
        return link;
    }

    function render(data, filters) {
        var container = document.querySelector(".questions-container");
        if (!container) return;
        container.replaceChildren();
        (data.questions || []).forEach(function (question) { container.appendChild(card(question)); });
        if (!container.children.length) {
            var empty = document.createElement("div");
            empty.className = "loading-inline";
            empty.textContent = "暂无匹配题目";
            container.appendChild(empty);
        }
        var page = data.page || {};
        var pageInfo = document.getElementById("pageInfo");
        if (pageInfo) pageInfo.textContent = "第 " + (page.page || filters.page) + " 页 / 共 " + (page.total_pages || 1) + " 页";
    }

    async function load(filters) {
        filters = filters || { page: 1, size: 5, difficulty: "all", keyword: "" };
        var params = new URLSearchParams({
            page: String(Math.max(1, filters.page || 1)),
            page_size: String(Math.max(1, filters.size || 5))
        });
        if (filters.difficulty && filters.difficulty !== "all") params.set("difficulty", filters.difficulty);
        if (filters.keyword) params.set("search", filters.keyword.trim());
        var data = await global.OJApi.request("/api/questions?" + params.toString());
        render(data, filters);
        return data;
    }

    global.OJQuestionList = { load: load, render: render };
    document.addEventListener("DOMContentLoaded", function () {
        var params = new URLSearchParams(global.location.search);
        load({
            page: Number(params.get("page") || 1),
            size: Number(params.get("size") || 5),
            difficulty: params.get("difficulty") || "all",
            keyword: params.get("q") || ""
        }).catch(function () {
            var container = document.querySelector(".questions-container");
            if (container) container.textContent = "题目加载失败，请稍后重试";
        });
    });
})(window);

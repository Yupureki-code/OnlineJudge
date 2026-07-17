(function (global) {
    "use strict";

    function questionId() {
        var match = global.location.pathname.match(/\/questions\/([^/]+)/);
        return match ? decodeURIComponent(match[1]) : "";
    }

    function render(question) {
        document.title = question.title + " - Yupureki-OJ";
        var title = document.querySelector(".question-title");
        if (title) title.textContent = question.id + ". " + question.title;
        var badge = document.querySelector(".difficulty-badge");
        if (badge) {
            badge.textContent = question.difficulty;
            badge.className = "difficulty-badge difficulty-" + String(question.difficulty || "").toLowerCase();
        }
        var description = document.querySelector(".description");
        if (description) description.textContent = question.description || "";
        var meta = document.querySelectorAll(".meta-item span:last-child");
        if (meta[0]) meta[0].textContent = Math.ceil(Number(question.time_limit_ms || 0) / 1000) + "秒";
        if (meta[1]) meta[1].textContent = Number(question.memory_limit_mb || 0) + "MB";
    }

    async function load() {
        var id = questionId();
        if (!id) throw new Error("QUESTION_ID_REQUIRED");
        var data = await global.OJApi.request("/api/questions/" + encodeURIComponent(id));
        render(data.question);
        return data.question;
    }

    function installSubmission() {
        var form = document.querySelector(".code-editor form");
        if (!form) return;
        form.action = "#";
        form.addEventListener("submit", async function (event) {
            event.preventDefault();
            event.stopImmediatePropagation();
            var button = form.querySelector("button[type=submit]");
            var editor = global.ace && global.ace.edit("editor");
            var code = editor ? editor.getValue() : (document.getElementById("code") || {}).value || "";
            if (!code.trim()) return;
            if (button) button.disabled = true;
            try {
                var data = await global.OJApi.request("/api/questions/" + encodeURIComponent(questionId()) + "/submissions", {
                    method: "POST",
                    headers: { "Content-Type": "application/json" },
                    body: JSON.stringify({ code: code, language: "cpp" })
                });
                var submission = data.submission || {};
                var id = String(submission.submission_id || "");
                if (!id) throw new Error("SUBMISSION_ID_MISSING");
                sessionStorage.setItem("oj_submission_" + id, JSON.stringify({ question_id: questionId(), created_at: Date.now() }));
                global.location.href = "/judge_result?submission_id=" + encodeURIComponent(id) + "&id=" + encodeURIComponent(questionId());
            } catch (error) {
                global.alert(error.message === "UNAUTHORIZED" ? "请先登录" : "提交失败，请稍后重试");
                if (button) button.disabled = false;
            }
        }, true);
    }

    document.addEventListener("DOMContentLoaded", function () {
        load().catch(function () {
            var description = document.querySelector(".description");
            if (description) description.textContent = "题目加载失败，请稍后重试";
        });
        installSubmission();
    });

    global.OJQuestionPage = { load: load, render: render };
})(window);

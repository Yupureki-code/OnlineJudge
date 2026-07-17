(function (global) {
    "use strict";

    if (!global.OJCsrf) throw new Error("OJCsrf must load before OJApi");
    var nativeFetch = global.OJCsrf.nativeFetch;

    function requestId() {
        if (global.crypto && typeof global.crypto.randomUUID === "function") {
            return global.crypto.randomUUID();
        }
        var bytes = new Uint8Array(16);
        global.crypto.getRandomValues(bytes);
        return Array.from(bytes, function (value) {
            return value.toString(16).padStart(2, "0");
        }).join("");
    }

    function remapPath(path) {
        var exact = {
            "/api/user/info": "/api/user",
            "/api/user/stats": "/api/user/statistics",
            "/api/user/name": "/api/user/profile",
            "/api/user/logout": "/api/auth/logout",
            "/api/auth/send_code": "/api/auth/registration-code",
            "/api/user/password/login": "/api/auth/login/password",
            "/api/user/security/send_code": "/api/user/security-code",
            "/api/user/email/change": "/api/user/email",
            "/api/user/password/change": "/api/user/password/change",
            "/api/user/account/delete": "/api/user"
        };
        if (exact[path]) return exact[path];
        if (path.indexOf("/api/admin/") === 0) path = "/admin/api/" + path.slice(11);
        path = path.replace(/^\/admin\/api\/questions\/([^/]+)\/tests$/, "/admin/api/questions/$1/test-cases");
        path = path.replace(/^\/admin\/api\/tests\/([^/]+)$/, "/admin/api/test-cases/$1");
        path = path.replace(/^\/api\/question\/([^/]+)\/pass_status$/, "/api/questions/$1/pass-status");
        path = path.replace(/^\/api\/question\/([^/]+)\/sample_tests$/, "/api/questions/$1/test-cases");
        path = path.replace(/^\/api\/question\/([^/]+)\/test$/, "/api/questions/$1/custom-tests");
        path = path.replace(/^\/api\/question\/([^/]+)\/submits$/, "/api/submissions");
        path = path.replace(/^\/api\/question\/([^/]+)$/, "/api/questions/$1");
        return path;
    }

    function remapUrl(input) {
        var rawUrl = typeof input === "string" || input instanceof URL ? input : input.url;
        var url = new URL(rawUrl, global.location.origin);
        if (url.origin !== global.location.origin) return url;
        var submissionHistory = url.pathname.match(/^\/api\/question\/([^/]+)\/submits$/);
        url.pathname = remapPath(url.pathname);
        if (submissionHistory) url.searchParams.set("question_id", submissionHistory[1]);
        if (url.searchParams.has("size")) {
            url.searchParams.set("page_size", url.searchParams.get("size"));
            url.searchParams.delete("size");
        }
        if (url.searchParams.has("q")) {
            url.searchParams.set("search", url.searchParams.get("q"));
            url.searchParams.delete("q");
        }
        if (url.searchParams.has("sort")) {
            url.searchParams.set("sort_by", url.searchParams.get("sort"));
            url.searchParams.delete("sort");
        }
        return url;
    }

    function parseJsonBody(body) {
        if (!body) return null;
        try { return JSON.parse(body); } catch (error) { return null; }
    }

    function int64String(value) {
        return value === undefined || value === null || value === "" ? value : String(value);
    }

    function mapQuestion(question) {
        if (!question) return question;
        question.number = question.number || question.id;
        question.desc = question.desc || question.description;
        question.star = question.star || question.difficulty;
        question.cpu_limit = question.cpu_limit || Math.ceil(Number(question.time_limit_ms || 0) / 1000);
        question.memory_limit = question.memory_limit || Number(question.memory_limit_mb || 0);
        return question;
    }

    function transformBody(originalPath, mappedPath, method, payload) {
        if (!payload) return payload;
        if (originalPath === "/api/user/password/login") {
            return { email_or_name: payload.username || payload.email_or_name || "", password: payload.password || "" };
        }
        if (originalPath === "/api/auth/verify_code") {
            if (payload.name || payload.password) {
                return {
                    email: payload.email || "",
                    verification_code: payload.code || payload.verification_code || "",
                    name: payload.name || "",
                    password: payload.password || ""
                };
            }
            return { email: payload.email || "", verification_code: payload.code || payload.verification_code || "" };
        }
        if (originalPath === "/api/user/email/change") {
            return { new_email: payload.email || payload.new_email || "", verification_code: payload.code || "" };
        }
        if (originalPath === "/api/user/password/change") {
            return { verification_code: payload.code || "", new_password: payload.new_password || "" };
        }
        if (originalPath === "/api/user/account/delete") {
            return { verification_code: payload.code || "" };
        }
        if (/^(?:\/api\/question\/[^/]+\/test|\/api\/questions\/[^/]+\/custom-tests)$/.test(originalPath)) {
            return {
                code: payload.code || "",
                language: payload.language || "cpp",
                custom_input: payload.custom_input || ""
            };
        }
        if (/^(?:\/api\/admin|\/admin\/api)\/questions(?:\/[^/]+)?$/.test(originalPath)) {
            return {
                question: {
                    id: String(payload.number || ""),
                    title: payload.title || "",
                    description: payload.desc || "",
                    difficulty: payload.star || "",
                    time_limit_ms: Number(payload.cpu_limit || 0) * 1000,
                    memory_limit_mb: Number(payload.memory_limit || 0)
                }
            };
        }
        if (/^(?:\/api\/admin\/(?:questions\/[^/]+\/tests|tests\/[^/]+)|\/admin\/api\/(?:questions\/[^/]+\/test-cases|test-cases\/[^/]+))$/.test(originalPath)) {
            var testCase = {
                ordinal: Number(payload.ordinal || 0),
                input: payload["in"] || payload.input || "",
                expected_output: payload["out"] || payload.expected_output || "",
                is_sample: !!payload.is_sample
            };
            if (method === "PUT") testCase.test_case_id = originalPath.split("/").pop();
            return method === "PUT" ? { test_case: testCase } : testCase;
        }
        return payload;
    }

    async function transformFormData(originalPath, formData) {
        if (originalPath !== "/api/user/avatar") return null;
        var file = formData.get("avatar");
        if (!file) return { content: "", content_type: "" };
        if (file.size > 2 * 1024 * 1024) throw new Error("FILE_TOO_LARGE");
        var bytes = new Uint8Array(await file.arrayBuffer());
        var chunks = [];
        for (var offset = 0; offset < bytes.length; offset += 0x8000) {
            chunks.push(String.fromCharCode.apply(null, bytes.subarray(offset, offset + 0x8000)));
        }
        return { content: btoa(chunks.join("")), content_type: file.type || "application/octet-stream" };
    }

    function normalize(payload) {
        if (!payload || typeof payload !== "object") return payload;
        if (payload.status) {
            payload.success = !!payload.status.success;
            payload.error_code = payload.status.message || "";
            payload.message = payload.status.message || payload.message || "";
        }
        if (payload.page) {
            payload.total = payload.page.total_items;
            payload.total_pages = payload.page.total_pages;
            payload.page_number = payload.page.page;
            payload.page_size = payload.page.page_size;
        }
        if (payload.question) {
            mapQuestion(payload.question);
            payload.data = payload.question;
        }
        if (Array.isArray(payload.questions)) payload.questions.forEach(mapQuestion);
        if (Array.isArray(payload.test_cases)) {
            payload.tests = payload.test_cases.map(function (item) {
                item.test_case_id = int64String(item.test_case_id);
                item.id = int64String(item.id !== undefined && item.id !== null ? item.id : item.test_case_id);
                item.output = item.output || item.expected_output;
                return item;
            });
        }
        if (Array.isArray(payload.sample_tests)) {
            payload.tests = payload.sample_tests.map(function (item) {
                item.test_case_id = int64String(item.test_case_id);
                item.id = int64String(item.id !== undefined && item.id !== null ? item.id : item.test_case_id);
                item.output = item.output || item.expected_output;
                return item;
            });
        }
        if (Array.isArray(payload.submissions)) {
            payload.submissions.forEach(function (submission) {
                submission.submission_id = int64String(submission.submission_id);
            });
            payload.submits = payload.submissions;
        }
        if (payload.submission && payload.submission.submission_id) {
            payload.submission.submission_id = int64String(payload.submission.submission_id);
            payload.submission_id = payload.submission.submission_id;
            payload.submission_status = payload.submission.status;
        }
        if (payload.solution) {
            Object.assign(payload, payload.solution);
            if (payload.author) {
                payload.author_name = payload.author.name;
                payload.author_avatar = payload.author.avatar_url;
            }
            if (payload.current_user_actions) Object.assign(payload, payload.current_user_actions);
        }
        if (payload.comment) Object.assign(payload, payload.comment);
        if (Array.isArray(payload.comments)) {
            payload.comments.forEach(function (comment) {
                comment.author_name = comment.author_name || ("用户 " + comment.user_id);
                comment.author_uid = comment.user_id;
            });
        }
        if (payload.actions && !Array.isArray(payload.actions)) Object.assign(payload, payload.actions);
        if (payload.avatar) payload.avatar_url = payload.avatar.url || payload.avatar.avatar_url || "";
        return payload;
    }

    async function clientFetch(input, init) {
        var rawUrl = typeof input === "string" || input instanceof URL ? input : input.url;
        var originalUrl = new URL(rawUrl, global.location.origin);
        var requestInput = typeof input === "string" ? originalUrl.href : input;
        var sourceRequest = new Request(requestInput, init || {});
        if (originalUrl.origin !== global.location.origin ||
            (originalUrl.pathname.indexOf("/api/") !== 0 && originalUrl.pathname.indexOf("/admin/api/") !== 0)) {
            return nativeFetch(sourceRequest);
        }

        var url = remapUrl(input);
        var method = sourceRequest.method.toUpperCase();
        var headers = new Headers(sourceRequest.headers);
        headers.set("Accept", "application/json");
        headers.set("X-Request-Id", headers.get("X-Request-Id") || requestId());

        var payload = null;
        var body = null;
        if (sourceRequest.body) {
            var contentType = sourceRequest.headers.get("Content-Type") || "";
            if (contentType.indexOf("multipart/form-data") === 0) {
                payload = await transformFormData(originalUrl.pathname, await sourceRequest.clone().formData());
            } else {
                var bodyBytes = await sourceRequest.clone().arrayBuffer();
                body = bodyBytes;
                payload = parseJsonBody(new TextDecoder().decode(bodyBytes));
            }
        }
        if (payload && originalUrl.pathname === "/api/user/avatar") {
            method = "PUT";
        }
        if (originalUrl.pathname === "/api/auth/verify_code") {
            url.pathname = payload && (payload.name || payload.password)
                ? "/api/auth/register"
                : "/api/auth/login/code";
        }
        if (originalUrl.pathname === "/api/user/name") method = "PATCH";
        if (originalUrl.pathname === "/api/user/email/change" ||
            originalUrl.pathname === "/api/user/password/change") method = "PUT";
        if (originalUrl.pathname === "/api/user/account/delete") method = "DELETE";
        payload = transformBody(originalUrl.pathname, url.pathname, method, payload);
        if (payload !== null) {
            body = JSON.stringify(payload);
            headers.set("Content-Type", "application/json");
        }

        async function send(forceCsrf) {
            var sendHeaders = new Headers(headers);
            if (method !== "GET" && method !== "HEAD") {
                var area = global.OJCsrf.areaForPath(url.pathname);
                sendHeaders.set("X-CSRF-Token", await global.OJCsrf.getToken(area, forceCsrf));
            }
            var requestInit = {
                method: method,
                headers: sendHeaders,
                credentials: "include",
                cache: sourceRequest.cache,
                redirect: sourceRequest.redirect,
                referrer: sourceRequest.referrer,
                referrerPolicy: sourceRequest.referrerPolicy,
                integrity: sourceRequest.integrity,
                keepalive: sourceRequest.keepalive,
                mode: sourceRequest.mode,
                signal: sourceRequest.signal
            };
            if (method !== "GET" && method !== "HEAD" && body !== null) requestInit.body = body;
            return nativeFetch(new Request(url.href, requestInit));
        }

        var response = await send(false);
        if (response.status === 403 && method !== "GET" && method !== "HEAD") {
            var retryBody = await response.clone().json().catch(function () { return {}; });
            if (retryBody.status && retryBody.status.message === "CSRF_TOKEN_INVALID") response = await send(true);
        }

        var responseContentType = response.headers.get("Content-Type") || "";
        if (responseContentType.indexOf("application/json") < 0) return response;
        var data = normalize(await response.clone().json());
        var responseHeaders = new Headers(response.headers);
        responseHeaders.set("Content-Type", "application/json; charset=utf-8");
        var normalizedResponse = new Response(JSON.stringify(data), {
            status: response.status,
            statusText: response.statusText,
            headers: responseHeaders
        });
        ["url", "redirected", "type"].forEach(function (property) {
            try { Object.defineProperty(normalizedResponse, property, { value: response[property] }); } catch (error) {}
        });
        return normalizedResponse;
    }

    async function request(path, options) {
        var response = await clientFetch(path, options || {});
        var data = await response.json().catch(function () { return {}; });
        if (!response.ok) {
            var error = new Error(data.error_code || data.message || "REQUEST_FAILED");
            error.status = response.status;
            error.data = data;
            throw error;
        }
        return data;
    }

    global.OJApi = { fetch: clientFetch, normalize: normalize, request: request };
})(window);

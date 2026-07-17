(function (global) {
    "use strict";

    var waiting = new Set([
        "SUBMISSION_STATUS_UNSPECIFIED",
        "SUBMISSION_STATUS_PENDING",
        "SUBMISSION_STATUS_QUEUED",
        "SUBMISSION_STATUS_JUDGING"
    ]);

    function sleep(milliseconds) {
        return new Promise(function (resolve) { global.setTimeout(resolve, milliseconds); });
    }

    async function poll(path, options) {
        options = options || {};
        var timeout = options.timeout || 120000;
        var started = Date.now();
        var delay = options.initialDelay || 700;
        var lastError = null;
        while (Date.now() - started < timeout) {
            try {
                var data = await global.OJApi.request(path);
                var status = data.submission_status || (data.submission && data.submission.status) || "";
                if (typeof options.onUpdate === "function") options.onUpdate(data, status);
                if (!waiting.has(status)) return data;
                lastError = null;
            } catch (error) {
                if (error.status === 401 || error.status === 403 || error.status === 404) throw error;
                lastError = error;
            }
            await sleep(delay);
            delay = Math.min(3000, Math.round(delay * 1.35));
        }
        var timeoutError = new Error(lastError ? "POLL_NETWORK_TIMEOUT" : "POLL_TIMEOUT");
        timeoutError.cause = lastError;
        throw timeoutError;
    }

    function pollSubmission(submissionId, options) {
        return poll("/api/submissions/" + encodeURIComponent(String(submissionId)), options);
    }

    function pollCustom(taskId, options) {
        return poll("/api/custom-tests/" + encodeURIComponent(String(taskId)), options);
    }

    global.OJSubmissionPoller = {
        poll: poll,
        pollCustom: pollCustom,
        pollSubmission: pollSubmission
    };
})(window);
